#include "ShaderResource.h"
#include "../../../General/Interface/IShader.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

ShaderResource::ShaderResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID), shaderResource(nullptr) {}

ShaderResource::~ShaderResource() {
	assert(shaderResource == nullptr);
}

const String& ShaderResource::GetShaderName() const {
	return GetUnique()->typeName;
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
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	ZPassBase& pass = GetPass();
	std::vector<IRender::Resource*> newResources;
	assert(newResources.empty());
	// compile default shader
	assert(shaderResource == nullptr);
	shaderResource = pass.Compile(render, queue);

	// compute hash value
	hashValue = pass.ExportHash(true);
}

IRender::Resource* ShaderResource::GetShaderResource() const {
	return shaderResource;
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

ZPassBase& ShaderResource::GetPass() {
	static ZPassBase dummy;
	assert(false);
	return dummy;
}

ZPassBase::Updater& ShaderResource::GetPassUpdater() {
	static ZPassBase dummy;
	static ZPassBase::Updater updater(dummy);
	assert(false);

	return updater;
}

TObject<IReflect>& ShaderResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

void ShaderResource::SetCode(const String& stage, const String& text, const std::vector<std::pair<String, String> >& config) {
	ICustomizeShader* customize = GetPass().QueryInterface(UniqueType<ICustomizeShader>());
	if (customize != nullptr) {
		customize->SetCode(stage, text, config);
	}
}

void ShaderResource::SetInput(const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config) {
	ICustomizeShader* customize = GetPass().QueryInterface(UniqueType<ICustomizeShader>());
	if (customize != nullptr) {
		customize->SetInput(stage, type, name, config);
	}
}

void ShaderResource::SetComplete() {
	ICustomizeShader* customize = GetPass().QueryInterface(UniqueType<ICustomizeShader>());
	if (customize != nullptr) {
		customize->SetComplete();
		Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		GetResourceManager().InvokeUpload(this);
	}
}