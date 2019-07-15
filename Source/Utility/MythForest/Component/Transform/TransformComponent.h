// TranformComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#ifndef __TRANSFORMCOMPONENT_H__
#define __TRANSFORMCOMPOENNT_H__

#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/Interface/IType.h"

namespace PaintsNow {
	namespace NsMythForest {
		class TransformComponent : public TAllocatedTiny<TransformComponent, UniqueComponent<Component, SLOT_TRANSFORM_COMPONENT> > {
		public:
			TransformComponent();
			enum {
				TRANSFORMCOMPONENT_DYNAMIC = COMPONENT_CUSTOM_BEGIN,
				TRANSFORMCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
			};
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void UpdateBoundingBox(Engine& engine, Float3Pair& box) override;
			virtual void Raycast(std::vector<RaycastResult>& results, Float3Pair& ray, uint32_t maxCount, IReflectObject* filter) const override;

		public:
			Float3 GetQuickTranslation() const;
			void SetRotation(const Float3& rotation);
			void EditorRotate(const Float2& from, const Float2& to);
			Float3 GetRotation();
			const Quaternion<float>& GetRotationQuaternion();
			void SetTranslation(const Float3& translation);
			const Float3& GetTranslation();
			void SetScale(const Float3& scale);
			const Float3& GetScale();
			void UpdateTransform();
			void GetAxises(Float3& xAxis, Float3& yAxis, Float3& zAxis) const;

			const MatrixFloat4x4& GetTransform() const;
			void SetTransform(const MatrixFloat4x4& transform);
			void SetObjectID(uint32_t id);
			uint32_t GetObjectID() const;

		protected:
			class TRS : public TReflected<TRS, SharedTiny> {
			public:
				Quaternion<float> rotation;
				Float3 scale;
				Float3 translation;
			};

			TShared<TRS>& Modify();

		protected:
			MatrixFloat4x4 transform;
			uint32_t uniqueObjectID;
			TShared<TRS> trsData;
		};
	}
}


#endif // __TRANSFORMCOMPONENT_H__