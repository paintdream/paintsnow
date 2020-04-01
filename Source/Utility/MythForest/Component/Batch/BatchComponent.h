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
		class BatchComponent : public TAllocatedTiny<BatchComponent, Component>, public NsSnowyStream::IDrawCallProvider::DataUpdater {
		public:
			BatchComponent();
			virtual ~BatchComponent();

			IRender::Resource::DrawCallDescription::BufferRange Allocate(const Bytes& data);
			void Update(IRender& render, IRender::Queue* queue);
			void InstanceInitialize(Engine& engine);
			void InstanceUninitialize(Engine& engine);

		private:
			Bytes currentData;
			IRender::Resource* buffer;
			uint32_t referenceCount;
		};
	}
}


#endif // __BATCHCOMPONENT_H__
