#include "MeshResource.h"
#include "../../../General/Interface/IShader.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

MeshResource::MeshResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID), deviceMemoryUsage(0) {}

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
	Flag() &= ~RESOURCE_UPLOADED;
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	assert(queue != nullptr);

	deviceMemoryUsage = 0;

	SpinLock(critical);
	bool preserve = mapCount.load(std::memory_order_relaxed) != 0;

	deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.indexBuffer, meshCollection.indices, Description::INDEX, preserve);
	deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.positionBuffer, meshCollection.vertices, Description::VERTEX, preserve);
	deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.normalBuffer, meshCollection.normals, Description::VERTEX, preserve);
	deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.tangentBuffer, meshCollection.tangents, Description::VERTEX, preserve);
	deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.colorBuffer, meshCollection.colors, Description::VERTEX, preserve);
	deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.boneIndexBuffer, meshCollection.boneIndices, Description::VERTEX, preserve);
	deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.boneWeightBuffer, meshCollection.boneWeights, Description::VERTEX, preserve);

	bufferCollection.texCoordBuffers.resize(meshCollection.texCoords.size(), nullptr);
	for (size_t i = 0; i < meshCollection.texCoords.size(); i++) {
		IAsset::TexCoord& texCoord = meshCollection.texCoords[i];
		deviceMemoryUsage += UpdateBuffer(render, queue, bufferCollection.texCoordBuffers[i], texCoord.coords, Description::VERTEX, preserve);
	}

	SpinUnLock(critical);
	Flag() |= RESOURCE_UPLOADED;
}

void MeshResource::Unmap() {
	GraphicResourceBase::Unmap();

	SpinLock(critical);
	meshCollection = IAsset::MeshCollection();
	SpinUnLock(critical);
}

void MeshResource::Detach(IRender& render, void* deviceContext) {
	Flag() &= ~RESOURCE_UPLOADED;

	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	assert(queue != nullptr);

	ClearBuffer(render, queue, bufferCollection.indexBuffer);
	ClearBuffer(render, queue, bufferCollection.positionBuffer);
	ClearBuffer(render, queue, bufferCollection.normalBuffer);
	ClearBuffer(render, queue, bufferCollection.tangentBuffer);
	ClearBuffer(render, queue, bufferCollection.colorBuffer);
	ClearBuffer(render, queue, bufferCollection.boneIndexBuffer);
	ClearBuffer(render, queue, bufferCollection.boneWeightBuffer);

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
MeshResource::BufferCollection::BufferCollection() : indexBuffer(nullptr), positionBuffer(nullptr), normalBuffer(nullptr), tangentBuffer(nullptr), colorBuffer(nullptr), boneIndexBuffer(nullptr), boneWeightBuffer(nullptr) {}

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
		ReflectProperty(normalBuffer)[Runtime][IShader::BindInput(IShader::BindInput::NORMAL)];
		ReflectProperty(tangentBuffer)[Runtime][IShader::BindInput(IShader::BindInput::TANGENT)];
		ReflectProperty(colorBuffer)[Runtime][IShader::BindInput(IShader::BindInput::COLOR)];
		ReflectProperty(boneIndexBuffer)[Runtime][IShader::BindInput(IShader::BindInput::BONE_INDEX)];
		ReflectProperty(boneWeightBuffer)[Runtime][IShader::BindInput(IShader::BindInput::BONE_WEIGHT)];
		ReflectProperty(texCoordBuffers)[Runtime][IShader::BindInput(IShader::BindInput::TEXCOORD)];
	}

	return *this;
}

void MeshResource::BufferCollection::GetDescription(std::vector<ZPassBase::Parameter>& desc, ZPassBase::Updater& updater) const {
	// Do not pass index buffer
	if (positionBuffer != nullptr) {
		desc.emplace_back(updater[IShader::BindInput::POSITION]);
	}

	if (normalBuffer != nullptr) {
		desc.emplace_back(updater[IShader::BindInput::NORMAL]);
	}

	if (tangentBuffer != nullptr) {
		desc.emplace_back(updater[IShader::BindInput::TANGENT]);
	}

	if (colorBuffer != nullptr) {
		desc.emplace_back(updater[IShader::BindInput::COLOR]);
	}

	if (boneIndexBuffer != nullptr) {
		desc.emplace_back(updater[IShader::BindInput::BONE_INDEX]);
	}

	if (boneWeightBuffer != nullptr) {
		desc.emplace_back(updater[IShader::BindInput::BONE_WEIGHT]);
	}

	for (uint32_t i = 0; i < texCoordBuffers.size(); i++) {
		desc.emplace_back(updater[IShader::BindInput::SCHEMA(IShader::BindInput::TEXCOORD + i)]);
	}
}

void MeshResource::BufferCollection::UpdateData(std::vector<IRender::Resource*>& data) const {
	if (positionBuffer != nullptr) {
		data.emplace_back(positionBuffer);
	}

	if (normalBuffer != nullptr) {
		data.emplace_back(normalBuffer);
	}

	if (tangentBuffer != nullptr) {
		data.emplace_back(tangentBuffer);
	}

	if (colorBuffer != nullptr) {
		data.emplace_back(colorBuffer);
	}

	if (boneIndexBuffer != nullptr) {
		data.emplace_back(boneIndexBuffer);
	}

	if (boneWeightBuffer != nullptr) {
		data.emplace_back(boneWeightBuffer);
	}

	for (size_t i = 0; i < texCoordBuffers.size(); i++) {
		assert(texCoordBuffers[i] != nullptr);
		data.emplace_back(texCoordBuffers[i]);
	}
}

size_t MeshResource::ReportDeviceMemoryUsage() const {
	return deviceMemoryUsage;
}
