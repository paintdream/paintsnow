#ifndef __ZSCRIPTTCC_H__
#define __ZSCRIPTTCC_H__

#include "../../../../Core/Interface/IScript.h"
#define TCC_IS_NATIVE
#include "Core/libtcc.h"

namespace PaintsNow {
	// A general interface of tcc
	// only a code frame now.

	class ZScriptTCC final : public IScript {
	public:
		ZScriptTCC(IThread& threadApi);
		virtual ~ZScriptTCC();

		class Request : public IScript::Request {
		public:
			Request(ZScriptTCC* s);
			virtual ~Request();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual IScript* GetScript();
			virtual bool Call(const AutoWrapperBase& defer, const Request::Ref& g);
			virtual std::vector<Key> Enumerate();
			static IScript::Request::TYPE ConvertType(int type);
			virtual Request::Ref Load(const String& script, const String& pathname);
			virtual TYPE GetCurrentType();
			virtual IScript::Request& Push();
			virtual IScript::Request& Pop();
			virtual IScript::Request& operator >> (IScript::Request::Ref&);
			virtual IScript::Request& operator << (const IScript::Request::Ref&);
			virtual IScript::Request& operator << (const IScript::Request::Nil&);
			virtual IScript::Request& operator << (const IScript::BaseDelegate&);
			virtual IScript::Request& operator >> (IScript::BaseDelegate&);
			virtual IScript::Request& operator << (const IScript::Request::Global&);
			virtual IScript::Request& operator << (const IScript::Request::Local&);
			virtual IScript::Request& operator << (const IScript::Request::TableStart&);
			virtual IScript::Request& operator >> (IScript::Request::TableStart&);
			virtual IScript::Request& operator << (const IScript::Request::TableEnd&);
			virtual IScript::Request& operator >> (const IScript::Request::TableEnd&);
			virtual IScript::Request& operator << (const IScript::Request::ArrayStart&);
			virtual IScript::Request& operator >> (IScript::Request::ArrayStart&);
			virtual IScript::Request& operator << (const IScript::Request::ArrayEnd&);
			virtual IScript::Request& operator >> (const IScript::Request::ArrayEnd&);
			virtual IScript::Request& operator << (const IScript::Request::Key&);
			virtual IScript::Request& operator >> (const IScript::Request::Key&);
			virtual IScript::Request& operator << (double value);
			virtual IScript::Request& operator >> (double& value);
			virtual IScript::Request& operator << (const String& str);
			virtual IScript::Request& operator >> (String& str);
			virtual IScript::Request& operator << (const char* str);
			virtual IScript::Request& operator >> (const char*& str);
			virtual IScript::Request& operator << (int64_t value);
			virtual IScript::Request& operator >> (int64_t& value);
			virtual IScript::Request& operator << (bool b);
			virtual IScript::Request& operator >> (bool& b);
			virtual IScript::Request& operator << (const AutoWrapperBase& wrapper);
			virtual IScript::Request& operator >> (const Skip& skip);
			virtual IScript::Request& MoveVariables(IScript::Request& target, size_t count);
			virtual bool IsValid(const BaseDelegate& d);
			virtual void Dereference(IScript::Request::Ref& ref);
			virtual IScript::Request::Ref Reference(const IScript::Request::Ref& d);
			virtual IScript::Request::TYPE GetReferenceType(const IScript::Request::Ref& d);
			virtual int GetCount();
			void SetIndex(int i);
			IScript::Request& CleanupIndex();

		private:
			ZScriptTCC* host;
			TCCState* state;
			String binary;
			String key;
		};

		friend class Request;

		virtual void Reset();
		virtual IScript::Request& GetDefaultRequest();
		virtual IScript* NewScript() const;
		virtual IScript::Request* NewRequest(const String& entry);
		virtual const char* GetFileExt() const;

	private:
		IScript::Request* defaultRequest;
		static bool instanceMutex;

#ifdef _DEBUG
		std::map<long, String> debugReferences;
#endif
	};
}

#endif // __ZSCRIPTTCC_H__
