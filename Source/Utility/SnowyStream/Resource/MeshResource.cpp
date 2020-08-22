#include "MeshResource.h"
#include "../../../General/Interface/IShader.h"

using namespace PaintsNow;

MeshResource::MeshResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), deviceMemoryUsage(0) {}

MeshResource::~MeshResource() {}

const Float3Pair& MeshResource::GetBoundingBox() const {
	return boundingBox;
}

static inline void ClearBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer) {
	if (buffer != nullptr) {
		render.DeleteResource(queue, buffer);
		buffer = nullptr;
	}
}

void MeshResource::Attach(IRender& render, void* deviceContext) {
	// Compute boundingBox
	std::vector<Float3>& positionBuffer = meshCollection.vertices;
	if (!positionBuffer.empty()) {
		Float3Pair bound(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
		for (size_t i = 0; i < positionBuffer.size(); i++) {
			Union(bound, positionBuffer[i]);
		}

		boundingBox = std::move(bound);
	}

	GraphicResourceBase::Attach(render, deviceContext);
}

typedef IRender::Resource::BufferDescription Description;

void MeshResource::Upload(IRender& render, void* deviceContext) {
	// Update buffers ...
	if (!(Flag().fetch_or(RESOURCE_UPLOADED) & RESOURCE_UPLOADED)) {
		SpinLock(critical);

		IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
		assert(queue != nullptr);

		deviceMemoryUsage = 0;

		assert(!meshCollection.vertices.empty());
		assert(meshCollection.boneIndices.size() == meshCollection.boneWeights.size());

		uint32_t cnt = 0;
		cnt += bufferCollection.hasNormalBuffer = !meshCollection.normals.empty();
		cnt += bufferCollection.hasTangentBuffer = !meshCollection.tangents.empty();
		cnt += bufferCollection.hasColorBuffer = !meshCollection.colors.empty();
		bufferCollection.hasIndexWeightBuffer = !meshCollection.indices.empty();

		std::vector<UChar4> normalTangentColorData(meshCollection.normals.size() * cnt);
		if (cnt != 0) {
			for (size_t i = 0; i < meshCollection.vertices.size(); i++) {
				uint32_t offset = 0;
				if (bufferCollection.hasNormalBuffer) {
					normalTangentColorData[i * cnt + offset++] = meshCollection.normals[i];
				}

				if (bufferCollection.hasTangentBuffer) {
					normalTangentColorData[i * cnt + offset++] = meshCollection.tangents[i];
				}

				if (bufferCollection.hasColorBuffer) {
					normalTangentColorData[i * cnt + offset++] = meshCollection.colors[i];
				}
			}
		}

		std::vector<UChar4> boneData(meshCollection.boneIndices.size() * 2);
		for (size_t i = 0; i < meshCollection.boneIndices.size(); i++) {
			boneData[i * 2] = meshCollection.boneIndices[i];
			boneData[i * 2 + 1] = meshCollection.boneWeights[i];
		}

		deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.indexBuffer, meshCollection.indices, Description::INDEX);
		deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.positionBuffer, meshCollection.vertices, Description::VERTEX);
		deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.normalTangentColorBuffer, normalTangentColorData, Description::VERTEX, cnt);
		deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.boneIndexWeightBuffer, boneData, Description::VERTEX, 2);

		bufferCollection.texCoordBuffers.resize(meshCollection.texCoords.size(), nullptr);
		for (size_t j = 0; j < meshCollection.texCoords.size(); j++) {
			IAsset::TexCoord& texCoord = meshCollection.texCoords[j];
			deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.texCoordBuffers[j], texCoord.coords, Description::VERTEX);
		}

		if (mapCount.load(std::memory_order_relaxed) == 0) {
			meshCollection = IAsset::MeshCollection();
		}

		SpinUnLock(critical);

		Flag().fetch_and(~Tiny::TINY_MODIFIED, std::memory_order_release);
	}
}

void MeshResource::Unmap() {
	GraphicResourceBase::Unmap();

	SpinLock(critical);
	meshCollection = IAsset::MeshCollection();
	SpinUnLock(critical);
}

void MeshResource::Detach(IRender& render, void* deviceContext) {
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	assert(queue != nullptr);

	ClearBuffer(render, queue, bufferCollection.indexBuffer);
	ClearBuffer(render, queue, bufferCollection.positionBuffer);
	ClearBuffer(render, queue, bufferCollection.normalTangentColorBuffer);
	ClearBuffer(render, queue, bufferCollection.boneIndexWeightBuffer);

	for (size_t i = 0; i < bufferCollection.texCoordBuffers.size(); i++) {
		ClearBuffer(render, queue, bufferCollection.texCoordBuffers[i]);
	}

	GraphicResourceBase::Detach(render, deviceContext);
}

void MeshResource::Download(IRender& render, void* deviceContext) {
	// download from device is not supported now
	assert(false);
}

// BufferCollection
MeshResource::BufferCollection::BufferCollection() : indexBuffer(nullptr), positionBuffer(nullptr), normalTangentColorBuffer(nullptr), boneIndexWeightBuffer(nullptr), hasNormalBuffer(false), hasTangentBuffer(false), hasColorBuffer(false), hasIndexWeightBuffer(false) {}

TObject<IReflect>& MeshResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(meshCollection);
		ReflectProperty(bufferCollection)[Runtime];
	}

	return *this;
}

TObject<IReflect>& MeshResource::BufferCollection::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(indexBuffer)[Runtime][IShader::BindInput(IShader::BindInput::INDEX)];
		ReflectProperty(positionBuffer)[Runtime][IShader::BindInput(IShader::BindInput::POSITION)];
		ReflectProperty(normalTangentColorBuffer)[Runtime][IShader::BindInput(IShader::BindInput::NORMAL)][IShader::BindInput(IShader::BindInput::TANGENT)][IShader::BindInput(IShader::BindInput::COLOR)];
		ReflectProperty(boneIndexWeightBuffer)[Runtime][IShader::BindInput(IShader::BindInput::BONE_INDEX)][IShader::BindInput(IShader::BindInput::BONE_WEIGHT)];
		ReflectProperty(texCoordBuffers)[Runtime][IShader::BindInput(IShader::BindInput::TEXCOORD)];
	}

	return *this;
}

void MeshResource::BufferCollection::GetDescription(std::vector<PassBase::Parameter>& desc, std::vector<std::pair<uint32_t, uint32_t> >& offsets, PassBase::Updater& updater) const {
	// Do not pass index buffer
	if (positionBuffer != nullptr) {
		desc.emplace_back(updater[IShader::BindInput::POSITION]);
		offsets.emplace_back(std::make_pair(0, 3));
	}

	if (normalTangentColorBuffer != nullptr) {
		uint32_t offset = 0;
		if (hasNormalBuffer) {
			desc.emplace_back(updater[IShader::BindInput::NORMAL]);
			offsets.emplace_back(std::make_pair(offset, 4));
			offset += sizeof(UChar4);
		}

		if (hasNormalBuffer) {
			desc.emplace_back(updater[IShader::BindInput::TANGENT]);
			offsets.emplace_back(std::make_pair(offset, 4));
			offset += sizeof(UChar4);
		}

		if (hasColorBuffer) {
			desc.emplace_back(updater[IShader::BindInput::COLOR]);
			offsets.emplace_back(std::make_pair(offset, 4));
		}
	}

	if (boneIndexWeightBuffer != nullptr) {
		desc.emplace_back(updater[IShader::BindInput::BONE_INDEX]);
		offsets.emplace_back(std::make_pair(0, 4));
		desc.emplace_back(updater[IShader::BindInput::BONE_WEIGHT]);
		offsets.emplace_back(std::make_pair((uint32_t)sizeof(UChar4), 4));
	}

	for (uint32_t i = 0; i < texCoordBuffers.size(); i++) {
		desc.emplace_back(updater[IShader::BindInput::SCHEMA(IShader::BindInput::TEXCOORD + i)]);
		offsets.emplace_back(std::make_pair(0, 4));
	}
}

void MeshResource::BufferCollection::UpdateData(std::vector<IRender::Resource*>& data) const {
	if (positionBuffer != nullptr) {
		data.emplace_back(positionBuffer);
	}

	if (normalTangentColorBuffer != nullptr) {
		if (hasNormalBuffer) {
			data.emplace_back(normalTangentColorBuffer);
		}

		if (hasTangentBuffer) {
			data.emplace_back(normalTangentColorBuffer);
		}

		if (hasColorBuffer) {
			data.emplace_back(normalTangentColorBuffer);
		}
	}

	if (boneIndexWeightBuffer != nullptr) {
		data.emplace_back(boneIndexWeightBuffer);
		data.emplace_back(boneIndexWeightBuffer);
	}

	for (size_t i = 0; i < texCoordBuffers.size(); i++) {
		assert(texCoordBuffers[i] != nullptr);
		data.emplace_back(texCoordBuffers[i]);
	}
}

size_t MeshResource::ReportDeviceMemoryUsage() const {
	return deviceMemoryUsage;
}
