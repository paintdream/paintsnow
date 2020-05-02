// LightComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __LIGHTCOMPONENT_H__
#define __LIGHTCOMPONENT_H__

#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../Stream/StreamComponent.h"
#include "../../../SnowyStream/Resource/TextureResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		class LightComponent : public TAllocatedTiny<LightComponent, RenderableComponent> {
		public:
			LightComponent();
			enum {
				LIGHTCOMPONENT_DIRECTIONAL = COMPONENT_CUSTOM_BEGIN,
				LIGHTCOMPONENT_CAST_SHADOW = COMPONENT_CUSTOM_BEGIN << 1,
				LIGHTCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 2
			};

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual FLAG GetEntityFlagMask() const override;
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData);
			virtual void UpdateBoundingBox(Engine& engine, Float3Pair& box) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;
			void BindShadowStream(uint32_t layer, TShared<StreamComponent> streamComponent, const Float2& gridSize);
			const Float3& GetColor() const;
			void SetColor(const Float3& color);
			float GetAttenuation() const;
			void SetAttenuation(float value);
			const Float3& GetRange() const;
			void SetRange(const Float3& range);

			// float spotAngle;
			// float temperature;
			struct ShadowLayer {
				TShared<StreamComponent> streamComponent;
				Float2 gridSize;
			};

			class ShadowGrid : public TReflected<ShadowGrid, SharedTiny> {
			public:
				TShared<NsSnowyStream::TextureResource> texture;
				uint32_t layer;
			};

		protected:
			void ReplaceStreamComponent(ShadowLayer& shadowLayer, TShared<StreamComponent> streamComponent);
			TShared<SharedTiny> StreamLoadHandler(Engine& engine, const UShort3& coord, TShared<SharedTiny> tiny);
			TShared<SharedTiny> StreamUnloadHandler(Engine& engine, const UShort3& coord, TShared<SharedTiny> tiny);

			Float3 color;
			float attenuation;
			Float3 range;
			std::vector<ShadowLayer> shadowLayers;
		};
	}
}


#endif // __LIGHTCOMPONENT_H__
