#include "RenderFlowComponent.h"
#include "RenderStage.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "RenderPort/RenderPortRenderTarget.h"
#include "RenderStage/FrameBarrierRenderStage.h"
#include <vector>

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

RenderFlowComponent::RenderFlowComponent() : resourceQueue(nullptr) {
	Flag() |= RENDERFLOWCOMPONENT_SYNC_DEVICE_RESOLUTION;
}

RenderFlowComponent::~RenderFlowComponent() {}

TObject<IReflect>& RenderFlowComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

void RenderFlowComponent::AddNode(RenderStage* stage) {
	assert(!(Flag() & TINY_ACTIVATED));
	Graph<RenderStage>::AddNode(stage);
}

const Int2& RenderFlowComponent::GetMainResolution() const {
	return mainResolution;
}

void RenderFlowComponent::SetMainResolution(const Int2& res) {
	mainResolution = res;
	Flag() |= RENDERFLOWCOMPONENT_RESOLUTION_MODIFIED;
}

void RenderFlowComponent::RemoveNode(RenderStage* stage) {
	assert(!(Flag() & TINY_ACTIVATED));
	// removes all symbols related with stage ...
	for (std::map<String, RenderStage::Port*>::const_iterator it = stage->GetPortMap().begin(); it != stage->GetPortMap().end(); ++it) {
		RenderStage::Port* port = it->second;
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
		assert(!(port->Flag() & TINY_ACTIVATED)); // no shared
		port->Flag() |= TINY_ACTIVATED;
		return port;
	} else {
		return nullptr;
	}
}

void RenderFlowComponent::EndPort(RenderStage::Port* port) {
	assert((port->Flag() & TINY_ACTIVATED));
	port->Flag() &= ~TINY_ACTIVATED;
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
		result.emplace_back(stage);
		return true;
	}

	// once batch
	bool operator () () {
		result.emplace_back(nullptr);
		return true;
	}

	std::vector<RenderStage*>& result;
};

void RenderFlowComponent::Compile() {
	assert(!(Flag() & TINY_ACTIVATED)); // aware of thread unsafe
	std::vector<RenderStage*> result;
	CallBatch batch(result);
	IterateTopological(batch, batch);
	result.emplace_back(nullptr); // sentinel for optimization

	std::swap(result, cachedRenderStages);
}

IRender::Queue* RenderFlowComponent::GetResourceQueue() const {
	return resourceQueue;
}

// Notice that this function is called in render device thread
void RenderFlowComponent::Render(Engine& engine) {
	if (Flag() & TINY_ACTIVATED) {
		// Commit resource queue first
		IRender& render = engine.interfaces.render;
		render.PresentQueues(&resourceQueue, 1, IRender::CONSUME);
		assert(cachedRenderStages[cachedRenderStages.size() - 1] == nullptr);
		for (size_t n = 0, m = 0; n < cachedRenderStages.size(); n++) {
			if (cachedRenderStages[n] == nullptr) {
				std::vector<ZRenderQueue*> renderQueues;
				for (size_t i = m; i < n; i++) {
					cachedRenderStages[i]->PrepareRenderQueues(engine, renderQueues);
				}

				ZRenderQueue::InvokeRenderQueuesParallel(render, renderQueues);
				m = n + 1;
			}
		}
	}
}

struct TextureKey {
	TextureKey(TShared<TextureResource>& res) {
		state = res->description.state;
		dimension = res->description.dimension;
	}

	inline bool operator < (const TextureKey& rhs) const {
		return memcmp(this, &rhs, sizeof(*this)) < 0;
	}

	IRender::Resource::TextureDescription::State state;
	UShort3 dimension;
};

class RenderTargetTextureCombiner : public IReflect {
public:
	RenderTargetTextureCombiner() {}

	virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
		static Unique unique = UniqueType<RenderPortRenderTarget>::Get();
		if (typeID == unique) {
			RenderPortRenderTarget& rt = static_cast<RenderPortRenderTarget&>(s);
			TShared<TextureResource>& texture = rt.renderTargetTextureResource;
			if (texture) {
				std::vector<RenderPortRenderTarget*>& s = reusableTextures[texture];
				for (size_t k = 0; k < s.size(); k++) {
					RenderPortRenderTarget*& target = s[k];
					const RenderPortRenderTarget::PortMap& portMap = target->GetTargetPortMap();
					RenderPortRenderTarget::PortMap::const_iterator it;
					for (it = portMap.begin(); it != portMap.end(); ++it) {
						RenderStage* renderStage = static_cast<RenderStage*>(it->first->GetNode());
						if (unlockedRenderStages.count(renderStage) == 0) {
							// give up
							break;
						}
					}

					if (it == portMap.end()) {
						texture = target->renderTargetTextureResource;
						target = &rt; // update reusable info
						return;
					}
				}

				s.emplace_back(&rt);
			}
		}
	}

	virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

	void UnlockRenderStage(RenderStage* renderStage) {
		unlockedRenderStages.insert(renderStage);
	}

private:
	std::map<TextureKey, std::vector<RenderPortRenderTarget*> > reusableTextures;
	std::set<RenderStage*> unlockedRenderStages;
};

void RenderFlowComponent::Optimize(bool enableParallelPresent) {
	// Optimize render stage texture storages
	// Tint by order
	
	RenderTargetTextureCombiner combine;
	for (size_t i = 0; i < cachedRenderStages.size(); i++) {
		RenderStage* renderStage = cachedRenderStages[i];

		if (renderStage != nullptr) {
			(*renderStage)(combine);

			if (!enableParallelPresent) {
				if (renderStage->QueryInterface(UniqueType<FrameBarrierRenderStage>()) == nullptr) {
					combine.UnlockRenderStage(renderStage);
				}
			}
		} else if (enableParallelPresent) {
			for (size_t k = i; k != 0; k--) {
				RenderStage* renderStage = cachedRenderStages[k - 1];
				if (renderStage == nullptr) break;

				combine.UnlockRenderStage(renderStage);
			}
		}
	}
}

void RenderFlowComponent::Initialize(Engine& engine, Entity* entity) {
	Compile();

	IRender::Device* device = engine.snowyStream.GetRenderDevice();
	IRender& render = engine.interfaces.render;
	resourceQueue = render.CreateQueue(device);

	for (size_t n = 0; n < cachedRenderStages.size(); n++) {
		RenderStage* renderStage = cachedRenderStages[n];
		if (renderStage != nullptr) {
			renderStage->PrepareResources(engine);
		}
	}

	SetMainResolution(engine, true);
	Optimize(engine.interfaces.render.SupportParallelPresent(engine.snowyStream.GetRenderDevice()));

	for (size_t i = 0; i < cachedRenderStages.size(); i++) {
		RenderStage* renderStage = cachedRenderStages[i];
		if (renderStage != nullptr) {
			renderStage->Initialize(engine);
		}
	}

	for (size_t j = 0; j < cachedRenderStages.size(); j++) {
		RenderStage* renderStage = cachedRenderStages[j];
		if (renderStage != nullptr) {
			renderStage->Tick(engine);
		}
	}

	BaseClass::Initialize(engine, entity);
}

void RenderFlowComponent::Uninitialize(Engine& engine, Entity* entity) {
	for (std::set<Node*>::iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
		(*it)->Uninitialize(engine);
	}

	IRender& render = engine.interfaces.render;
	render.DeleteQueue(resourceQueue);

	// Deactivate this
	Flag() &= ~TINY_ACTIVATED;
	BaseClass::Uninitialize(engine, entity);
}

void RenderFlowComponent::SetMainResolution(Engine& engine, bool sizeOnly) {
	bool updateResolution = false;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();
	if (Flag() & RENDERFLOWCOMPONENT_SYNC_DEVICE_RESOLUTION) {
		Int2 size = engine.interfaces.render.GetDeviceResolution(device);
		if (size.x() == 0 || size.y() == 0) return;

		if (mainResolution.x() != size.x() || mainResolution.y() != size.y()) {
			mainResolution = size;
			updateResolution = true;
		}
	} else if (!!(Flag() & RENDERFLOWCOMPONENT_RESOLUTION_MODIFIED)) {
		updateResolution = true;
	}

	uint32_t width = mainResolution.x(), height = mainResolution.y();
	if (updateResolution) {
		for (size_t i = 0; i < cachedRenderStages.size(); i++) {
			RenderStage* renderStage = cachedRenderStages[i];
			if (renderStage != nullptr) {
				renderStage->SetMainResolution(engine, resourceQueue, width, height, sizeOnly);
			}
		}

		Flag() &= ~RENDERFLOWCOMPONENT_RESOLUTION_MODIFIED;
	}
}

void RenderFlowComponent::RenderSyncTick(Engine& engine) {
	if (Flag() & TINY_ACTIVATED) {
		SetMainResolution(engine, false);

		IRender::Device* device = engine.snowyStream.GetRenderDevice();
		// Cleanup modified flag for all ports
		for (size_t k = 0; k < cachedRenderStages.size(); k++) {
			RenderStage* stage = cachedRenderStages[k];
			if (stage != nullptr) {
				for (std::map<String, RenderPort*>::const_iterator it = stage->GetPortMap().begin(); it != stage->GetPortMap().end(); ++it) {
					it->second->Flag() &= ~TINY_MODIFIED;
				}
			}
		}

		for (size_t i = 0; i < cachedRenderStages.size(); i++) {
			RenderStage* stage = cachedRenderStages[i];
			if (stage != nullptr) {
				stage->Tick(engine);
			}
		}
	}

	Flag() &= ~RENDERFLOWCOMPONENT_RENDER_SYNC_TICKING;
}

void RenderFlowComponent::DispatchEvent(Event& event, Entity* entity) {
	if (event.eventID == Event::EVENT_FRAME) {
		Engine& engine = event.engine;
		const Tiny::FLAG condition = RENDERFLOWCOMPONENT_RENDER_SYNC_TICKING | TINY_ACTIVATED;
		while ((Flag() & condition) == condition) {
			YieldThread();
		}

		Flag() |= RENDERFLOWCOMPONENT_RENDER_SYNC_TICKING;
		engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &RenderFlowComponent::RenderSyncTick), std::ref(engine)));

		Render(event.engine);
	} else if (event.eventID == Event::EVENT_TICK) {
		// No operations on trivial tick
	}
}
