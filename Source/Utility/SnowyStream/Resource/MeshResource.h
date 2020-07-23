// MeshResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __MESH_RESOURCE_H__
#define __MESH_RESOURCE_H__

#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"
#include "../../../General/Misc/PassBase.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class MeshResource : public TReflected<MeshResource, GraphicResourceBase> {
		public:
			MeshResource(ResourceManager& manager, const String& uniqueID);
			virtual ~MeshResource();
			virtual void Upload(IRender& render, void* deviceContext) override;
			virtual void Download(IRender& render, void* deviceContext) override;
			virtual void Attach(IRender& render, void* deviceContext) override;
			virtual void Detach(IRender& render, void* deviceContext) override;
			virtual void Unmap() override;
			virtual size_t ReportDeviceMemoryUsage() const;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			const Float3Pair& GetBoundingBox() const;

		public:
			IAsset::MeshCollection meshCollection;
			Float3Pair boundingBox;
			size_t deviceMemoryUsage;

			struct BufferCollection : public TReflected<BufferCollection, IReflectObjectComplex> {
				BufferCollection();
				virtual TObject<IReflect>& operator () (IReflect& reflect) override;
				void GetDescription(std::vector<PassBase::Parameter>& desc, std::vector<std::pair<uint32_t, uint32_t> >& offsets, PassBase::Updater& updater) const;
				void UpdateData(std::vector<IRender::Resource*>& data) const;

				IRender::Resource* indexBuffer;
				IRender::Resource* positionBuffer;
				IRender::Resource* normalTangentColorBuffer;
				IRender::Resource* boneIndexWeightBuffer;
				bool hasNormalBuffer;
				bool hasTangentBuffer;
				bool hasColorBuffer;
				bool hasIndexWeightBuffer;
				std::vector<IRender::Resource*> texCoordBuffers;
			} bufferCollection;
		};
	}
}

#endif // __MESH_RESOURCE_H__