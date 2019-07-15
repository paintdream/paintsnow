// BatchComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __BATCHCOMPONENT_H__
#define __BATCHCOMPONENT_H__

#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		// Manages Vertex/Uniform/Instance buffers statically
		class RenderableComponent;
		class BatchComponent : public TAllocatedTiny<BatchComponent, Component> {
		public:
			BatchComponent();
			virtual ~BatchComponent();
			virtual void Initialize(Engine& engine, Entity* entity) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;
			virtual void DispatchEvent(Event& event, Entity* entity) override;

			IRender::Resource::DrawCallDescription::BufferRange Allocate(IRender& render, const Bytes& data);
			void Flush(IRender& render);

			void InstanceInitialize(Engine& engine);
			void InstanceUninitialize(Engine& engine);

		private:
			Bytes currentData;
			IRender::Resource* buffer;
			IRender::Queue* queue;
			uint32_t referenceCount;
		};
	}
}


#endif // __BATCHCOMPONENT_H__
