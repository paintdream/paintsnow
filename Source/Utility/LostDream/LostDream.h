// LostDream.h - Simple unit testing
// By PaintDream (paintdream@paintdream.com)
// 2017-12-30
//

#ifndef __LOSTDREAM_H__
#define __LOSTDREAM_H__

#include "../../Core/Template/TFactory.h"
#include "../../Core/Interface/IReflect.h"
#include <list>

namespace PaintsNow {
	namespace NsLostDream {
		class LostDream {
		public:
			class Qualifier : public TReflected<Qualifier, IReflectObjectComplex> {
			public:
				virtual ~Qualifier();
				virtual bool Initialize() = 0;
				virtual bool Run(int randomSeed, int length) = 0;
				virtual void Summary() = 0;
			};

			virtual ~LostDream();
			bool RegisterQualifier(const TFactoryBase<Qualifier>& q, int count);
			bool RunQualifiers(bool stopOnError, int initRandomSeed, int length);

		protected:
			std::list<std::pair<Qualifier*, int> > qualifiers;
		};
	}
}

#endif // __LOSTDREAM_H__