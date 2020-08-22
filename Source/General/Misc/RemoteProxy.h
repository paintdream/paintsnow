// RemoteProxy.h
// By PaintDream (paintdream@paintdream.com)
// 2016-7-15
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/ITunnel.h"
#include "../../Core/System/MemoryStream.h"

namespace PaintsNow {
	class TableImpl;

	enum VALUE_TYPE { TYPE_INT8 = 0, TYPE_BOOL, TYPE_INT16, TYPE_INT32, TYPE_INT64, TYPE_FLOAT, TYPE_DOUBLE, TYPE_STRING, TYPE_OBJECT, TYPE_TABLE, TYPE_ERROR };
	template <class T>
	struct TypeTraits {
		enum { Type = TYPE_ERROR };
	};

	template <>
	struct TypeTraits<bool> {
		enum { Type = TYPE_BOOL };
	};

	template <>
	struct TypeTraits<int8_t> {
		enum { Type = TYPE_INT8 };
	};

	template <>
	struct TypeTraits<uint8_t> {
		enum { Type = TYPE_INT8 };
	};

	template <>
	struct TypeTraits<int16_t> {
		enum { Type = TYPE_INT16 };
	};

	template <>
	struct TypeTraits<uint16_t> {
		enum { Type = TYPE_INT16 };
	};

	template <>
	struct TypeTraits<int32_t> {
		enum { Type = TYPE_INT32 };
	};

	template <>
	struct TypeTraits<uint32_t> {
		enum { Type = TYPE_INT32 };
	};

	template <>
	struct TypeTraits<int64_t> {
		enum { Type = TYPE_INT64 };
	};

	template <>
	struct TypeTraits<uint64_t> {
		enum { Type = TYPE_INT64 };
	};

	template <>
	struct TypeTraits<float> {
		enum { Type = TYPE_FLOAT };
	};

	template <>
	struct TypeTraits<double> {
		enum { Type = TYPE_DOUBLE };
	};

	template <>
	struct TypeTraits<String> {
		enum { Type = TYPE_STRING };
	};

	template <>
	struct TypeTraits<IScript::BaseDelegate> {
		enum { Type = TYPE_OBJECT };
	};

	template <>
	struct TypeTraits<TableImpl> {
		enum { Type = TYPE_TABLE };
	};

	class ValueBase {
	public:
		ValueBase();
		virtual ~ValueBase();
		virtual Unique QueryValueUnique() const = 0;
		virtual ValueBase* Clone() const = 0;
		virtual char GetType() const = 0;
		virtual void Reflect(IReflect& reflect) = 0;
	};


	// save raw value
	template <class T>
	class Value : public ValueBase {
	public:
		enum { Type = TypeTraits<T>::Type };
		virtual char GetType() const {
			assert((int)Type != TYPE_ERROR);
			return Type;
		}
		Value(const T& v) : value(v) {}
		virtual Unique QueryValueUnique() const {
			static Unique type = UniqueType<T>::Get();
			return type;
		}

		virtual void Reflect(IReflect& reflect) {
			ReflectProperty(value);
		}

		virtual ValueBase* Clone() const {
			return new Value(value);
		}

		T value;
	};

	template <>
	class Value<const char*> : public ValueBase {
	public:
		enum { Type = TypeTraits<String>::Type };
		virtual char GetType() const {
			return Type;
		}
		Value(const char*& t) : value(t) {}
		virtual Unique QueryValueUnique() const {
			static Unique u = UniqueType<String>::Get();;
			return u;
		}

		virtual ValueBase* Clone() const {
			return new Value<String>(value);
		}

		virtual void Reflect(IReflect& reflect) {
			if (reflect.IsReflectProperty()) {
				ReflectProperty(value);
			}
		}

		String value;
	};

	class Variant : public TReflected<Variant, IReflectObjectComplex> {
	public:
		Variant() : value(nullptr) {}

		template <class T>
		static ValueBase* Construct(const T& object) {
			char* buffer = new char[sizeof(Value<T>) + sizeof(size_t)];
			ValueBase* value = new(buffer + sizeof(size_t)) Value<T>(object);
			*(size_t*)buffer = 1;
			return value;
		}

		template <class T>
		Variant(const T& object) {
			value = Construct(object);
		}

		inline void AddRef() {
			if (value != nullptr) {
				size_t* ref = (size_t*)((char*)value - sizeof(size_t));
				(*ref)++;
			}
		}

		inline void ReleaseObject() {
			if (value != nullptr) {
				size_t* ref = (size_t*)((char*)value - sizeof(size_t));
				assert(*ref < 0x100);
				if (--(*ref) == 0) {
					value->~ValueBase(); // call destructor manually
					delete[](char*)ref;
				}
			}
		}

		Variant(const Variant& var);
		Variant& operator = (const Variant& var);

		virtual ~Variant();

		ValueBase* Get() const {
			return value;
		}

		ValueBase* operator -> () const {
			return value;
		}

		TObject<IReflect>& operator () (IReflect& reflect) override;
	private:
		ValueBase* value;
	};

	class TableImpl {
	public:
		std::vector<Variant> arrayPart;
		std::map<String, Variant> mapPart;
	};
	
	template <>
	class Value<TableImpl> : public ValueBase {
	public:
		enum { Type = TypeTraits<TableImpl>::Type };

		virtual char GetType() const {
			return Type;
		}

		Value() {}
		Value(const TableImpl& t) : value(t) {}

		virtual Unique QueryValueUnique() const {
			static Unique u = UniqueType<TableImpl>::Get();
			return u;
		}

		virtual ValueBase* Clone() const {
			return new Value<TableImpl>(value);
		}

		virtual void Reflect(IReflect& reflect);

		TableImpl value;
	};

	class RemoteFactory : public TReflected<RemoteFactory, IReflectObjectComplex> {
	public:
		void Initialize(IScript::Request& request, const TWrapper<void, IScript::Request&, IReflectObject&, const IScript::Request::Ref&>& callback);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, const String&> NewObject;
		TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, IScript::BaseDelegate> QueryObject;
		IScript::Request::Ref globalRef;
	};

	class RemoteProxy : public TReflected<RemoteProxy, IReflectObjectComplex>, public IScript {
	public:
		enum STATUS { CONNECTED = 0, CLOSED, ABORTED, TIMEOUT };
		RemoteProxy(IThread& threadApi, ITunnel& tunnel, const TWrapper<IScript::Object*, const String&>& creator, const String& entry, const TWrapper<void, IScript::Request&, bool, STATUS, const String&>& statusHandler = TWrapper<void, IScript::Request&, bool, STATUS, const String&>());
		virtual ~RemoteProxy();
		virtual void SetEntry(const String& entry);
		virtual void Reconnect(IScript::Request& request);

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		struct ObjectInfo {
			ObjectInfo();
			~ObjectInfo();

			struct Entry {
				std::pair<IScript::Request::Ref, size_t> CallFilter(IScript::Request& request, bool pre) {
					return std::make_pair(IScript::Request::Ref(reinterpret_cast<size_t>((void*)&obj)), index);
				}
				String name;
				IReflect::Param retValue;
				std::vector<IReflect::Param> params;
				IScript::Request::AutoWrapperBase* wrapper;
				TWrapper<std::pair<IScript::Request::Ref, size_t>, IScript::Request&, bool> method;
				IScript::BaseDelegate obj;
				size_t index;
			};

			int64_t refCount;
			std::vector<Entry> collection;
			bool needQuery;
		};

		class Request : public TReflected<Request, IScript::Request> {
		public:
			Request(RemoteProxy& host, ITunnel::Connection* connection, const TWrapper<void, IScript::Request&, bool, STATUS, const String&>& statusHandler);
			virtual ~Request();
			void Attach(ITunnel::Connection* connnection);
			void Reconnect();

		public:
			virtual int GetCount();
			virtual IScript* GetScript();

			virtual void QueryInterface(const TWrapper<void, IScript::Request&, IReflectObject&, const Ref&>& callback, IReflectObject& target, const Ref& g);

			virtual bool Call(const AutoWrapperBase& defer, const Request::Ref& g);
			virtual std::vector<Key> Enumerate();
			virtual TYPE GetCurrentType();
			virtual IScript::Request::Ref Load(const String& script, const String& pathname = String());
			virtual IScript::Request& Push();
			virtual IScript::Request& Pop();
			virtual IScript::Request& CleanupIndex();
			virtual IScript::Request& operator >> (IScript::Request::Arguments&) override;
			virtual IScript::Request& operator >> (Ref&);
			virtual IScript::Request& operator << (const Ref&);
			virtual IScript::Request& operator << (const Nil&);
			virtual IScript::Request& operator << (const BaseDelegate&);
			virtual IScript::Request& operator >> (BaseDelegate&);
			virtual IScript::Request& operator << (const Global&);

			virtual IScript::Request& operator << (const TableStart&);
			virtual IScript::Request& operator >> (TableStart&);
			virtual IScript::Request& operator << (const TableEnd&);
			virtual IScript::Request& operator >> (const TableEnd&);
			virtual IScript::Request& operator << (const ArrayStart&);
			virtual IScript::Request& operator >> (ArrayStart&);
			virtual IScript::Request& operator << (const ArrayEnd&);
			virtual IScript::Request& operator >> (const ArrayEnd&);
			virtual IScript::Request& operator << (const Key&);
			virtual IScript::Request& operator >> (const Key&);
			virtual IScript::Request& operator << (double value);
			virtual IScript::Request& operator >> (double& value);
			virtual IScript::Request& operator << (const String& str);
			virtual IScript::Request& operator >> (String& str);
			virtual IScript::Request& operator << (const char* str);
			virtual IScript::Request& operator >> (const char*& str);
			virtual IScript::Request& operator << (bool b);
			virtual IScript::Request& operator >> (bool& b);

			virtual IScript::Request& operator << (const AutoWrapperBase& wrapper);
			virtual IScript::Request& operator << (int64_t u);
			virtual IScript::Request& operator >> (int64_t& u);

			virtual bool IsValid(const BaseDelegate& d);
			virtual Ref Reference(const Ref& d);
			virtual TYPE GetReferenceType(const Ref& d);
			virtual void Dereference(Ref& ref);
			virtual IScript::Request& MoveVariables(IScript::Request& target, size_t count);

			class Packet : public TReflected<Packet, IReflectObjectComplex> {
			public:
				Packet();
				virtual TObject<IReflect>& operator () (IReflect& reflect) override;
				int64_t object; // 0 for global routines
				int64_t procedure;
				int64_t callback;
				bool response;
				bool deferred;
				std::vector<Variant> vars;
				std::vector<std::pair<int64_t, int64_t> > localDelta;
				std::vector<std::pair<int64_t, int64_t> > remoteDelta;
			};

			void OnConnection(ITunnel::EVENT event);
			void Process();
			void ProcessPacket(Packet& packet);
			void PostPacket(Packet& packet);
			void Run();
			IScript::Request::Ref ReferenceEx(const IScript::BaseDelegate* base);
			void DereferenceEx(IScript::BaseDelegate* base);
			void ApplyDelta(std::map<IScript::Object*, ObjectInfo>& info, const std::vector<std::pair<int64_t, int64_t> >& delta, bool retrieve);
			void QueryObjectInterface(ObjectInfo& objectInfo, const IScript::BaseDelegate& d, const TWrapper<void, IScript::Request&, IReflectObject&, const Ref&>& callback, IReflectObject& target);

		public:
			void RequestNewObject(IScript::Request& request, const String& url);
			void RequestQueryObject(IScript::Request& request, IScript::BaseDelegate base);

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		public:

			virtual void DoLock();
			virtual void UnLock();
			RemoteProxy& host;
			IThread& threadApi;

			std::vector<Variant> buffer;
			String key;
			int idx;
			int initCount;
			int tableLevel;
			ITunnel::Connection* connection;

			ObjectInfo globalRoutines;
			std::set<IScript::Request::AutoWrapperBase*> localCallbacks;
			std::set<IReflectObject*> tempObjects;
			std::map<IScript::Object*, size_t> localObjectRefDelta;
			std::map<IScript::Object*, size_t> remoteObjectRefDelta;
			std::map<IScript::Object*, ObjectInfo> localActiveObjects;
			std::map<IScript::Object*, ObjectInfo> remoteActiveObjects;
			std::set<IScript::BaseDelegate*> localReferences;
			TWrapper<void, IScript::Request&, bool, STATUS, const String&> statusHandler;
			IThread::Lock* requestLock;
			IThread::Event* syncCallEvent;
			ITunnel::Packet state;
			enum { CHUNK_SIZE = 0x1000 };
			MemoryStream stream;

			bool manually;
			int lockCount;
		};

		friend class Request;

		virtual const char* GetFileExt() const;
		virtual IScript* NewScript() const;
		virtual IScript::Request* NewRequest(const String& entry);
		virtual IScript::Request& GetDefaultRequest();

		const TWrapper<IScript::Object*, const String&>& GetObjectCreator() const;
		bool Run();
		virtual void Reset() override;
		void Stop();

	protected:
		const ITunnel::Handler OnConnection(ITunnel::Connection* connection);
		bool ThreadProc(IThread::Thread* thread, size_t context);
		void HandleEvent(ITunnel::EVENT event);

	protected:
		ITunnel& tunnel;
		TWrapper<IScript::Object*, const String&> objectCreator;
		TWrapper<void, IScript::Request&, bool, STATUS, const String&> statusHandler;
		Request defaultRequest;
		ITunnel::Dispatcher* dispatcher;
		std::atomic<IThread::Thread*> dispThread;
		ObjectInfo globalRoutines;
		String entry;
	};
}
