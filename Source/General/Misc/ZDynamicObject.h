// ZDynamicObject.h
// 2015-6-21
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __ZDYNAMICOBJECT_H__
#define __ZDYNAMICOBJECT_H__

#include "../../Core/Interface/IReflect.h"

namespace PaintsNow {
	class ZDynamicObject;
	class ZDynamicInfo : public IUniqueInfo {
	public:
		struct MemController {
			void (*Creator)(void*);
			void (*Deletor)(void*);
			void (*Assigner)(void*, const void*);
		};

		struct Field {
			Field();
			String name;
			Unique type;
			MemController* controller;

			struct {
				uint32_t reflectable : 1;
				uint32_t offset : 31;
			};

			bool operator < (const Field& rhs) const {
				return offset < rhs.offset;
			}
		};

		const Field* operator [] (const String& key) const;

		virtual IReflectObject* Create() const;

		std::vector<Field> fields;
		std::map<String, uint32_t> mapNameToField;
	};

	class ZDynamicUniqueAllocator : public IUniqueAllocator {
	public:
		ZDynamicUniqueAllocator();

		virtual IUniqueInfo* Alloc(const String& name, size_t size);
		virtual IUniqueInfo* Get(const String& name);

		IUniqueInfo* AllocFromDescriptor(const String& name, const std::vector<ZDynamicInfo::Field>& descriptors);
		std::map<String, ZDynamicInfo> mapType;
	};

	class ZDynamicObject : public TReflected<ZDynamicObject, IReflectObjectComplex> {
	public:
		ZDynamicInfo* GetDynamicInfo() const;
		void Set(const ZDynamicInfo::Field& field, const void* value);
		void* Get(const ZDynamicInfo::Field& field) const;
		virtual void ReleaseObject() override;
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		friend class ZDynamicInfo;
		ZDynamicObject(ZDynamicInfo* info);
		virtual ~ZDynamicObject();
		ZDynamicObject& operator = (const ZDynamicObject& rhs);

		ZDynamicInfo* dynamicInfo;
	};

	class ZDynamicVector : public TReflected<ZDynamicVector, IReflectObjectComplex> {
	public:
		static ZDynamicInfo::MemController& GetVectorController();
		void Reinit(Unique unique, ZDynamicInfo::MemController* mc, size_t n, bool reflectable);
		void Set(size_t i, const void* value);
		void* Get(size_t i) const;
		class Iterator : public IIterator {
		public:
			Iterator(ZDynamicVector* vec);
			virtual IIterator* New() const;
			virtual void Attach(void* base);
			virtual void* GetHost() const;
			virtual void Initialize(size_t count);
			virtual size_t GetTotalCount() const;
			virtual void* Get();
			virtual const IReflectObject& GetPrototype() const;
			virtual Unique GetPrototypeUnique() const;
			virtual Unique GetPrototypeReferenceUnique() const;
			virtual bool IsLayoutLinear() const;
			virtual bool IsLayoutPinned() const;
			virtual bool Next();

		private:
			ZDynamicVector* base;
			uint32_t i;
		};

	protected:
		static void VectorCreator(void* buffer);
		static void VectorDeletor(void* buffer);
		static void VectorAssigner(void* dst, const void* src);

		friend class ZDynamicObject;
		friend class Iterator;
		ZDynamicVector(Unique unique, ZDynamicInfo::MemController*, size_t c, bool reflectable);
		virtual ~ZDynamicVector();
		ZDynamicVector& operator = (const ZDynamicVector&);
		void Init();
		void Cleanup();

		Unique unique;
		ZDynamicInfo::MemController* memController;
		struct {
			uint32_t reflectable : 1;
			uint32_t count : 31;
		};
		void* buffer;

	private:
		ZDynamicVector(const ZDynamicVector&);
	};

	class ZDynamicObjectWrapper : public TReflected<ZDynamicObjectWrapper, IReflectObjectComplex> {
	public:
		ZDynamicObjectWrapper(ZDynamicUniqueAllocator& allocator);
		virtual ~ZDynamicObjectWrapper();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		ZDynamicUniqueAllocator& uniqueAllocator;
		ZDynamicObject* dynamicObject;
	};
}

#endif // __ZDYHNAMICOBJECT_H__