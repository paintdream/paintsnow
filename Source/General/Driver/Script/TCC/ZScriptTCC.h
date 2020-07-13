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
			virtual IScript* GetScript() override;
			virtual bool Call(const AutoWrapperBase& defer, const Request::Ref& g) override;
			virtual std::vector<Key> Enumerate() override;
			static IScript::Request::TYPE ConvertType(int type);
			virtual Request::Ref Load(const String& script, const String& pathname) override;
			virtual TYPE GetCurrentType() override;
			virtual IScript::Request& Push() override;
			virtual IScript::Request& Pop() override;
			virtual IScript::Request& operator >> (IScript::Request::Arguments&) override;
			virtual IScript::Request& operator >> (IScript::Request::Ref&) override;
			virtual IScript::Request& operator << (const IScript::Request::Ref&) override;
			virtual IScript::Request& operator << (const IScript::Request::Nil&) override;
			virtual IScript::Request& operator << (const IScript::BaseDelegate&) override;
			virtual IScript::Request& operator >> (IScript::BaseDelegate&) override;
			virtual IScript::Request& operator << (const IScript::Request::Global&) override;
			virtual IScript::Request& operator << (const IScript::Request::TableStart&) override;
			virtual IScript::Request& operator >> (IScript::Request::TableStart&) override;
			virtual IScript::Request& operator << (const IScript::Request::TableEnd&) override;
			virtual IScript::Request& operator >> (const IScript::Request::TableEnd&) override;
			virtual IScript::Request& operator << (const IScript::Request::ArrayStart&) override;
			virtual IScript::Request& operator >> (IScript::Request::ArrayStart&) override;
			virtual IScript::Request& operator << (const IScript::Request::ArrayEnd&) override;
			virtual IScript::Request& operator >> (const IScript::Request::ArrayEnd&) override;
			virtual IScript::Request& operator << (const IScript::Request::Key&) override;
			virtual IScript::Request& operator >> (const IScript::Request::Key&) override;
			virtual IScript::Request& operator << (double value) override;
			virtual IScript::Request& operator >> (double& value) override;
			virtual IScript::Request& operator << (const String& str) override;
			virtual IScript::Request& operator >> (String& str) override;
			virtual IScript::Request& operator << (const char* str) override;
			virtual IScript::Request& operator >> (const char*& str) override;
			virtual IScript::Request& operator << (int64_t value) override;
			virtual IScript::Request& operator >> (int64_t& value) override;
			virtual IScript::Request& operator << (bool b) override;
			virtual IScript::Request& operator >> (bool& b) override;
			virtual IScript::Request& operator << (const AutoWrapperBase& wrapper) override;
			virtual IScript::Request& MoveVariables(IScript::Request& target, size_t count) override;
			bool IsValid(const BaseDelegate& d);
			virtual void Dereference(IScript::Request::Ref& ref) override;
			virtual IScript::Request::Ref Reference(const IScript::Request::Ref& d) override;
			virtual IScript::Request::TYPE GetReferenceType(const IScript::Request::Ref& d) override;
			virtual int GetCount() override;
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
