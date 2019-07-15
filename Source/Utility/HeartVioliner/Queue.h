// Queue.h
// By PaintDream (paintdream@paintdream.com)
// 2015-1-4
//

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/ITimer.h"
#include "../../Core/Template/TFactory.h"
#include "../../Core/System/Kernel.h"
#include "../../Core/Interface/IType.h"
#include "Clock.h"
#include <queue>
#include <list>

namespace PaintsNow {
	namespace NsHeartVioliner {
		class Queue : public TReflected<Queue, WarpTiny>, public TaskRepeat {
		public:
			Queue();
			void Attach(TShared<Clock> clock);
			void Detach();

			void Listen(IScript::Request& request, const IScript::Request::Ref& listener);
			void Push(IScript::Request& request, IScript::Request::Ref& ref, int64_t timeStamp);
			virtual void ScriptUninitialize(IScript::Request& request);
			void Clear(IScript::Request& request);
			virtual void Execute(void* context) override;
			virtual void Abort(void* context) override;
			void ExecuteWithTimeStamp(IScript::Request& request, int64_t timeStamp);

		protected:
			void Post(IScript::Request& request, IScript::Request::Ref ref, int64_t timeStamp);

		private:
			struct Task {
				Task(const IScript::Request::Ref& r, int64_t t) : timeStamp(t), ref(r) {}
				int64_t timeStamp;
				IScript::Request::Ref ref;
				bool operator < (const Task& task) const {
					return timeStamp > task.timeStamp;
				}
			};

			std::priority_queue<Task> q;
			std::list<IScript::Request::Ref> listeners;
			TShared<Clock> clock;
		};
	}
}


#endif // __QUEUE_H__
