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
	stageTemplates[#type] = Wrap(&New<type, const String&>::Invoke);

RenderFlowComponentModule::RenderFlowComponentModule(Engine& engine) : BaseClass(engine) {
	// Register built-ins
	REGISTER_TEMPLATE(AntiAliasingRenderStage);
	REGISTER_TEMPLATE(BloomRenderStage);
	REGISTER_TEMPLATE(FrameBarrierRenderStage);
	REGISTER_TEMPLATE(ForwardLightingRenderStage);
	REGISTER_TEMPLATE(DepthResolveRenderStage);
	REGISTER_TEMPLATE(DepthBoundingRenderStage);
	REGISTER_TEMPLATE(DepthBoundingSetupRenderStage);
	REGISTER_TEMPLATE(DeferredLightingRenderStage);
	REGISTER_TEMPLATE(DeviceRenderStage);
	REGISTER_TEMPLATE(GeometryBufferRenderStage);
	REGISTER_TEMPLATE(LightBufferRenderStage);
	REGISTER_TEMPLATE(PhaseLightRenderStage);
	REGISTER_TEMPLATE(ScreenRenderStage);
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

void RenderFlowComponentModule::RegisterNodeTemplate(String& key, const TWrapper<RenderStage*, const String&>& t) {
	stageTemplates[key] = t;
}

TShared<RenderFlowComponent> RenderFlowComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<RenderFlowComponent> renderFlowComponent = TShared<RenderFlowComponent>::From(allocator->New());
	renderFlowComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return renderFlowComponent;
}

TShared<RenderStage> RenderFlowComponentModule::RequestNewRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& name, const String& config) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	std::map<String, TWrapper<RenderStage*, const String&> >::const_iterator it = stageTemplates.find(name);
	if (it != stageTemplates.end()) {
		TShared<RenderStage> renderStage = TShared<RenderStage>::From(it->second(config));
		renderStage->ReflectNodePorts();
		renderFlowComponent->AddNode(renderStage());
		return renderStage;
	} else {
		request.Error(String("Could not find render stage: ") + name);
		return nullptr;
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
	for (size_t i = 0; i < s->GetPorts().size(); i++) {
		RenderStage::Port* port = s->GetPorts()[i].port;
		request << begintable <<
			key("Name") << s->GetPorts()[i].name <<
			key("Type") << port->GetUnique()->GetName() <<
			key("Targets") << beginarray;

		for (size_t j = 0; j < port->GetLinks().size(); j++) {
			RenderStage::Port* targetPort = static_cast<RenderStage::Port*>(port->GetLinks()[j].port);
			RenderStage* target = static_cast<RenderStage*>(targetPort->GetNode());
			String targetPortName;
			// locate port
			for (size_t k = 0; k < target->GetPorts().size(); k++) {
				if (target->GetPorts()[k].port == targetPort) {
					targetPortName = target->GetPorts()[k].name;
					break;
				}
			}

			request << begintable
				<< key("RenderStage") << target
				<< key("Port") << targetPortName
				<< key("Direction") << !!(port->GetLinks()[j].flag & Tiny::TINY_PINNED)
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

	if (!fromPort->GetLinks().empty() && ((fromPort->Flag() & Tiny::TINY_UNIQUE) || (toPort->Flag() & Tiny::TINY_UNIQUE))) {
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

TShared<RenderPolicy> RenderFlowComponentModule::RequestNewRenderPolicy(IScript::Request& request, const String& name, uint32_t priority) {
	CHECK_REFERENCES_NONE();

	TShared<RenderPolicy> policy = TShared<RenderPolicy>::From(new RenderPolicy());
	policy->renderPortName = name;
	policy->priority = priority;
	return policy;
}
