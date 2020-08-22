// HoneyData.h
// By PaintDream (paintdream@paintdream.com)
// 2015-12-31
//

#pragma once
#include "../../General/Interface/IDatabase.h"
#include "../../Core/Interface/IScript.h"
#include "../../General/Misc/DynamicObject.h"
#include "../../Core/System/Kernel.h"

namespace PaintsNow {
	class SchemaResolver : public IReflect {
	public:
		SchemaResolver();
		virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta);
		static void SetValueString(IScript::Request& request, char* base);
		static void SetValueText(IScript::Request& request, char* base);
		static void SetValueFloat(IScript::Request& request, char* base);
		static void SetValueInt(IScript::Request& request, char* base);
		static void SetValueNull(IScript::Request& request, char* base);
		virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta);

		typedef void(*Set)(IScript::Request& request, char* base);
		std::vector<std::pair<Set, size_t> > setters;
	};

	class Honey : public TReflected<Honey, WarpTiny> {
	public:
		Honey(IDatabase::MetaData* metaData);
		virtual ~Honey();
		bool Step();
		void WriteLine(IScript::Request& request);
		virtual TObject<IReflect>& operator () (IReflect& reflect);

	protected:
		SchemaResolver resolver;
		IDatabase::MetaData* metaData;
	};

	class HoneyData : public IDatabase::MetaData {
	public:
		HoneyData();
		virtual ~HoneyData();

		virtual IIterator* New() const;
		virtual void Attach(void* base);
		virtual void Initialize(size_t count);
		virtual size_t GetTotalCount() const;
		virtual void* Get();
		virtual const IReflectObject& GetPrototype() const;
		virtual bool Next();
		virtual bool IsLayoutLinear() const;
		virtual bool IsLayoutPinned() const;
		virtual void* GetHost() const;
		virtual Unique GetPrototypeUnique() const;
		virtual Unique GetPrototypeReferenceUnique() const;
		virtual const String& GetInternalName() const;

		void Enter();
		void Leave();

		void SetFloat(size_t i);
		void SetString(size_t i);
		void SetInteger(size_t i);

	private:
		DynamicUniqueAllocator uniqueAllocator;
		DynamicObject* dynamicObject;
		IScript::Request* request;
		IScript::Request::Ref tableRef;
		size_t index;
		size_t count;
		typedef void (HoneyData::* Set)(size_t i);
		std::vector<Set> sets;
	};

	IScript::Request& operator >> (IScript::Request& request, HoneyData& honey);
}
