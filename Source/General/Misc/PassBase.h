// PassBase -- Render Pass
// By PaintDream (paintdream@paintdream.com)
// 2014-12-3
//

#ifndef __PASSBASE_H__
#define __PASSBASE_H__

#include "../../General/Interface/IShader.h"
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Template/TEvent.h"

namespace PaintsNow {
	// New version of PassBase !
	class PassBase : public TReflected<PassBase, IReflectObjectComplex> {
	public:
		PassBase();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual ~PassBase();
		IRender::Resource* Compile(IRender& render, IRender::Queue* queue);
		Bytes ExportHash(bool onlyConstants);

		struct Name {
			bool operator < (const Name& rhs) const;
			Bytes key;
			IShader::BindInput::SCHEMA schema;
		};

		struct Parameter {
			Parameter();
			template <class T>
			Parameter& operator = (const T& value) {
				if (internalAddress != nullptr) {
					assert(type == UniqueType<T>::Get());
					memcpy(internalAddress, &value, sizeof(T));
				}

				return *this;
			}

			inline Parameter& operator = (const Bytes& data) {
				if (internalAddress != nullptr) {
					assert(type->GetSize() == data.GetSize());
					memcpy(internalAddress, data.GetData(), data.GetSize());
				}

				return *this;
			}

			inline operator bool() const {
				return internalAddress != nullptr;
			}

			// output
			uint8_t linearLayout : 1;
			uint8_t resourceType : 7;
			uint8_t slot; // N'st slot of buffer/texture
			uint16_t stride;
			uint16_t offset;
			uint32_t length;
			void* internalAddress;
			Unique type;
		};

		class Updater : public IReflect {
		public:
			Updater(PassBase& pass);
			static Bytes MakeKeyFromString(const String& s);

			Parameter& operator [] (const Bytes& key);
			Parameter& operator [] (IShader::BindInput::SCHEMA schema);

			void Capture(IRender::Resource::DrawCallDescription& drawCallDescription, std::vector<Bytes>& bufferData, uint32_t bufferMask);
			void Update(IRender& render, IRender::Queue* queue, IRender::Resource::DrawCallDescription& drawCall, std::vector<IRender::Resource*>& newBuffers, std::vector<Bytes>& bufferData, uint32_t bufferMask);
			void Flush();
			virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta);
			virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}
			
			uint32_t GetBufferCount() const;
			uint32_t GetTextureCount() const;

		private:
			std::vector<const IShader::BindBuffer*> buffers;
			std::map<const IShader::BindBuffer*, uint32_t> bufferSize;
			std::map<const IShader::BindBuffer*, uint32_t> bufferID;
			uint32_t textureCount;
			std::map<Name, size_t> mapParameters;
			std::vector<size_t> quickUpdaters;
			std::vector<Parameter> parameters;

#ifdef _DEBUG
			std::set<const void*> disabled;
#endif
		};

		class PartialData;
		class PartialUpdater {
		public:
			Bytes ComputeHash() const;
			void Snapshot(std::vector<Bytes>& buffers, std::vector<IRender::Resource::DrawCallDescription::BufferRange>& bufferResources, std::vector<IRender::Resource*>& textureResources, const PartialData& data) const;
			
			std::vector<Parameter> parameters;
		};

		class PartialData : public TReflected<PartialData, IReflectObjectComplex> {
		public:
			void Export(PartialUpdater& particalUpdater, Updater& updater) const;
		};

		static inline bool ValidateDrawCall(const IRender::Resource::DrawCallDescription& drawCall) {
			// Check DrawCall completeness
			if (drawCall.indexBufferResource.buffer == nullptr) return false;
			if (drawCall.shaderResource == nullptr) return false;

			for (size_t i = 0; i < drawCall.textureResources.size(); i++) {
				if (drawCall.textureResources[i] == nullptr) return false;
			}

			for (size_t k = 0; k < drawCall.bufferResources.size(); k++) {
				if (drawCall.bufferResources[k].buffer == nullptr) return false;
			}

			return true;
		}
	};
}

#endif // __PASSBASE_H__
