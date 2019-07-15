// ShaderResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __SHADER_RESOURCE_H__
#define __SHADER_RESOURCE_H__

#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"
#include "../../../General/Misc/ZPassBase.h"
#include "Passes/CustomizeShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class ShaderResource : public TReflected<ShaderResource, GraphicResourceBase>, public ICustomizeShader {
		public:
			ShaderResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID);
			virtual ~ShaderResource();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			static const String& GetShaderPathPrefix();

			virtual uint64_t GetMemoryUsage() const;
			virtual const String& GetShaderName() const;
			virtual bool operator << (IStreamBase& stream) override;
			virtual bool operator >> (IStreamBase& stream) const override;
			virtual void Upload(IRender& render, void* deviceContext) override;
			virtual void Download(IRender& render, void* deviceContext) override;
			virtual void Attach(IRender& render, void* deviceContext) override;
			virtual void Detach(IRender& render, void* deviceContext) override;
			virtual IReflectObject* Clone() const override;
			virtual ZPassBase& GetPass();
			virtual ZPassBase::Updater& GetPassUpdater();

			IRender::Resource* GetShaderResource() const;
			const Bytes& GetHashValue() const;

			virtual void SetCode(const String& stage, const String& text, const std::vector<std::pair<String, String> >& config) override final;
			virtual void SetInput(const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config) override final;
			virtual void SetComplete() override final;

		protected:
			Bytes hashValue;
			IRender::Resource* shaderResource;
		};

		template <class T>
		class ShaderResourceImpl : public TReflected<ShaderResourceImpl<T>, ShaderResource> {
		public:
 			// gcc do not support referencing base type in template class. manunaly specified here.
			typedef TReflected<ShaderResourceImpl<T>, ShaderResource> BaseClass;
			ShaderResourceImpl(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID, Tiny::FLAG f = 0) : BaseClass(manager, uniqueID), updater(pass) { BaseClass::Flag() |= f; }
		
			virtual ZPassBase& GetPass() override {
				return pass;
			}

			virtual ZPassBase::Updater& GetPassUpdater() override {
				return updater;
			}

			virtual IReflectObject* Clone() const override {
				ShaderResource* resource = new ShaderResourceImpl<T>(BaseClass::resourceManager, ""); // on the fly
				resource->Flag() |= BaseClass::RESOURCE_ORPHAN;
				return resource;
			}

			virtual Unique GetBaseUnique() const override {
				return UniqueType<ShaderResource>::Get();
			}

		protected:
			T pass;
			ZPassBase::Updater updater;
		};

	}
}

#endif // __SHADER_RESOURCE_H__