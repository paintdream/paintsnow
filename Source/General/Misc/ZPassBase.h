// ZPassBase -- Render Pass
// By PaintDream (paintdream@paintdream.com)
// 2014-12-3
//

#ifndef __ZPassBase_H__
#define __ZPassBase_H__

#include "../../General/Interface/IShader.h"
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Template/TEvent.h"

namespace PaintsNow {
	// New version of ZPassBase !
	class ZPassBase : public TReflected<ZPassBase, IReflectObjectComplex> {
	public:
		ZPassBase();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual ~ZPassBase();
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

			inline Parameter& operator = (const String& data) {
				if (internalAddress != nullptr) {
					assert(type->GetSize() == data.size());
					memcpy(internalAddress, data.c_str(), data.size());
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
			uint16_t length;
			void* internalAddress;
			Unique type;
		};

		class Updater : public IReflect {
		public:
			Updater(ZPassBase& pass);
			Parameter& operator [] (const Bytes& key);
			Parameter& operator [] (IShader::BindInput::SCHEMA schema);

			void Capture(IRender::Resource::DrawCallDescription& drawCallDescription, std::vector<Bytes>& bufferData, uint32_t bufferMask);
			void Update(IRender& render, IRender::Queue* queue, IRender::Resource::DrawCallDescription& drawCall, std::vector<IRender::Resource*>& newBuffers, std::vector<Bytes>& bufferData, uint32_t bufferMask);
			void Flush();
			virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta);
			virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

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
			void Snapshot(std::vector<Bytes>& buffers, const PartialData& data) const;
			
			std::vector<Parameter> parameters;
			std::vector<std::pair<uint8_t, std::vector<uint16_t> > > groupInfos;
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

#endif // __ZPassBase_H__
