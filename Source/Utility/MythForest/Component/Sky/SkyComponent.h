// SkyComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../Model/ModelComponent.h"

namespace PaintsNow {
	class SkyComponent : public TAllocatedTiny<SkyComponent, ModelComponent> {
	public:
		SkyComponent(const TShared<MeshResource>& meshResource, const TShared<BatchComponent>& batchComponent);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		size_t ReportGraphicMemoryUsage() const override;
	};
}
