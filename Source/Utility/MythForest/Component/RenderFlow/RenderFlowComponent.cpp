#include "RenderFlowComponent.h"
#include "RenderStage.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include "RenderPort/RenderPortRenderTarget.h"
#include "RenderStage/FrameBarrierRenderStage.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <vector>

using namespace PaintsNow;

RenderFlowComponent::RenderFlowComponent() : mainResolution(640, 480), mainQueue(nullptr), eventResourcePrepared(nullptr) {
	Flag().fetch_or(RENDERFLOWCOMPONENT_SYNC_DEVICE_RESOLUTION, std::memory_order_relaxed);
}

RenderFlowComponent::~RenderFlowComponent() {}

TObject<IReflect>& RenderFlowComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

Tiny::FLAG RenderFlowComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_SPECIAL_EVENT;
}

void RenderFlowComponent::AddNode(RenderStage* stage) {
	assert(!(Flag().load(std::memory_order_acquire) & TINY_ACTIVATED));
	Graph<RenderStage>::AddNode(stage);
}

UShort2 RenderFlowComponent::GetMainResolution() const {
	return mainResolution;
}

void RenderFlowComponent::SetMainResolution(const UShort2 res) {
	mainResolution = res;
	Flag().fetch_or(RENDERFLOWCOMPONENT_RESOLUTION_MODIFIED, std::memory_order_relaxed);
}

void RenderFlowComponent::RemoveNode(RenderStage* stage) {
	assert(!(Flag().load(std::memory_order_acquire) & TINY_ACTIVATED));
	// removes all symbols related with stage ...
	for (size_t i = 0; i < stage->GetPorts().size(); i++) {
		RenderStage::Port* port = stage->GetPorts()[i].port;
		for (std::vector<String>::const_iterator it = port->publicSymbols.begin(); it != port->publicSymbols.end(); ++it) {
			symbolMap.erase(*it);
		}

		port->publicSymbols.clear();
	}

	Graph<RenderStage>::RemoveNode(stage);
}

RenderStage::Port* RenderFlowComponent::BeginPort(const String& symbol) {
	std::map<String, std::pair<RenderStage*, String> >::const_iterator s = symbolMap.find(symbol);
	if (s != symbolMap.end()) {
		RenderStage::Port* port = (*s->second.first)[s->second.second];
		assert(!(port->Flag().load(std::memory_order_acquire) & TINY_ACTIVATED)); // no shared
		port->Flag().fetch_or(TINY_ACTIVATED, std::memory_order_relaxed);
		return port;
	} else {
		return nullptr;
	}
}

void RenderFlowComponent::EndPort(RenderStage::Port* port) {
	assert((port->Flag().load(std::memory_order_acquire) & TINY_ACTIVATED));
	port->Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_relaxed);
}

bool RenderFlowComponent::ExportSymbol(const String& symbol, RenderStage* renderStage, const String& port) {
	RenderStage::Port* p = (*renderStage)[port];
	if (p != nullptr) {
		std::map<String, std::pair<RenderStage*, String> >::const_iterator s = symbolMap.find(symbol);

		if (s != symbolMap.end()) {
			// remove old entry
			RenderStage::Port* port = (*s->second.first)[s->second.second];

			if (port != nullptr) {
				std::binary_erase(port->publicSymbols, symbol);
			}
		}

		std::binary_insert(p->publicSymbols, port);
		symbolMap[symbol] = std::make_pair(renderStage, port);
		return true;
	} else {
		return false;
	}
}

struct CallBatch {
public:
	CallBatch(std::vector<RenderStage*>& r) : result(r) {}
	// once item
	bool operator () (RenderStage* stage) {
		if (stage->Flag().load(std::memory_order_relaxed) & RenderStage::RENDERSTAGE_ENABLED) {
			result.emplace_back(stage);
		} else {
			fprintf(stderr, "RenderStage: %s (%p) discarded by render flow optimizer.", stage->GetUnique()->GetBriefName().c_str(), stage);
		}

		return true;
	}

	// once batch
	bool operator () () {
		if (result.empty() || result.back() != nullptr) {
			result.emplace_back(nullptr);
		}

		return true;
	}

	std::vector<RenderStage*>& result;
};

void RenderFlowComponent::MarkupRenderStages() {
	std::queue<RenderStage*> renderStages;
	for (std::map<String, std::pair<RenderStage*, String> >::const_iterator it = symbolMap.begin(); it != symbolMap.end(); ++it) {
		RenderStage* renderStage = (*it).second.first;
		renderStage->Flag().fetch_or(RenderStage::RENDERSTAGE_ENABLED, std::memory_order_relaxed);
		renderStages.push(renderStage);
	}

	while (!renderStages.empty()) {
		RenderStage* renderStage = renderStages.front();
		renderStages.pop();

		assert(renderStage->Flag().load(std::memory_order_acquire) & RenderStage::RENDERSTAGE_ENABLED);

		const std::vector<RenderStage::PortInfo>& ports = renderStage->GetPorts();
		for (size_t i = 0; i < ports.size(); i++) {
			const std::vector<RenderPort::LinkInfo>& links = ports[i].port->GetLinks();
			for (size_t j = 0; j < links.size(); j++) {
				const RenderPort::LinkInfo& link = links[j];
				if (!(link.flag & Tiny::TINY_PINNED)) {
					RenderStage* target = static_cast<RenderStage*>(link.port->GetNode());
					if (!(target->Flag().fetch_or(RenderStage::RENDERSTAGE_ENABLED, std::memory_order_relaxed) & RenderStage::RENDERSTAGE_ENABLED)) {
						renderStages.push(target);
					}
				}
			}
		}
	}
}

void RenderFlowComponent::Compile() {
	OPTICK_EVENT();
	assert(!(Flag().load(std::memory_order_acquire) & TINY_ACTIVATED)); // aware of thread unsafe

	MarkupRenderStages();

	std::vector<RenderStage*> result;
	CallBatch batch(result);
	IterateTopological(batch, batch);
	result.emplace_back(nullptr); // sentinel for optimization

	// set barrier stages
	uint16_t index = 0;
	for (size_t n = 0; n < result.size(); n++) {
		RenderStage* renderStage = result[n];
		if (renderStage == nullptr) {
			index++;
		} else {
			renderStage->SetFrameBarrierIndex(index);
		}
	}

	std::swap(result, cachedRenderStages);
}

// Notice that this function is called in render device thread
void RenderFlowComponent::Render(Engine& engine) {
	OPTICK_EVENT();
	if (Flag().load(std::memory_order_acquire) & TINY_ACTIVATED) {
		Flag().fetch_or(RENDERFLOWCOMPONENT_RENDERING, std::memory_order_acquire);

		// Commit resource queue first
		IRender& render = engine.interfaces.render;
		render.SubmitQueues(&mainQueue, 1, IRender::SUBMIT_EXECUTE_ALL);
		assert(cachedRenderStages[cachedRenderStages.size() - 1] == nullptr);
		std::vector<IRender::Queue*> deletedQueues;

		IRender::Resource::EventDescription desc;
		desc.setState = true;
		render.RequestDownloadResource(mainQueue, eventResourcePrepared, &desc);
		if (desc.newState && engine.snowyStream.GetRenderResourceManager()->GetCompleted()) {
			Flag().fetch_or(RENDERFLOWCOMPONENT_RESOURCE_PREPARED, std::memory_order_release);
		}

		if (Flag().load(std::memory_order_acquire) & RENDERFLOWCOMPONENT_RESOURCE_PREPARED) {
			for (size_t n = 0, m = 0; n < cachedRenderStages.size(); n++) {
				if (cachedRenderStages[n] == nullptr) {
					std::vector<IRender::Queue*> repeatQueues;
					std::vector<IRender::Queue*> instantQueues;
					for (size_t i = m; i < n; i++) {
						cachedRenderStages[i]->Commit(engine, repeatQueues, instantQueues, deletedQueues, mainQueue);
					}

					render.SubmitQueues(&mainQueue, 1, IRender::SUBMIT_EXECUTE_ALL);

					if (!instantQueues.empty()) {
						render.SubmitQueues(&instantQueues[0], verify_cast<uint32_t>(instantQueues.size()), IRender::SUBMIT_EXECUTE_ALL);
					}

					if (!repeatQueues.empty()) {
						render.SubmitQueues(&repeatQueues[0], verify_cast<uint32_t>(repeatQueues.size()), IRender::SUBMIT_EXECUTE_REPEAT);
					}

					m = n + 1;
				}
			}

			for (size_t k = 0; k < deletedQueues.size(); k++) {
				render.DeleteQueue(deletedQueues[k]);
			}

			deletedQueues.clear();
		}

		Flag().fetch_and(~RENDERFLOWCOMPONENT_RENDERING, std::memory_order_release);
	}
}

struct TextureKey {
	inline bool operator < (const TextureKey& rhs) const {
		int c = memcmp(&state, &rhs.state, sizeof(state));
		return c > 0 ? false : c < 0 ? true : dimension < rhs.dimension;
	}

	IRender::Resource::TextureDescription::State state;
	UShort3 dimension;
};

void RenderFlowComponent::ResolveSamplessAttachments() {
	OPTICK_EVENT();
	std::map<RenderPortRenderTargetStore*, RenderPortRenderTargetStore*> targetUnions;

	for (size_t i = 0; i < cachedRenderStages.size(); i++) {
		RenderStage* renderStage = cachedRenderStages[i];

		if (renderStage != nullptr) {
			const std::vector<RenderStage::PortInfo>& portInfos = renderStage->GetPorts();
			std::vector<size_t*> stageHolders;

			for (size_t k = 0; k < portInfos.size(); k++) {
				const RenderStage::PortInfo& portInfo = portInfos[k];
				RenderPortRenderTargetStore* rt = portInfo.port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
				if (rt != nullptr) {
					RenderPortRenderTargetLoad* loader = rt->QueryLoad();
					if (loader == nullptr || loader->GetLinks().empty()) {
						targetUnions[rt] = rt;
						// set sampless as default.
						rt->renderTargetDescription.state.media = IRender::Resource::TextureDescription::RENDERBUFFER;
					} else {
						RenderPortRenderTargetStore* parent = loader->GetLinks().back().port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
						assert(parent != nullptr);
						targetUnions[rt] = targetUnions[parent];
					}

					// check sampless
					for (size_t n = 0; n < rt->GetLinks().size(); n++) {
						loader = rt->GetLinks()[n].port->QueryInterface(UniqueType<RenderPortRenderTargetLoad>());
						if (loader == nullptr || loader->GetLinks().empty()) {
							targetUnions[rt]->renderTargetDescription.state.media = IRender::Resource::TextureDescription::TEXTURE_RESOURCE;
							break;
						}
					}
				}
			}
		}
	}
}

void RenderFlowComponent::SetupTextures(Engine& engine) {
	OPTICK_EVENT();
	// SetupTextures render stage texture storages
	// Scan for sampless attachments

	// Tint by order
	typedef std::vector<TShared<TextureResource> > TextureList;
	typedef std::map<TextureKey, TextureList> TextureMap;
	typedef std::map<TextureResource*, size_t> TextureRefMap;

	TextureMap textureMap;
	TextureRefMap textureRefMap;

	for (size_t i = 0; i < cachedRenderStages.size(); i++) {
		RenderStage* renderStage = cachedRenderStages[i];

		if (renderStage != nullptr) {
			const std::vector<RenderStage::PortInfo>& portInfos = renderStage->GetPorts();
			std::vector<size_t*> stageHolders;

			for (size_t k = 0; k < portInfos.size(); k++) {
				const RenderStage::PortInfo& portInfo = portInfos[k];
				RenderPortRenderTargetStore* rt = portInfo.port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());

				if (rt != nullptr) {
					RenderPortRenderTargetLoad* loader = rt->QueryLoad();
					if (loader == nullptr || loader->GetLinks().empty()) {
						// trying to allocate from cache
						TextureKey textureKey;
						textureKey.state = rt->renderTargetDescription.state;
						assert(textureKey.state.attachment);
						textureKey.dimension = rt->renderTargetDescription.dimension;
						TextureMap::iterator it = textureMap.find(textureKey);
						TShared<TextureResource> texture;

						if (it != textureMap.end()) {
							for (size_t n = 0; n < (*it).second.size(); n++) {
								TShared<TextureResource>& res = (*it).second[n];
								size_t& v = textureRefMap[res()];
								if (v == 0) { // reuseable
									texture = res;
									v = rt->GetLinks().size() + 1;
									stageHolders.emplace_back(&v);
									break;
								}
							}
						}

						if (!texture) {
							texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("Transient", rt), false, ResourceBase::RESOURCE_VIRTUAL);
							texture->description.state = textureKey.state;
							texture->description.dimension = textureKey.dimension;
							texture->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
							texture->GetResourceManager().InvokeUpload(texture(), mainQueue);

							TextureList& textureList = textureMap[textureKey];
							textureList.emplace_back(texture);

							textureRefMap[texture()] = rt->GetLinks().size() + 1;
						}

						rt->attachedTexture = texture;
						texture->description.frameBarrierIndex = Math::Max(texture->description.frameBarrierIndex, renderStage->frameBarrierIndex);
					}
				}
			}

			for (size_t n = 0; n < stageHolders.size(); n++) {
				(*stageHolders[n])--;
			}

			for (size_t t = 0; t < portInfos.size(); t++) {
				const RenderStage::PortInfo& portInfo = portInfos[t];
				const std::vector<RenderPort::LinkInfo>& links = portInfo.port->GetLinks();
				for (size_t j = 0; j < links.size(); j++) {
					RenderPortRenderTargetStore* rt = links[j].port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
					if (rt != nullptr && rt->attachedTexture) {
						textureRefMap[rt->attachedTexture()]--;
					}
				}
			}
		}
	}
}

void RenderFlowComponent::Initialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	Compile();

	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	IRender& render = engine.interfaces.render;
	assert(mainQueue == nullptr);
	mainQueue = render.CreateQueue(device);
	assert(eventResourcePrepared == nullptr);
	eventResourcePrepared = render.CreateResource(device, IRender::Resource::RESOURCE_EVENT);
	IThread& thread = engine.interfaces.thread;
	frameSyncLock = thread.NewLock();
	frameSyncEvent = thread.NewEvent();

	for (size_t n = 0; n < cachedRenderStages.size(); n++) {
		RenderStage* renderStage = cachedRenderStages[n];
		if (renderStage != nullptr) {
			renderStage->Prepare(engine, mainQueue);
		}
	}

	SetMainResolution(engine);
	ResolveSamplessAttachments();
	SetupTextures(engine);

	for (size_t i = 0; i < cachedRenderStages.size(); i++) {
		RenderStage* renderStage = cachedRenderStages[i];
		if (renderStage != nullptr) {
			renderStage->Initialize(engine, mainQueue);
		}
	}

	for (size_t j = 0; j < cachedRenderStages.size(); j++) {
		RenderStage* renderStage = cachedRenderStages[j];
		if (renderStage != nullptr) {
			renderStage->Tick(engine, mainQueue);
		}
	}

	render.ExecuteResource(mainQueue, eventResourcePrepared);
	BaseClass::Initialize(engine, entity);
}

void RenderFlowComponent::Uninitialize(Engine& engine, Entity* entity) {
	// Wait for rendering finished.
	IThread& thread = engine.interfaces.thread;

	Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_release);
	while (Flag().load(std::memory_order_acquire) & RENDERFLOWCOMPONENT_RENDERING) {
		thread.Sleep(10);
	}

	for (size_t i = 0; i < allNodes.size(); i++) {
		allNodes[i]->Uninitialize(engine, mainQueue);
	}

	IRender& render = engine.interfaces.render;

	render.DeleteResource(mainQueue, eventResourcePrepared);
	eventResourcePrepared = nullptr;

	render.DeleteQueue(mainQueue);
	mainQueue = nullptr;

	thread.DeleteEvent(frameSyncEvent);
	frameSyncEvent = nullptr;
	thread.DeleteLock(frameSyncLock);
	frameSyncLock = nullptr;

	// Deactivate this
	BaseClass::Uninitialize(engine, entity);
}

void RenderFlowComponent::SetMainResolution(Engine& engine) {
	bool updateResolution = false;
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	if (Flag().load(std::memory_order_relaxed) & RENDERFLOWCOMPONENT_SYNC_DEVICE_RESOLUTION) {
		Int2 size = engine.interfaces.render.GetDeviceResolution(device);
		if (size.x() == 0 || size.y() == 0) return;

		if (mainResolution.x() != size.x() || mainResolution.y() != size.y()) {
			mainResolution = UShort2(verify_cast<uint16_t>(size.x()), verify_cast<uint16_t>(size.y()));
			updateResolution = true;
		}
	} else if (!!(Flag().load(std::memory_order_relaxed) & RENDERFLOWCOMPONENT_RESOLUTION_MODIFIED)) {
		updateResolution = true;
	}

	if (updateResolution) {
		for (size_t i = 0; i < cachedRenderStages.size(); i++) {
			RenderStage* renderStage = cachedRenderStages[i];
			if (renderStage != nullptr) {
				renderStage->SetMainResolution(engine, mainQueue, mainResolution);
			}
		}

		Flag().fetch_and(~RENDERFLOWCOMPONENT_RESOLUTION_MODIFIED, std::memory_order_release);
	}
}

void RenderFlowComponent::RenderSyncTick(Engine& engine) {
	OPTICK_EVENT();
	// check if all resources prepared.

	const Tiny::FLAG condition = TINY_ACTIVATED | RENDERFLOWCOMPONENT_RESOURCE_PREPARED;
	if ((Flag().load(std::memory_order_acquire) & condition) == condition) {
		IRender& render = engine.interfaces.render;
		SetMainResolution(engine);

		IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
		// Cleanup modified flag for all ports
		for (size_t k = 0; k < cachedRenderStages.size(); k++) {
			RenderStage* stage = cachedRenderStages[k];
			if (stage != nullptr) {
				for (size_t j = 0; j < stage->GetPorts().size(); j++) {
					stage->GetPorts()[j].port->Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
				}
			}
		}

		for (size_t i = 0; i < cachedRenderStages.size(); i++) {
			RenderStage* stage = cachedRenderStages[i];
			if (stage != nullptr) {
				stage->Tick(engine, mainQueue);
			}
		}
	}

	Flag().fetch_and(~RENDERFLOWCOMPONENT_RENDER_SYNC_TICKING, std::memory_order_release);

	IThread& thread = engine.interfaces.thread;
	thread.Signal(frameSyncEvent);
}

void RenderFlowComponent::DispatchEvent(Event& event, Entity* entity) {
	OPTICK_EVENT();
	if (event.eventID == Event::EVENT_FRAME) {
		if (Flag().load(std::memory_order_acquire) & TINY_ACTIVATED) {
			Engine& engine = event.engine;
			const Tiny::FLAG condition = RENDERFLOWCOMPONENT_RENDER_SYNC_TICKING | TINY_ACTIVATED;
			IThread& thread = engine.interfaces.thread;
			OPTICK_PUSH("Wait for sync ticking");
			if ((Flag().load(std::memory_order_acquire) & condition) == condition) {
				thread.DoLock(frameSyncLock);
				while ((Flag().load(std::memory_order_acquire) & condition) == condition) {
					thread.Wait(frameSyncEvent, frameSyncLock, 10);
				}
				thread.UnLock(frameSyncLock);
			}
			OPTICK_POP();

			OPTICK_PUSH("Render");
			Render(event.engine);
			OPTICK_POP();

			Flag().fetch_or(RENDERFLOWCOMPONENT_RENDER_SYNC_TICKING, std::memory_order_release);
			engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &RenderFlowComponent::RenderSyncTick), std::ref(engine)));
		}
	} else if (event.eventID == Event::EVENT_TICK) {
		// No operations on trivial tick
	}
}
