// ScriptReflect.h
// By PaintDream (paintdream@paintdream.com)
// 2015-7-8
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IArchive.h"
#include "../../Core/System/Kernel.h"

namespace PaintsNow {
	class ScriptReflect : public IReflect {
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
			Unique GetValueType() const override {
				singleton Unique u = UniqueType<T>::Get();
				return u;
			}

			void WriteValue(IScript::Request& request, const void* base) const override {
				request << *(reinterpret_cast<const T*>(base));
			}

			void ReadValue(IScript::Request& request, void* base) const override {
				request >> *(reinterpret_cast<T*>(base));
			}
		};

		struct Type {
			Type(const String& n = "", const ValueParserBase* b = nullptr) : name(n), parser(b) {}
			String name;
			const ValueParserBase* parser;
		};

		ScriptReflect(IScript::Request& request, bool read, const std::unordered_map<Unique, Type>& reflectParserMap = ScriptReflect::GetGlobalMap());

		void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override;
		void Method(const char* name, const TProxy<>* p, const IReflect::Param& retValue, const std::vector<IReflect::Param>& params, const MetaChainBase* meta) override;

		const Type& GetType(Unique id) const;
		static const std::unordered_map<Unique, Type>& GetGlobalMap();
		template <class T>
		ScriptReflect& operator >> (T& object) {
			singleton Unique u = UniqueType<T>::Get();
			Perform(object, &object, u);
			return *this;
		}

		template <class T>
		ScriptReflect& operator << (const T& object) {
			assert(!read);
			singleton Unique u = UniqueType<T>::Get();
			Perform(object, const_cast<T*>(&object), u);
			return *this;
		}

	protected:
		void Perform(const IReflectObject& s, void* base, Unique id);
		static std::unordered_map<Unique, Type> globalReflectParserMap;
		void Atom(Unique typeID, void* base);
		const std::unordered_map<Unique, Type>& reflectParserMap;
		IScript::Request& request;
		bool read;
	};

	class Tunnel;
	class Proxy {
	public:
		Proxy(Tunnel* hostTunnel = nullptr, const TProxy<>* routine = nullptr);
		void OnCall(IScript::Request& request);

	private:
		Tunnel* hostTunnel;
		const TProxy<>* routine;
	};

	
	class Bridge : public TReflected<Bridge, IReflectObjectComplex>, public ISyncObject {
	public:
		Bridge(IThread& thread);
		~Bridge() override;

		virtual IReflectObject* Create(IScript::Request& request, IArchive& archive, const String& path, const String& data) = 0;
		virtual void Call(IReflectObject* object, const TProxy<>* p, IScript::Request& request) = 0;
		virtual std::unordered_map<Unique, ScriptReflect::Type>& GetReflectMap() = 0;
		// virtual void Dump(IScript::Request& request, Tunnel& tunnel, IReflectObject* object) = 0;
	};

	class Tunnel : public TReflected<Tunnel, WarpTiny> {
	public:
		Tunnel(Bridge* bridge, IReflectObject* host);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		~Tunnel() override;
		void ForwardCall(const TProxy<>* p, IScript::Request& request);
		Proxy& NewProxy(const TProxy<>* p);
		IReflectObject* GetHost() const;
		void Dump(IScript::Request& request);

	private:
		Bridge* bridge;
		IReflectObject* host;
		std::list<Proxy> proxy;
	};

	class ObjectDumper : public ScriptReflect {
	public:
		ObjectDumper(IScript::Request& request, Tunnel& tunnel, const std::unordered_map<Unique, Type>& m);
		void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override;
		void Method(const char* name, const TProxy<>* p, const IReflect::Param& retValue, const std::vector<IReflect::Param>& params, const MetaChainBase* meta) override;

	private:
		IScript::Request& request;
		Tunnel& tunnel;
	};
}

