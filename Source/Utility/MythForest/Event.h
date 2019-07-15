// Event.h
// By PaintDream (paintdream@paintdream.com)
// 2015-9-12
//

#ifndef __EVENT_H__
#define __EVENT_H__

#include "../../Core/System/Tiny.h"
#include "Engine.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Event {
		public:
			enum EVENT_ID {
				EVENT_PRETICK,
				EVENT_TICK,
				EVENT_POSTTICK,
				EVENT_FRAME,
				EVENT_FRAME_SYNC_TICK,
				EVENT_INPUT,
				// TACH events
				EVENT_ATTACH_COMPONENT,
				EVENT_DETACH_COMPONENT,
				EVENT_ENTITY_ACTIVATE,
				EVENT_ENTITY_DEACTIVATE,
				EVENT_CUSTOM,
				EVENT_END
			};

			static void ReflectEventIDs(IReflect& reflect) {
				if (reflect.IsReflectEnum()) {
					ReflectEnum(EVENT_PRETICK);
					ReflectEnum(EVENT_TICK);
					ReflectEnum(EVENT_POSTTICK);
					ReflectEnum(EVENT_FRAME);
					ReflectEnum(EVENT_FRAME_SYNC_TICK);
					ReflectEnum(EVENT_INPUT);
					ReflectEnum(EVENT_ATTACH_COMPONENT);
					ReflectEnum(EVENT_DETACH_COMPONENT);
					ReflectEnum(EVENT_ENTITY_ACTIVATE);
					ReflectEnum(EVENT_ENTITY_DEACTIVATE);
					ReflectEnum(EVENT_CUSTOM);
					ReflectEnum(EVENT_END);
				}
			}

#if defined(_MSC_VER) && _MSC_VER <= 1200
			Event();
#endif
			Event(Engine& engine, EVENT_ID id, TShared<SharedTiny> sender, TShared<SharedTiny> detail = nullptr, const String& meta = "");

			std::reference_wrapper<Engine> engine;	// (0/0)
			EVENT_ID eventID;						
			String eventMeta;
			TShared<SharedTiny> eventSender;		
			TShared<SharedTiny> eventDetail;		
		};
	}
}


#endif // __EVENT_H__