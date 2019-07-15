// ZScriptReflect.h
// By PaintDream (paintdream@paintdream.com)
// 2015-7-8
//

#ifndef __ZSCRIPTREFLECT_H__
#define __ZSCRIPTREFLECT_H__

#include "../../Core/Interface/IScript.h"
#include "../../Core/Interface/IReflect.h"

namespace PaintsNow {
	class ZScriptReflect : public IReflect {
	public:
		// for IReflectObject
		class ValueParserBase {
		public:
			virtual Unique GetValueType() const = 0;
			virtual void WriteValue(IScript::Request& request, const void* base) const = 0;
			virtual void ReadValue(IScript::Request& request, void* base) const = 0;
		};

		template <class T>
		class ValueParser : public ValueParserBase {
		public:
			typedef T type;
			virtual Unique GetValueType() const {
				static Unique u = UniqueType<T>::Get();
				return u;
			}

			virtual void WriteValue(IScript::Request& request, const void* base) const {
				request << *(reinterpret_cast<const T*>(base));
			}

			virtual void ReadValue(IScript::Request& request, void* base) const {
				request >> *(reinterpret_cast<T*>(base));
			}
		};

		struct Type {
			Type(const String& n = "", const ValueParserBase* b = nullptr) : name(n), parser(b) {}
			String name;
			const ValueParserBase* parser;
		};

		ZScriptReflect(IScript::Request& request, bool read, const std::map<Unique, Type>& reflectParserMap = ZScriptReflect::GetGlobalMap());

		virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta);
		virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const IReflect::Param& retValue, const std::vector<IReflect::Param>& params, const MetaChainBase* meta);

		const Type& GetType(Unique id) const;
		static const std::map<Unique, Type>& GetGlobalMap();
		template <class T>
		ZScriptReflect& operator >> (T& object) {
			static Unique u = UniqueType<T>::Get();
			Perform(object, &object, u);
			return *this;
		}

		template <class T>
		ZScriptReflect& operator << (const T& object) {
			assert(!read);
			static Unique u = UniqueType<T>::Get();
			Perform(object, const_cast<T*>(&object), u);
			return *this;
		}

	protected:
		void Perform(const IReflectObject& s, void* base, Unique id);
		static std::map<Unique, Type> globalReflectParserMap;
		void Atom(Unique typeID, void* base);
		const std::map<Unique, Type>& reflectParserMap;
		IScript::Request& request;
		bool read;
	};
}

#endif // __ZSCRIPTREFLECT_H__
