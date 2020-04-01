// Unit.h
// By PaintDream (paintdream@paintdream.com)
// 2018-7-27
//

#ifndef __UNIT_H__
#define __UNIT_H__

#include "../../Core/System/Kernel.h"
#include "../../Core/Template/TBuffer.h"
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IStreamBase.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Unit : public TReflected<Unit, WarpTiny> {
		public:
			struct RaycastResult {
				Float3 position; // local position
				Float3 normal;
				Float2 coord;
				float distance;
				TShared<Unit> unit;
				TShared<Unit> parent;
				Unique metaType;
				Bytes metaData;
			};

			static bool EmplaceRaycastResult(std::vector<RaycastResult>& results, uint32_t maxCount, const RaycastResult& item);
			virtual void Raycast(std::vector<RaycastResult>& results, Float3Pair& ray, uint32_t maxCount, IReflectObject* metaInfo) const;
			virtual String GetDescription() const;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			enum {
				UNIT_CUSTOM_BEGIN = WARP_CUSTOM_BEGIN
			};
		};

		class MetaUnitIdentifier : public TReflected<MetaUnitIdentifier, MetaStreamPersist> {
		public:
			MetaUnitIdentifier();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			template <class T, class D>
			inline const MetaUnitIdentifier& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef MetaUnitIdentifier Type;
			};

			typedef MetaUnitIdentifier Type;

			virtual bool Read(IStreamBase& streamBase, void* ptr) const;
			virtual bool Write(IStreamBase& streamBase, const void* ptr) const;
			virtual const String& GetUniqueName() const;

		private:
			String uniqueName;
		};
	}
}


#endif // __UNIT_H__