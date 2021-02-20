// BridgeSunset.h -- Asynchronous routine dispatching module for script
// By PaintDream (paintdream@paintdream.com)
// 2015-1-2
//


#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../Core/System/Kernel.h"
#include "../../Core/System/TaskGraph.h"

namespace PaintsNow {
	class BridgeSunset : public TReflected<BridgeSunset, IScript::Library>, public IScript::RequestPool, public ISyncObject {
	public:
		BridgeSunset(IThread& threadApi, IScript& script, uint32_t threadCount, uint32_t warpCount);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		~BridgeSunset() override;
		void ScriptInitialize(IScript::Request& request) override;
		void ScriptUninitialize(IScript::Request& request) override;
		void Dispatch(ITask* task);
		Kernel& GetKernel();
		void ContinueScriptDispatcher(IScript::Request& request, IHost* host, size_t paramCount, const TWrapper<void, IScript::Request&>& continuer);

	protected:
		/// <summary>
		/// Create a new task graph
		/// </summary>
		/// <param name="statupWarp"> startup warp index of graph </param>
		/// <returns> TaskGraph object </returns>
		TShared<TaskGraph> RequestNewGraph(IScript::Request& request, int32_t startupWarp);

		/// <summary>
		/// Queue a new task to an existing task graph
		/// </summary>
		/// <param name="graph"> TaskGraph object </param>
		/// <param name="tiny"> task tiny object </param>
		/// <param name="callback"> task callback </param>
		/// <returns> Queued task id </returns>
		void RequestQueueGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, IScript::Delegate<WarpTiny> tiny, IScript::Request::Ref callback);

		/// <summary>
		/// Set execution dependency between two tasks.
		/// </summary>
		/// <param name="graph"> TaskGraph object </param>
		/// <param name="prev"> pre task </param>
		/// <param name="next"> post task </param>
		/// <returns></returns>
		void RequestConnectGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, int32_t prev, int32_t next);

		/// <summary>
		/// Execute TaskGraph at once
		/// </summary>
		/// <param name="graph"> Execute TaskGraph at once </param>
		/// <returns></returns>
		void RequestExecuteGraph(IScript::Request& request, IScript::Delegate<TaskGraph> graph);

		/// <summary>
		/// Queue a routine based on a tiny
		/// </summary>
		/// <param name="tiny"> the tiny object </param>
		/// <param name="callback"> callback </param>
		/// <returns></returns>
		void RequestQueueRoutine(IScript::Request& request, IScript::Delegate<WarpTiny> tiny, IScript::Request::Ref callback);

		/// <summary>
		/// Get count of warps
		/// </summary>
		/// <returns> The count of warps </returns>
		uint32_t RequestGetWarpCount(IScript::Request& request);

	public:
		ThreadPool threadPool;
		Kernel kernel;
		TWrapper<void, IScript::Request&, IHost*, size_t, const TWrapper<void, IScript::Request&>&> origDispatcher;
	};

#define CHECK_THREAD_IN_LIBRARY(warpTiny) \
	(MUST_CHECK_REFERENCE_ONCE); \
	if (bridgeSunset.GetKernel().GetCurrentWarpIndex() != warpTiny->GetWarpIndex()) { \
		request.Error("Threading routine failed on " #warpTiny); \
		assert(false); \
	}

	template <bool deref>
	class ScriptTaskTemplateBase : public TaskOnce {
	public:
		ScriptTaskTemplateBase(IScript::Request::Ref ref) : callback(ref) {}

		void Abort(void* context) override {
			if (deref) {
				BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

				IScript::Request& req = bridgeSunset.GetScript().GetDefaultRequest();
				req.DoLock();
				req.Dereference(callback);
				req.UnLock();
			}

			TaskOnce::Abort(context);
		}

		IScript::Request::Ref callback;
	};

#if defined(_MSC_VER) && _MSC_VER <= 1200
	template <bool deref>
	class ScriptTaskTemplate : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplate(IScript::Request::Ref ref) : ScriptTaskTemplateBase<deref>(ref) {}
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			req.DoLock();
			req.Push();
			req.Call(sync, callback);
			req.Pop();
			if (deref) {
				req.Dereference(callback);
			}
			req.UnLock();

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}
	};

	inline ITask* CreateTaskScript(IScript::Request::Ref ref) {
		return new ScriptTaskTemplate<false>(ref);
	}

	inline ITask* CreateTaskScriptOnce(IScript::Request::Ref ref) {
		return new ScriptTaskTemplate<true>(ref);
	}

	template <bool deref, class A>
	class ScriptTaskTemplateA : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateA(IScript::Request::Ref ref, const A& a) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			req.DoLock();
			req.Push();
			req.Call(sync, callback, pa);
			req.Pop();
			if (deref) {
				req.Dereference(callback);
			}
			req.UnLock();


			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}

		typename std::decay<A>::type pa;
	};

	template <class A>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a) {
		return new ScriptTaskTemplateA<false, A>(ref, a);
	}

	template <class A>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a) {
		return new ScriptTaskTemplateA<true, A>(ref, a);
	}

	template <bool deref, class A, class B>
	class ScriptTaskTemplateB : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateB(IScript::Request::Ref ref, const A& a, const B& b) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			req.DoLock();
			req.Push();
			req.Call(sync, callback, pa, pb);
			req.Pop();
			if (deref) {
				req.Dereference(callback);
			}
			req.UnLock();


			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
	};

	template <class A, class B>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b) {
		return new ScriptTaskTemplateB<false, A, B>(ref, a, b);
	}

	template <class A, class B>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b) {
		return new ScriptTaskTemplateB<true, A, B>(ref, a, b);
	}

	template <bool deref, class A, class B, class C>
	class ScriptTaskTemplateC : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateC(IScript::Request::Ref ref, const A& a, const B& b, const C& c) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			req.DoLock();
			req.Push();
			req.Call(sync, callback, pa, pb, pc);
			req.Pop();
			if (deref) {
				req.Dereference(callback);
			}
			req.UnLock();

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
	};

	template <class A, class B, class C>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c) {
		return new ScriptTaskTemplateC<false, A, B, C>(ref, a, b, c);
	}

	template <class A, class B, class C>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c) {
		return new ScriptTaskTemplateC<true, A, B, C>(ref, a, b, c);
	}

	template <bool deref, class A, class B, class C, class D>
	class ScriptTaskTemplateD : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateD(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			req.DoLock();
			req.Push();
			req.Call(sync, callback, pa, pb, pc, pd);
			req.Pop();
			if (deref) {
				req.Dereference(callback);
			}
			req.UnLock();

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
	};

	template <class A, class B, class C, class D>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d) {
		return new ScriptTaskTemplateD<false, A, B, C, D>(ref, a, b, c, d);
	}

	template <class A, class B, class C, class D>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d) {
		return new ScriptTaskTemplateD<true, A, B, C, D>(ref, a, b, c, d);
	}

	template <bool deref, class A, class B, class C, class D, class E>
	class ScriptTaskTemplateE : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateE(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			req.DoLock();
			req.Push();
			req.Call(sync, callback, pa, pb, pc, pd, pe);
			req.Pop();
			if (deref) {
				req.Dereference(callback);
			}
			req.UnLock();

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
	};

	template <class A, class B, class C, class D, class E>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
		return new ScriptTaskTemplateE<false, A, B, C, D, E>(ref, a, b, c, d, e);
	}

	template <class A, class B, class C, class D, class E>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
		return new ScriptTaskTemplateE<true, A, B, C, D, E>(ref, a, b, c, d, e);
	}

	template <bool deref, class A, class B, class C, class D, class E, class F>
	class ScriptTaskTemplateF : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateF(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) : ScriptTaskTemplateBase<deref>(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			req.DoLock();
			req.Push();
			req.Call(sync, callback, pa, pb, pc, pd, pe, pf);
			req.Pop();
			if (deref) {
				req.Dereference(callback);
			}
			req.UnLock();

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
		typename std::decay<F>::type pf;
	};

	template <class A, class B, class C, class D, class E, class F>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
		return new ScriptTaskTemplateF<false, A, B, C, D, E, F>(ref, a, b, c, d, e, f);
	}

	template <class A, class B, class C, class D, class E, class F>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
		return new ScriptTaskTemplateF<true, A, B, C, D, E, F>(ref, a, b, c, d, e, f);
	}

	template <bool deref, class A, class B, class C, class D, class E, class F, class G>
	class ScriptTaskTemplateG : public ScriptTaskTemplateBase<deref> {
	public:
		ScriptTaskTemplateG(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) : ScriptTaskTemplateBase<deref>(ref) {
			pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); pg = const_cast<G&>(g);
		}
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			req.DoLock();
			req.Push();
			req.Call(sync, callback, pa, pb, pc, pd, pe, pf, pg);
			req.Pop();
			if (deref) {
				req.Dereference(callback);
			}
			req.UnLock();

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
		typename std::decay<F>::type pf;
		typename std::decay<G>::type pg;
	};

	template <class A, class B, class C, class D, class E, class F, class G>
	ITask* CreateTaskScript(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
		return new ScriptTaskTemplateG<false, A, B, C, D, E, F, G>(ref, a, b, c, d, e, f, g);
	}

	template <class A, class B, class C, class D, class E, class F, class G>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
		return new ScriptTaskTemplateG<true, A, B, C, D, E, F, G>(ref, a, b, c, d, e, f, g);
	}

	// Routine tasks

	template <class T>
	class ScriptHandlerTemplate : public TaskOnce {
	public:
		ScriptHandlerTemplate(T ref) : callback(ref) {}
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			callback(req);

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}

		T callback;
	};

	template <class T>
	ITask* CreateTaskScriptHandler(T ref) {
		return new ScriptHandlerTemplate(ref);
	}

	template <class T, class A>
	class ScriptHandlerTemplateA : public TaskOnce {
	public:
		ScriptHandlerTemplateA(T ref, const A& a) : callback(ref) { pa = const_cast<A&>(a); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			callback(req, pa);

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}

		T callback;
		typename std::decay<A>::type pa;
	};

	template <class T, class A>
	ITask* CreateTaskScriptHandler(T ref, const A& a) {
		return new ScriptHandlerTemplateA<T, A>(ref, a);
	}

	template <class T, class A, class B>
	class ScriptHandlerTemplateB : public TaskOnce {
	public:
		ScriptHandlerTemplateB(T ref, const A& a, const B& b) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			callback(req, pa, pb);

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
	};

	template <class T, class A, class B>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b) {
		return new ScriptHandlerTemplateB<T, A, B>(ref, a, b);
	}

	template <class T, class A, class B, class C>
	class ScriptHandlerTemplateC : public TaskOnce {
	public:
		ScriptHandlerTemplateC(T ref, const A& a, const B& b, const C& c) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			callback(req, pa, pb, pc);

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
	};

	template <class T, class A, class B, class C>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c) {
		return new ScriptHandlerTemplateC<T, A, B, C>(ref, a, b, c);
	}

	template <class T, class A, class B, class C, class D>
	class ScriptHandlerTemplateD : public TaskOnce {
	public:
		ScriptHandlerTemplateD(T ref, const A& a, const B& b, const C& c, const D& d) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			callback(pa, pb, pc, pd);

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
	};

	template <class T, class A, class B, class C, class D>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c, const D& d) {
		return new ScriptHandlerTemplateD<T, A, B, C, D>(ref, a, b, c, d);
	}

	template <class T, class A, class B, class C, class D, class E>
	class ScriptHandlerTemplateE : public TaskOnce {
	public:
		ScriptHandlerTemplateE(T ref, const A& a, const B& b, const C& c, const D& d, const E& e) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			callback(req, pa, pb, pc, pd, pe);

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
	};

	template <class T, class A, class B, class C, class D, class E>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c, const D& d, const E& e) {
		return new ScriptHandlerTemplateE<T, A, B, C, D, E>(ref, a, b, c, d, e);
	}

	template <class T, class A, class B, class C, class D, class E, class F>
	class ScriptHandlerTemplateF : public TaskOnce {
	public:
		ScriptHandlerTemplateF(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) : callback(ref) { pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = f; }
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			callback(req, pa, pb, pc, pd, pe, pf);

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
		typename std::decay<F>::type pf;
	};

	template <class T, class A, class B, class C, class D, class E, class F>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) {
		return new ScriptHandlerTemplateF<T, A, B, C, D, E, F>(ref, a, b, c, d, e, f);
	}

	template <class T, class A, class B, class C, class D, class E, class F, class G>
	class ScriptHandlerTemplateG : public TaskOnce {
	public:
		ScriptHandlerTemplateG(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) : callback(ref) {
			pa = const_cast<A&>(a); pb = const_cast<B&>(b); pc = const_cast<C&>(c); pd = const_cast<D&>(d); pe = const_cast<E&>(e); pf = const_cast<F&>(f); pg = const_cast<G&>(g);
		}
		virtual void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);

			IScript::Request& req = *bridgeSunset.AcquireSafe();
			callback(req, pa, pb, pc, pd, pe, pf, pg);

			bridgeSunset.ReleaseSafe(&req);
			delete this;
		}

		T callback;
		typename std::decay<A>::type pa;
		typename std::decay<B>::type pb;
		typename std::decay<C>::type pc;
		typename std::decay<D>::type pd;
		typename std::decay<E>::type pe;
		typename std::decay<F>::type pf;
		typename std::decay<F>::type pg;
	};

	template <class T, class A, class B, class C, class D, class E, class F, class G>
	ITask* CreateTaskScriptHandler(T ref, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g) {
		return new ScriptHandlerTemplateG<T, A, B, C, D, E, F, G>(ref, a, b, c, d, e, f, g);
	}

#else
	template <bool deref, typename... Args>
	class ScriptTaskTemplate : public ScriptTaskTemplateBase<deref> {
	public:
		typedef ScriptTaskTemplateBase<deref> BaseClass;
		template <typename... Params>
		ScriptTaskTemplate(IScript::Request::Ref c, Params&&... params) : BaseClass(c), arguments(std::forward<Params>(params)...) {}

		template <typename T, size_t index>
		struct Writer {
			void operator () (IScript::Request& request, T& arg) {
				request << std::get<std::tuple_size<T>::value - index>(arg);
				Writer<T, index - 1>()(request, arg);
			}
		};

		template <typename T>
		struct Writer<T, 0> {
			void operator () (IScript::Request& request, T& arg) {}
		};

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& req = *bridgeSunset.AcquireSafe();
			req.DoLock();
			req.Push();
			Writer<decltype(arguments), sizeof...(Args)>()(req, arguments);
			req.Call(sync, BaseClass::callback);
			if (deref) {
				req.Dereference(BaseClass::callback);
			}
			req.Pop();
			req.UnLock();
			bridgeSunset.ReleaseSafe(&req);

			delete this;
		}

		std::tuple<typename std::decay<Args>::type...> arguments;
	};

	template <typename... Args>
	ITask* CreateTaskScript(IScript::Request::Ref ref, Args&&... args) {
		return new ScriptTaskTemplate<false, Args...>(ref, std::forward<Args>(args)...);
	}

	template <typename... Args>
	ITask* CreateTaskScriptOnce(IScript::Request::Ref ref, Args&&... args) {
		return new ScriptTaskTemplate<true, Args...>(ref, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	class ScriptHandlerTemplate : public TaskOnce {
	public:
		template <typename... Params>
		ScriptHandlerTemplate(T t, Params&&... params) : callback(t), arguments(std::forward<Params>(params)...) {}

		template <size_t... S>
		void Apply(IScript::Request& context, seq<S...>) {
			callback(context, std::move(std::get<S>(arguments))...);
		}

		void Execute(void* context) override {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& req = *bridgeSunset.AcquireSafe();
			Apply(req, gen_seq<sizeof...(Args)>());
			bridgeSunset.ReleaseSafe(&req);

			delete this;
		}

		T callback;
		std::tuple<typename std::decay<Args>::type...> arguments;
	};

	template <typename T, typename... Args>
	ITask* CreateTaskScriptHandler(T t, Args&&... args) {
		return new ScriptHandlerTemplate<T, Args...>(t, std::forward<Args>(args)...);
	}

#endif		
}
