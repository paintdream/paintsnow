// MeshResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __MESH_RESOURCE_H__
#define __MESH_RESOURCE_H__

#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"
#include "../../../General/Misc/ZPassBase.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class MeshResource : public TReflected<MeshResource, GraphicResourceBase> {
		public:
			MeshResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID);
			virtual ~MeshResource();
			virtual uint64_t GetMemoryUsage() const;
			virtual void Upload(IRender& render, void* deviceContext) override;
			virtual void Download(IRender& render, void* deviceContext) override;
			virtual void Attach(IRender& render, void* deviceContext) override;
			virtual void Detach(IRender& render, void* deviceContext) override;
			virtual bool Unmap() override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			const Float3Pair& GetBoundingBox() const;

		public:
			IAsset::MeshCollection meshCollection;
			Float3Pair boundingBox;

			struct BufferCollection : public TReflected<BufferCollection, IReflectObjectComplex> {
				BufferCollection();
				virtual TObject<IReflect>& operator () (IReflect& reflect) override;
				void GetDescription(std::vector<ZPassBase::Parameter>& desc, ZPassBase::Updater& updater) const;
				void UpdateData(std::vector<IRender::Resource*>& data) const;

				IRender::Resource* indexBuffer;
				IRender::Resource* positionBuffer;
				IRender::Resource* normalBuffer;
				IRender::Resource* tangentBuffer;
				IRender::Resource* colorBuffer;
				IRender::Resource* boneIndexBuffer;
				IRender::Resource* boneWeightBuffer;
				std::vector<IRender::Resource*> texCoordBuffers;
			} bufferCollection;
		};
	}
}

#endif // __MESH_RESOURCE_H__