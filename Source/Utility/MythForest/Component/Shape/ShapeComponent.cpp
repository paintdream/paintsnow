#include "ShapeComponent.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

ShapeComponent::ShapeComponent() {}
ShapeComponent::~ShapeComponent() {
	Cleanup();
}

static inline UShort3 ToLocalInt(const Float3Pair& bound, const Float3& pt, uint32_t divCount) {
	Float3 local = Math::ToLocal(bound, pt);
	UShort3 ret;
	for (uint32_t i = 0; i < 3; i++) {
		ret[i] = verify_cast<uint16_t>(Math::Clamp((uint32_t)(local[i] * divCount), 0u, divCount - 1));
	}

	return ret;
}

struct CodeIndex {
	CodeIndex(uint32_t c = 0, uint32_t i = 0) : code(c), index(i) {}
	bool operator < (const CodeIndex& rhs) const {
		return code < rhs.code;
	}

	uint32_t code;
	uint32_t index;
};

// Generate Z-code
static inline CodeIndex Encode(const UShort3Pair& box, uint32_t level, uint32_t index) {
	const uint16_t* p = &box.first.data[0];

	uint32_t bitMask = 1 << level;
	uint32_t code = 0;
	for (uint32_t n = 0; n < level * 6; n++) {
		size_t r = n % 6;
		code = (code << 1) | !!(p[r] & bitMask);

		if (r == 5) {
			bitMask >>= 1;
		}
	}

	// remaining
	return CodeIndex(code, index);
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
		if (index == ~(uint32_t)0) {
			assert(i != 0);
			break;
		}

		const UInt3& face = indices[index];

		for (size_t t = 0; t < 3; t++) {
			const Float3& v = vertices[face[t]];

			for (size_t m = 0; m < 3; m++) {
				if (i == 0 && t == 0 && m == 0) {
					box = Float3Pair(v, v);
				} else {
					Math::Union(box, v);
				}

				patch.vertices[i / 4][t][m][i & 3] = v[m];
			}
		}
	}

	patch.SetIndex(index);
	patch.GetKey() = box;
	return &patch;
}

void ShapeComponent::Cleanup() {
	if (meshResource) {
		meshResource->UnMap();
		meshResource = nullptr;
	}
}

void ShapeComponent::Update(Engine& engine, const TShared<MeshResource>& resource) {
	assert(!(Flag().load(std::memory_order_acquire) & TINY_PINNED));
	if (!(Flag().fetch_or(TINY_UPDATING, std::memory_order_acquire) & TINY_UPDATING)) {
		engine.GetKernel().GetThreadPool().Dispatch(CreateTaskContextFree(Wrap(this, &ShapeComponent::RoutineUpdate), std::ref(engine), resource), 1);
	}
}

void ShapeComponent::CheckBounding(Patch* root, Float3Pair& targetKey) {
#ifdef _DEBUG
	// ranged queryer
	Float3Pair entry = targetKey;
	for (Patch* p = root; p != nullptr; p = (Patch*)(p->GetRight())) {
		assert(Math::Overlap(p->GetKey(), targetKey));

		// culling
		Float3Pair before = targetKey;
		int index = p->GetIndex();
		float save = Patch::Meta::SplitPush(std::true_type(), targetKey, p->GetKey(), index);
		// cull right in left node
		if (p->GetLeft() != nullptr) {
			CheckBounding(p->GetLeft(), targetKey);
		}

		Patch::Meta::SplitPop(targetKey, index, save);
	}
#endif
}

void ShapeComponent::RoutineUpdate(Engine& engine, const TShared<MeshResource>& resource) {
	if (resource == meshResource)
		return;

	OPTICK_EVENT();

	SnowyStream& snowyStream = engine.snowyStream;
	if (resource->Map()) {
		Cleanup();
		meshResource = resource;

		IAsset::MeshCollection& meshCollection = meshResource->meshCollection;
		Float3Pair bound = meshResource->boundingBox;

		// Build Tree
		const std::vector<Float3>& vertices = meshCollection.vertices;
		const std::vector<UInt3>& indices = meshCollection.indices;

		assert(!vertices.empty() && !indices.empty());

		// safe
		for (uint32_t n = 0; n < 3; n++) {
			if (bound.second[n] - bound.first[n] < 1e-4f) {
				bound.second[n] = bound.first[n] + 1e-4f;
			}
		}

		// convert to local position
		uint32_t level = Math::Min(Math::Log2(indices.size() / 8), (uint32_t)sizeof(uint32_t) * 8 / 6);
		uint32_t divCount = 1 << (level + 1);

		std::vector<CodeIndex> codeIndices;
		codeIndices.reserve(indices.size());
		for (uint32_t i = 0; i < indices.size(); i++) {
			const UInt3& index = indices[i];
			UShort3 first = ToLocalInt(bound, vertices[index.x()], divCount);
			UShort3Pair box(first, first);
			Math::Union(box, ToLocalInt(bound, vertices[index.y()], divCount));
			Math::Union(box, ToLocalInt(bound, vertices[index.z()], divCount));
			codeIndices.emplace_back(Encode(box, level, i));
		}

		// sort by z-code
		std::sort(codeIndices.begin(), codeIndices.end());

		// make tree
		std::vector<Patch> linearPatches;

		if (!codeIndices.empty()) {
			uint32_t k = 0;
			linearPatches.reserve((codeIndices.size() + MAX_PATCH_COUNT - 1) / MAX_PATCH_COUNT);
			linearPatches.emplace_back(Patch());

			for (uint32_t m = 0; m < codeIndices.size(); m++) {
				const CodeIndex& codeIndex = codeIndices[m];
				if (k == MAX_PATCH_COUNT) {
					k = 0;
					linearPatches.emplace_back(Patch());
				}

				linearPatches.back().indices[k++] = codeIndex.index;
			}

			while (k != MAX_PATCH_COUNT) {
				linearPatches.back().indices[k++] = ~(uint32_t)0;
			}
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

		CheckBounding(root, bound);

		std::swap(newPatches, patches);
		boundingBox = bound;
	} else {
		resource->UnMap();
		assert(false);
	}

	Flag().fetch_or(TINY_PINNED, std::memory_order_relaxed);
	Flag().fetch_and(~TINY_UPDATING, std::memory_order_release);
}

struct ShapeComponent::PatchRayCuller {
	PatchRayCuller(const Float3Pair& r) : ray(r) {}
	const Float3Pair& ray;

	bool operator () (const Float3Pair& box) {
		TVector<float, 2> intersect = Math::IntersectBox(box, ray);
		return (intersect[1] >= -0.0f && intersect[0] <= intersect[1]);
	}

	std::set<TKdTree<Float3Pair, Void>*> traversed;
};

struct ShapeComponent::PatchRayCaster {
	typedef TVector<TVector<float, 4>, 3> Group;
	PatchRayCaster(const std::vector<Float3>& v, const std::vector<UInt3>& i, const Float3Pair& r) : vertices(v), indices(i), ray(r), hitPatch(nullptr), hitIndex(0), distance(FLT_MAX) {
		Math::ExtendVector(rayGroup.first, TVector<float, 3>(r.first));
		Math::ExtendVector(rayGroup.second, TVector<float, 3>(r.second));
	}

	bool operator () (const TKdTree<Float3Pair, VertexStorage>& node) {
		const Patch& patch = static_cast<const Patch&>(node);
		TVector<TVector<float, 4>, 3> res;
		TVector<TVector<float, 4>, 2> uv;

		static_assert(MAX_PATCH_COUNT % 4 == 0, "Must be 4n size");
		for (uint32_t k = 0; k < MAX_PATCH_COUNT / 4; k++) {
			Math::IntersectTriangle(res, uv, patch.vertices[k], rayGroup);
			for (uint32_t m = 0; m < 4; m++) {
				if (patch.indices[k * 4 + m] == ~(uint32_t)0)
					return true;

				if (uv[0][m] >= 0.0f && uv[1][m] >= 0.0f && uv[0][m] + uv[1][m] <= 1.0f) {
					Float3 hit(res[0][m], res[1][m], res[2][m]);
					Float3 vec = hit - ray.first;
					if (Math::DotProduct(vec, ray.second) > 0) {
						float s = Math::SquareLength(vec);
						if (s < distance) {
							distance = s;
							hitPatch = &patch;
							hitIndex = patch.indices[k * 4 + m];
							coord = Float4(uv[0][m], uv[1][m], 0, 0);
							intersection = hit;
						}
					}
				}
			}
		}

		return true;
	}

	const std::vector<Float3>& vertices;
	const std::vector<UInt3>& indices;
	std::pair<Group, Group> rayGroup;
	const Patch* hitPatch;
	uint32_t hitIndex;
	const Float3Pair& ray;
	Float3 intersection;
	float distance;
	Float4 coord;
};

float ShapeComponent::Raycast(RaycastTask& task, Float3Pair& ray, MatrixFloat4x4& transform, Unit* parent, float ratio) const {
	if (Flag().load(std::memory_order_acquire) & TINY_PINNED) {
		if (!patches.empty()) {
			OPTICK_EVENT();

			Float3Pair box = boundingBox;
			IAsset::MeshCollection& meshCollection = meshResource->meshCollection;
			PatchRayCaster q(meshCollection.vertices, meshCollection.indices, ray);
			PatchRayCuller c(ray);
			(const_cast<Patch&>(patches[0])).Query(std::true_type(), box, q, c);

			if (q.hitPatch != nullptr) {
				RaycastResult result;
				result.transform = transform;
				result.position = q.intersection;
				result.squareDistance = q.distance * ratio;
				result.faceIndex = q.hitIndex;
				result.coord = q.coord;
				result.unit = const_cast<ShapeComponent*>(this);
				result.parent = parent;
				task.EmplaceResult(std::move(result));
			}
		}

		return ratio;
	} else {
		return 0.0f;
	}
}

const TShared<MeshResource>& ShapeComponent::GetMesh() const {
	return meshResource;
}

