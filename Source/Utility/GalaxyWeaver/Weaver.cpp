#include "Weaver.h"
#include <sstream>

using namespace PaintsNow;

static const String WEAVER_VERSION = "2018.1.25";

Weaver::Weaver(BridgeSunset& sunset, SnowyStream& ns, MythForest& mf, ITunnel& tunnel, const String& entry) : bridgeSunset(sunset), snowyStream(ns), mythForest(mf), BaseClass(sunset.GetThreadApi(), tunnel, entry) {}

void Weaver::ScriptUninitialize(IScript::Request& request) {
	IScript* script = request.GetScript();
	if (script == &bridgeSunset.GetScript()) {
		request.DoLock();
		request.Dereference(rpcCallback);
		request.Dereference(connectionCallback);
		request.UnLock();
	}
	
	ProxyStub::ScriptUninitialize(request);
}

static void ReplaceCallback(IScript::Request& request, IScript::Request::Ref& target, const IScript::Request::Ref& ref) {
	if (target) {
		request.Dereference(target);
	}

	target = ref;
}

void Weaver::SetRpcCallback(IScript::Request& request, const IScript::Request::Ref& ref) {
	ReplaceCallback(request, rpcCallback, ref);
}

void Weaver::SetConnectionCallback(IScript::Request& request, const IScript::Request::Ref& ref) {
	ReplaceCallback(request, connectionCallback, ref);
}

void Weaver::OnConnectionStatus(IScript::Request& request, bool isAuto, RemoteProxy::STATUS status, const String & message) {
	if (status == RemoteProxy::CONNECTED || status == RemoteProxy::CLOSED || status == RemoteProxy::ABORTED) {
		String code = status == RemoteProxy::CONNECTED ? "Connected" : status == RemoteProxy::CLOSED ? "Closed" : "Aborted";
		if (connectionCallback) {
			request.DoLock();
			request.Push();
			request.Call(sync, connectionCallback, code, message);
			request.Pop();
			request.UnLock();
		}
	}

	if (status == RemoteProxy::CLOSED || status == RemoteProxy::ABORTED) {
		// restart if not manually stopped
		if (Flag().load(std::memory_order_acquire) & TINY_ACTIVATED) {
			remoteProxy.Reset();
		}
	}
}

void Weaver::Start() {
	if (Flag().load(std::memory_order_acquire) & TINY_ACTIVATED) {
		Stop();
	}

	remoteProxy.Run();
	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_relaxed);
}

void Weaver::Stop() {
	Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_relaxed);
	remoteProxy.Stop();
}

TObject<IReflect>& Weaver::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RpcCheckVersion)[ScriptMethod = "CheckVersion"];
		ReflectMethod(RpcComplete)[ScriptMethod = "Complete"];
		ReflectMethod(RpcPostResource)[ScriptMethod = "PostResource"];
		ReflectMethod(RpcDebugPrint)[ScriptMethod = "DebugPrint"];
		ReflectMethod(RpcPostEntity)[ScriptMethod = "PostEntity"];
		ReflectMethod(RpcPostEntityGroup)[ScriptMethod = "PostEntityGroup"];
		ReflectMethod(RpcPostEntityComponent)[ScriptMethod = "PostEntityComponent"];
		ReflectMethod(RpcPostModelComponent)[ScriptMethod = "PostModelComponent"];
		ReflectMethod(RpcPostModelComponentMaterial)[ScriptMethod = "PostModelComponentMaterial"];
		ReflectMethod(RpcPostEnvCubeComponent)[ScriptMethod = "PostEnvCubeComponent"];
		ReflectMethod(RpcPostTransformComponent)[ScriptMethod = "PostTransformComponent"];
		ReflectMethod(RpcPostSpaceComponent)[ScriptMethod = "PostSpaceComponent"];
		ReflectMethod(RpcUpdateView)[ScriptMethod = "UpdateView"];
	}

	return *this;
}

void Weaver::RpcCheckVersion(IScript::Request& request) {
	// Write current version string to output buffer.
	request.DoLock();
	request << WEAVER_VERSION;
	request.UnLock();

	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcCheckVersion"), WEAVER_VERSION));
	}
}

void Weaver::RpcPostResource(IScript::Request& request, const String& path, const String& extension, const String& resourceData) {
	bool success = true;
	// Make memory stream for deserialization
	size_t length = resourceData.length();
	MemoryStream memoryStream(length);
	memoryStream.Write(resourceData.c_str(), length);
	// resource use internal persist and needn't set environment here
	// memoryStream.SetEnvironment(snowyStream);
	assert(length == resourceData.length());

	memoryStream.Seek(IStreamBase::BEGIN, 0);
	// Create resource from memory
	TShared<ResourceBase> resource = snowyStream.CreateResource(path, extension, false, ResourceBase::RESOURCE_VIRTUAL);
	resource->Map();
	IStreamBase* filter = snowyStream.GetInterfaces().assetFilterBase.CreateFilter(memoryStream);
	SpinLock(resource->critical);
	*filter >> *resource;
	SpinUnLock(resource->critical);

	if (resource) {
		// Serialize it to disk
		success = resource->Persist();
	}

	resource->Unmap();
	filter->Destroy();

	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcPostResource"), path, extension));
	}
}

void Weaver::RpcComplete(IScript::Request& request) {
	
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcComplete")));
	}
}

void Weaver::RpcPostEntity(IScript::Request& request, uint32_t entityID, uint32_t groupID, const String& entityName) {
	/*
	TShared<Entity> entity = mythForest.CreateEntity(0); // default warp 0
	TShared<FormComponent> formComponent = TShared<FormComponent>::From(allocator->New(entityID));
	entity->AddComponent(mythForest.GetEngine(), formComponent());*/

	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcPostEntity"), entityID, groupID, entityName));
	}
}

void Weaver::RpcPostEntityGroup(IScript::Request& request, uint32_t groupID, const String& groupName) {
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcPostEntityGroup"), groupID, groupName));
	}
}

void Weaver::RpcPostEntityComponent(IScript::Request& request, uint32_t entityID, uint32_t componentID) {
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcPostEntityComponent"), entityID, componentID));
	}
}

void Weaver::RpcPostModelComponent(IScript::Request& request, uint32_t componentID, const String& resource, float viewDistance) {
	
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcPostModelComponent"), componentID, resource, viewDistance));
	}
}

void Weaver::RpcPostModelComponentMaterial(IScript::Request& request, uint32_t componentID, uint32_t meshGroupID, const String& materialResource) {
	
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcPostModelComponentMaterial"), componentID, meshGroupID, materialResource));
	}
}

void Weaver::RpcPostTransformComponent(IScript::Request& request, uint32_t componentID, const Float3& position, const Float3& scale, const Float3& rotation) {
	
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcPostTransformComponent"), componentID, position, scale, rotation));
	}
}

void Weaver::RpcPostEnvCubeComponent(IScript::Request& request, uint32_t componentID, const String& texturePath) {
	
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcPostEnvCubeComponent"), componentID, texturePath));
	}
}

void Weaver::RpcPostSpaceComponent(IScript::Request& request, uint32_t componentID, uint32_t groupID) {
	
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcPostSpaceComponent"), componentID, groupID));
	}
}

void Weaver::RpcUpdateView(IScript::Request& request,  const MatrixFloat4x4& viewMatrix, const Float4& fovNearFarAspect) {
	
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcUpdateView"), viewMatrix, fovNearFarAspect));
	}
}

void Weaver::RpcInitialize(IScript::Request& request, const String& clientVersion) {
	
}

void Weaver::RpcUninitialize(IScript::Request& request) {
	
}

void Weaver::RpcDebugPrint(IScript::Request& request, const String& text) {
	
	if (rpcCallback) {
		bridgeSunset.Dispatch(CreateTaskScript(rpcCallback, String("RpcDebugPrint"), text));
	}
}