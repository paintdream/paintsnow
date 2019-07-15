#include "DirectScriptBridge.h"

using namespace PaintsNow;
using namespace PaintsNow::NsRayForce;

DirectScriptBridge::DirectScriptBridge(IThread& thread, const TFactoryBase<IScript>& factory) : Bridge(thread), reflectMap(ZScriptReflect::GetGlobalMap()), factoryBase(factory) {
}

DirectScriptBridge::~DirectScriptBridge() {
}

IReflectObject* DirectScriptBridge::Create(IScript::Request& request, IArchive& archive, const String& path, const String& data) {
	// Not implemented
	assert(false);
	return nullptr;
}

void DirectScriptBridge::Call(IReflectObject* object, const TProxy<>* p, IScript::Request& request) {
	// Not implemented
	assert(false);
}

std::map<Unique, ZScriptReflect::Type>& DirectScriptBridge::GetReflectMap() {
	return reflectMap;
}