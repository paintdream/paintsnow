#include "ShaderResource.h"
#include "../ResourceManager.h"
#include "../../../General/Interface/IShader.h"

using namespace PaintsNow;

ShaderResource::ShaderResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), shaderResource(nullptr) {}

ShaderResource::~ShaderResource() {
	assert(shaderResource == nullptr);
}

const String& ShaderResource::GetShaderName() const {
	return GetUnique()->GetName();
}

bool ShaderResource::operator << (IStreamBase& stream) {
	return false;
}

bool ShaderResource::operator >> (IStreamBase& stream) const {
	return false;
}

const Bytes& ShaderResource::GetHashValue() const {
	return hashValue;
}

void ShaderResource::Attach(IRender& render, void* deviceContext) {
	// compute hash value
	PassBase& pass = GetPass();
	hashValue = pass.ExportHash(true);

	if (shaderResource != nullptr) return; // already attached.

	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	// compile default shader
	shaderResource = pass.Compile(render, queue);

#ifdef _DEBUG
	render.SetResourceNotation(shaderResource, GetLocation());
#endif
}

IRender::Resource* ShaderResource::GetShaderResource() const {
	return shaderResource;
}

void ShaderResource::SetShaderResource(IRender::Resource* res) {
	shaderResource = res;
}

void ShaderResource::Detach(IRender& render, void* deviceContext) {
	if (shaderResource != nullptr) {
		IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
		render.DeleteResource(queue, shaderResource);
		shaderResource = nullptr;
	}
}

IReflectObject* ShaderResource::Clone() const {
	// By default, new created one are not compiled
	ShaderResource* clone = new ShaderResource(resourceManager, uniqueLocation);
	clone->shaderResource = nullptr;
	clone->Flag().fetch_or(RESOURCE_ORPHAN, std::memory_order_acquire);
	return clone;
}

void ShaderResource::Upload(IRender& render, void* deviceContext) {

}

void ShaderResource::Download(IRender& render, void* deviceContext) {

}

const String& ShaderResource::GetShaderPathPrefix() {
	static const String shaderPrefix = "[Runtime]/ShaderResource/";
	return shaderPrefix;
}

PassBase& ShaderResource::GetPass() {
	static PassBase dummy;
	assert(false);
	return dummy;
}

PassBase::Updater& ShaderResource::GetPassUpdater() {
	static PassBase dummy;
	static PassBase::Updater updater(dummy);
	assert(false);

	return updater;
}

TObject<IReflect>& ShaderResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}
