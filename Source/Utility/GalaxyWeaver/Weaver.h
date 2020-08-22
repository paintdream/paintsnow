// Weaver.h
// By PaintDream
// 2016-3-22
//

#pragma once
#include "Controller.h"
#include "../BridgeSunset/BridgeSunset.h"
#include "../SnowyStream/SnowyStream.h"
#include "../MythForest/MythForest.h"

namespace PaintsNow {
	class Weaver : public TReflected<Weaver, ProxyStub> {
	public:
		Weaver(BridgeSunset& bridgeSunset, SnowyStream& snowyStream, MythForest& mythForest, ITunnel& tunnel, const String& entry);
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual void ScriptUninitialize(IScript::Request& request) override;
		void SetRpcCallback(IScript::Request& request, const IScript::Request::Ref& ref);
		void SetConnectionCallback(IScript::Request& request, const IScript::Request::Ref& ref);

	public:
		void OnConnectionStatus(IScript::Request& request, bool state, RemoteProxy::STATUS status, const String& message);
		// Local controls
		void Start();
		void Stop();

	protected:
		BridgeSunset& bridgeSunset;
		SnowyStream& snowyStream;
		MythForest& mythForest;
		IScript::Request::Ref rpcCallback;
		IScript::Request::Ref connectionCallback;

	public:
		// Remote routines
		void RpcCheckVersion(IScript::Request& request);
		void RpcInitialize(IScript::Request& request, const String& clientVersion);
		void RpcUninitialize(IScript::Request& request);

		void RpcDebugPrint(IScript::Request& request, const String& text);
		void RpcPostResource(IScript::Request& request, const String& location, const String& extension, const String& resourceData);
		void RpcPostEntity(IScript::Request& request, uint32_t entityID, uint32_t groupID, const String& entityName);
		void RpcPostEntityGroup(IScript::Request& request, uint32_t groupID, const String& groupName);
		void RpcPostEntityComponent(IScript::Request& request, uint32_t entityID, uint32_t componentID);
		void RpcPostModelComponent(IScript::Request& request, uint32_t componentID, const String& meshResource, float viewDistance);
		void RpcPostModelComponentMaterial(IScript::Request& request, uint32_t componentID, uint32_t meshGroupID, const String& materialResource);
		void RpcPostTransformComponent(IScript::Request& request, uint32_t componentID, const Float3& position, const Float3& scale, const Float3& rotation);
		void RpcPostSpaceComponent(IScript::Request& request, uint32_t componentID, uint32_t groupID);
		void RpcPostEnvCubeComponent(IScript::Request& request, uint32_t componentID, const String& texturePath);
		void RpcUpdateView(IScript::Request& request, const MatrixFloat4x4& viewMatrix, const Float4& fovNearFarAspect);
		void RpcComplete(IScript::Request& request);
	};
}

