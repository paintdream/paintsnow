#include "RenderFlowComponentModule.h"

#include "RenderStage/AntiAliasingRenderStage.h"
#include "RenderStage/BloomRenderStage.h"
#include "RenderStage/FrameBarrierRenderStage.h"
#include "RenderStage/ForwardLightingRenderStage.h"
#include "RenderStage/DeferredLightingRenderStage.h"
#include "RenderStage/DepthBoundingRenderStage.h"
#include "RenderStage/DepthBoundingSetupRenderStage.h"
#include "RenderStage/DepthResolveRenderStage.h"
#include "RenderStage/DeviceRenderStage.h"
#include "RenderStage/GeometryBufferRenderStage.h"
#include "RenderStage/LightBufferRenderStage.h"
#include "RenderStage/PhaseLightRenderStage.h"
#include "RenderStage/ScreenRenderStage.h"
#include "RenderStage/ScreenSpaceTraceRenderStage.h"
#include "RenderStage/ShadowMaskRenderStage.h"
#include "RenderStage/WidgetRenderStage.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

#define REGISTER_TEMPLATE(type) \
	TFactory<type, RenderStage> stage##type; \
	stageTemplates[#type] = stage##type;

#define REGISTER_TEMPLATE_CONSTRUCT(type) \
	TFactoryConstruct<type, RenderStage> stage##type; \
	stageTemplates[#type] = stage##type;

RenderFlowComponentModule::RenderFlowComponentModule(Engine& engine) : BaseClass(engine) {
	// Register built-ins
	REGISTER_TEMPLATE(AntiAliasingRenderStage);
	REGISTER_TEMPLATE_CONSTRUCT(BloomRenderStage);
	REGISTER_TEMPLATE(FrameBarrierRenderStage);
	REGISTER_TEMPLATE(ForwardLightingRenderStage);
	REGISTER_TEMPLATE(DepthResolveRenderStage);
	REGISTER_TEMPLATE_CONSTRUCT(DepthBoundingRenderStage);
	REGISTER_TEMPLATE_CONSTRUCT(DepthBoundingSetupRenderStage);
	REGISTER_TEMPLATE(DeferredLightingRenderStage);
	REGISTER_TEMPLATE_CONSTRUCT(DeviceRenderStage);
	REGISTER_TEMPLATE(GeometryBufferRenderStage);
	REGISTER_TEMPLATE_CONSTRUCT(LightBufferRenderStage);
	REGISTER_TEMPLATE(PhaseLightRenderStage);
	REGISTER_TEMPLATE_CONSTRUCT(ScreenRenderStage);
	REGISTER_TEMPLATE(ScreenSpaceTraceRenderStage);
	REGISTER_TEMPLATE(ShadowMaskRenderStage);
	REGISTER_TEMPLATE(WidgetRenderStage);
}

TObject<IReflect>& RenderFlowComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestNewRenderPolicy)[ScriptMethod = "NewRenderPolicy"];
		ReflectMethod(RequestNewRenderStage)[ScriptMethod = "NewRenderStage"];
		ReflectMethod(RequestEnumerateRenderStagePorts)[ScriptMethod = "EnumerateRenderStagePorts"];
		ReflectMethod(RequestLinkRenderStagePort)[ScriptMethod = "LinkRenderStagePort"];
		ReflectMethod(RequestUnlinkRenderStagePort)[ScriptMethod = "UnlinkRenderStagePort"];
		ReflectMethod(RequestExportRenderStagePort)[ScriptMethod = "ExportRenderStagePort"];
		ReflectMethod(RequestBindRenderTargetTexture)[ScriptMethod = "BindRenderTargetTexture"];
		ReflectMethod(RequestDeleteRenderStage)[ScriptMethod = "DeleteRenderStage"];
	}

	return *this;
}

void RenderFlowComponentModule::RegisterNodeTemplate(String& key, const TFactoryBase<RenderStage>& t) {
	stageTemplates[key] = t;
}

void RenderFlowComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<RenderFlowComponent> renderFlowComponent = TShared<RenderFlowComponent>::From(allocator->New());
	renderFlowComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << renderFlowComponent;
	request.UnLock();
}

void RenderFlowComponentModule::RequestNewRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& name, const String& config) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	std::map<String, TFactoryBase<RenderStage> >::const_iterator it = stageTemplates.find(name);
	if (it != stageTemplates.end()) {
		RenderStage* renderStage = it->second(config);
		renderStage->ReflectNodePorts();
		renderFlowComponent->AddNode(renderStage);
		engine.GetKernel().YieldCurrentWarp();

		request.DoLock();
		request << renderStage;
		request.UnLock();
		renderStage->ReleaseObject();
	} else {
		request.Error(String("Could not find render stage: ") + name);
	}
}

void RenderFlowComponentModule::RequestEnumerateRenderStagePorts(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> renderStage) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(renderStage);

	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	RenderStage* s = renderStage.Get();
	request << beginarray;
	for (std::map<String, RenderStage::Port*>::const_iterator it = s->GetPortMap().begin(); it != s->GetPortMap().end(); ++it) {
		RenderStage::Port* port = it->second;
		request << begintable <<
			key("Name") << it->first <<
			key("Type") << port->GetUnique().info->typeName <<
			key("Targets") << beginarray;

		for (std::map<GraphPort<SharedTiny>*, Tiny::FLAG>::const_iterator ip = port->GetTargetPortMap().begin(); ip != port->GetTargetPortMap().end(); ++ip)	 {
			RenderStage::Port* targetPort = static_cast<RenderStage::Port*>(ip->first);
			RenderStage* target = static_cast<RenderStage*>(targetPort->GetNode());
			String targetPortName;
			// locate port
			for (std::map<String, RenderStage::Port*>::const_iterator iw = target->GetPortMap().begin(); iw != target->GetPortMap().end(); ++iw) {
				if (iw->second == targetPort) {
					targetPortName = iw->first;
					break;
				}
			}

			request << begintable
				<< key("RenderStage") << target
				<< key("Port") << targetPortName
				<< key("Direction") << !!(ip->second)
				<< endtable;
		}

		request << endarray << endtable;
	}
	
	request << endarray;
	request.UnLock();
}

void RenderFlowComponentModule::RequestLinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(from);
	CHECK_DELEGATE(to);

	RenderStage::Port* fromPort = (*from.Get())[fromPortName];
	RenderStage::Port* toPort = (*to.Get())[toPortName];

	if (fromPort == nullptr) {
		request.Error(String("Unable to locate port: ") + fromPortName);
		return;
	}

	if (toPort == nullptr) {
		request.Error(String("Unable to locate port: ") + toPortName);
		return;
	}

	if (!fromPort->GetTargetPortMap().empty() && ((fromPort->Flag() & Tiny::TINY_UNIQUE) || (toPort->Flag() & Tiny::TINY_UNIQUE))) {
		request.Error(String("Sharing policy conflicts when connecting from: ") + fromPortName + " to " + toPortName);
		return;
	}

	fromPort->Link(toPort, toPort->Flag() & RenderStage::RENDERSTAGE_WEAK_LINKAGE ? 0 : Tiny::TINY_PINNED);
}

void RenderFlowComponentModule::RequestUnlinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(from);
	CHECK_DELEGATE(to);

	RenderStage::Port* fromPort = (*from.Get())[fromPortName];
	RenderStage::Port* toPort = (*to.Get())[toPortName];

	if (fromPort == nullptr) {
		request.Error(String("Unable to locate port: ") + fromPortName);
		return;
	}

	if (toPort == nullptr) {
		request.Error(String("Unable to locate port: ") + toPortName);
		return;
	}
	
	fromPort->UnLink(toPort);
}

void RenderFlowComponentModule::RequestExportRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> stage, const String& port, const String& symbol) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(stage);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	if (!renderFlowComponent->ExportSymbol(symbol, stage.Get(), port)) {
		request.Error(String("Unable to export port: ") + port);
	}
}

void RenderFlowComponentModule::RequestBindRenderTargetTexture(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& symbol, IScript::Delegate<TextureResource> renderTargetResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	// TODO:
}

void RenderFlowComponentModule::RequestDeleteRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> stage) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(stage);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	renderFlowComponent->RemoveNode(stage.Get());
}

void RenderFlowComponentModule::RequestNewRenderPolicy(IScript::Request& request, const String& name, uint32_t priority) {
	CHECK_REFERENCES_NONE();

	TShared<RenderPolicy> policy = TShared<RenderPolicy>::From(new RenderPolicy());
	policy->renderPortName = name;
	policy->priority = priority;

	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << policy;
	request.UnLock();
}
