// ShapeComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __SHAPECOMPONENT_H__
#define __SHAPECOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../../Core/Template/TKdTree.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ShapeComponent : public TAllocatedTiny<ShapeComponent, Component> {
		public:
			ShapeComponent();
			virtual ~ShapeComponent();
			void Update(Engine& engine, TShared<NsSnowyStream::MeshResource> resource);
			virtual void Raycast(std::vector<RaycastResult>& results, Float3Pair& ray, uint32_t maxCount, IReflectObject* filter) const override;
			enum {
				MAX_PATCH_COUNT = (64 - sizeof(TKdTree<Float3Pair>)) / sizeof(uint32_t)
			};

		protected:
			void Cleanup();
			struct_aligned(64) Patch : public TKdTree<Float3Pair> {
				uint32_t indices[MAX_PATCH_COUNT];
			};

			struct PatchRaycaster;
			static void MakeHeapInternal(std::vector<Patch>& target, const Patch* begin, const Patch* end);
			static Patch* MakeBound(Patch& patch, const std::vector<Float3>& vertices, const std::vector<UInt3>& indices);

			TShared<NsSnowyStream::MeshResource> meshResource;
			std::vector<Patch> patches;
		};
	}
}


#endif // __SHAPECOMPONENT_H__