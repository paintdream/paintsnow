#include "ShapeComponent.h"

using namespace PaintsNow;

ShapeComponent::ShapeComponent() {}
ShapeComponent::~ShapeComponent() {
	Cleanup();
}

static inline UShort3 ToLocalInt(const Float3Pair& bound, const Float3& pt, uint32_t divCount) {
	Float3 local = ToLocal(bound, pt);
	UShort3 ret;
	for (uint32_t i = 0; i < 3; i++) {
		ret[i] = safe_cast<uint16_t>(Math::Clamp((uint32_t)(local[i] * divCount), 0u, divCount - 1));
	}

	return ret;
}

struct CodeIndex {
	CodeIndex(uint32_t c = 0, uint32_t i = 0, uint32_t l = 0) : code(c), index(i), level(l) {}
	bool operator < (const CodeIndex& rhs) const {
		return code < rhs.code;
	}

	uint32_t code;
	uint32_t index;
	uint32_t level;
};

// Generate Z-code
static inline CodeIndex Encode(const UShort3Pair& box, uint32_t level, uint32_t i) {
	uint32_t code = 0;
	uint16_t bitMask = 1 << (level - 1);
	while (level > 0) {
		uint16_t start = 0, end = 0;
		for (uint32_t i = 0; i < 3; i++) {
			start |= !!(box.first[i] & bitMask) << i;
			end |= !!(box.second[i] & bitMask) << i;
		}

		code = (code << 3) | start;
		if (start != end) {
			break;
		}

		bitMask >>= 1;
		--level;
	}

	// remaining
	return CodeIndex(code << (3 * level), i, level);
}

void ShapeComponent::MakeHeapInternal(std::vector<Patch>& output, Patch* begin, Patch* end) {
	if (begin >= end) return;

	Patch* mid = begin + (end - begin) / 2;
	output.emplace_back(std::move(*mid));
	MakeHeapInternal(output, begin, mid);
	MakeHeapInternal(output, mid + 1, end);
}

ShapeComponent::Patch* ShapeComponent::MakeBound(Patch& patch, const std::vector<Float3>& vertices, const std::vector<UInt3>& indices, int index) {
	Float3Pair box;
	for (uint32_t i = 0; i < MAX_PATCH_COUNT; i++) {
		uint32_t index = patch.indices[i];
		if (index == ~(uint32_t)0) return &patch;
		const UInt3& face = indices[index];
		const Float3& first = vertices[face[0]];
		const Float3& second = vertices[face[1]];
		const Float3& third = vertices[face[2]];

		if (i == 0) {
			box = Float3Pair(first, first);
		} else {
			Union(box, first);
		}

		Union(box, second);
		Union(box, third);
	}

	patch.SetIndex(index);
	patch.SetKey(box);
	return &patch;
}

void ShapeComponent::Cleanup() {
	if (meshResource) {
		meshResource->Unmap();
		meshResource = nullptr;
	}
}

void ShapeComponent::Update(Engine& engine, const TShared<MeshResource>&resource) {
	static_assert(sizeof(Patch) == 64, "Patch size must be 64.");
	if (resource == meshResource) return;

	SnowyStream& snowyStream = engine.snowyStream;
	Cleanup();

	resource->Map();
	meshResource = resource;

	IAsset::MeshCollection& meshCollection = meshResource->meshCollection;
	Float3Pair bound = meshResource->boundingBox;

	// Build Tree
	const std::vector<Float3>& vertices = meshCollection.vertices;
	const std::vector<UInt3>& indices = meshCollection.indices;

	if (indices.empty()) return;

	// safe
	for (uint32_t n = 0; n < 3; n++) {
		if (bound.second[n] - bound.first[n] < 1e-4f) {
			bound.second[n] = bound.first[n] + 1e-4f;
		}
	}

	// convert to local position
	uint32_t level = Math::Min(Math::Log2(indices.size() / 8), (uint32_t)(sizeof(uint32_t) * 8 / 3));
	uint32_t divCount = 1 << level;

	std::vector<CodeIndex> codeIndices;
	codeIndices.reserve(indices.size());
	for (uint32_t i = 0; i < indices.size(); i++) {
		const UInt3& index = indices[i];
		UShort3 first = ToLocalInt(bound, vertices[index.x()], divCount);
		UShort3Pair box(first, first);
		Union(box, ToLocalInt(bound, vertices[index.y()], divCount));
		Union(box, ToLocalInt(bound, vertices[index.z()], divCount));
		codeIndices.emplace_back(Encode(box, level, i));
	}

	// sort by z-code
	std::sort(codeIndices.begin(), codeIndices.end());

	// make tree
	std::vector<Patch> linearPatches;
	linearPatches.reserve(indices.size());

	Patch patch;
	uint32_t k = 0;
	uint32_t lastLevel = ~(uint32_t)0;
	for (uint32_t m = 0; m < codeIndices.size(); m++) {
		const CodeIndex& codeIndex = codeIndices[m];
		if (codeIndex.level != lastLevel || k == MAX_PATCH_COUNT) {
			if (!linearPatches.empty() && k != MAX_PATCH_COUNT) {
				linearPatches.back().indices[k] = ~(uint32_t)0;
			}

			lastLevel = codeIndex.level;
			k = 0;
			linearPatches.emplace_back(Patch());
		}

		linearPatches.back().indices[k++] = codeIndex.index;
	}

	if (!linearPatches.empty() && k != MAX_PATCH_COUNT) {
		linearPatches.back().indices[k++] = ~(uint32_t)0;
	}

	// heap order
	std::vector<Patch> newPatches;
	newPatches.reserve(linearPatches.size());
	MakeHeapInternal(newPatches, &linearPatches[0], &linearPatches[0] + linearPatches.size());

	// Connect
	Patch* root = MakeBound(newPatches[0], vertices, indices, 0);
	for (uint32_t s = 1; s < newPatches.size(); s++) {
		root->Attach(MakeBound(newPatches[s], vertices, indices, s % 6));
	}

	std::swap(newPatches, patches);
}

struct ShapeComponent::PatchRaycaster {
	PatchRaycaster(const std::vector<Float3>& v, const std::vector<UInt3>& i, const Float3Pair& r) : vertices(v), indices(i), ray(r), hitPatch(nullptr), hitIndex(0), distance(FLT_MAX) {}

	bool operator () (const Float3Pair& box, const TKdTree<Float3Pair>& node) {
		const Patch& patch = static_cast<const Patch&>(node);
		if (Math::IntersectBox(box, ray)) {
			for (uint32_t i = 0; i < MAX_PATCH_COUNT; i++) {
				uint32_t idx = patch.indices[i];
				if (idx == ~(uint32_t)0) break;

				const UInt3& index = indices[idx];
				const Float3 points[3] = {
					vertices[index.x()], vertices[index.y()], vertices[index.z()]
				};

				Float3 res;
				Float2 uv;
				if (Math::Intersect3D(res, uv, points, ray)) {
					float s = SquareLength(res - ray.first);
					if (s < distance) {
						distance = s;
						hitPatch = &patch;
						hitIndex = idx;
						intersection = res;
					}
				}
			}

			return true;
		} else {
			return false;
		}
	}

	const std::vector<Float3>& vertices;
	const std::vector<UInt3>& indices;
	const Float3Pair& ray;
	Float3 intersection;
	const Patch* hitPatch;
	uint32_t hitIndex;
	float distance;
};

float ShapeComponent::Raycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio) const {
	if (!patches.empty()) {
		Float3Pair box(ray.first, ray.first);
		Union(box, ray.second);
		IAsset::MeshCollection& meshCollection = meshResource->meshCollection;
		PatchRaycaster q(meshCollection.vertices, meshCollection.indices, ray);
		(const_cast<Patch&>(patches[0])).Query(box, q);

		if (q.hitPatch != nullptr) {
			RaycastResult result;
			result.position = q.intersection;
			result.distance = q.distance * ratio;
			result.unit = const_cast<ShapeComponent*>(this);
			result.parent = parent;
			task.EmplaceResult(std::move(result));
		}
	}

	return ratio;
}
