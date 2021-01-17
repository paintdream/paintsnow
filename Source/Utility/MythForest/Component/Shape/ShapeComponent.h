// ShapeComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../../Core/Template/TKdTree.h"

namespace PaintsNow {
	class ShapeComponent : public TAllocatedTiny<ShapeComponent, Component> {
	public:
		ShapeComponent();
		~ShapeComponent() override;
		void Update(Engine& engine, const TShared<MeshResource>&resource);
		float Raycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio = 1) const override;
		enum {
			MAX_PATCH_COUNT = 8
		};

	protected:
		void Cleanup();
		struct_aligned(8) Patch : public TKdTree<Float3Pair> {
			union {
				uint32_t indices[MAX_PATCH_COUNT];
				uint64_t alignment;
			};
		};

		struct PatchRaycaster;
		static void MakeHeapInternal(std::vector<Patch>& target, Patch* begin, Patch* end);
		static Patch* MakeBound(Patch& patch, const std::vector<Float3>& vertices, const std::vector<UInt3>& indices, int index);

		TShared<MeshResource> meshResource;
		std::vector<Patch> patches;
	};
}

