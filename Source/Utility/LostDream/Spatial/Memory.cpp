#include "Memory.h"
#include "../../../Core/Template/TAllocator.h"
#include "../../../Core/System/ThreadPool.h"
#include "../../../Core/Driver/Thread/Pthread/ZThreadPthread.h"

using namespace PaintsNow;
using namespace PaintsNow::NsLostDream;

bool Memory::Initialize() {
	return true;
}

struct_aligned(64) data {
	int32_t values[16];
};

bool Memory::Run(int randomSeed, int length) {
	TShared<TObjectAllocator<data> > trunks = TShared<TObjectAllocator<data> >::From(new TObjectAllocator<data>());
	ZThreadPthread threadApi;
	ThreadPool threadPool(threadApi, 8);

	std::vector<data*> ptrs;
	srand(1);
	for (size_t i = 0; i < 77777; i++) {
		data* p = trunks->New();
		ptrs.emplace_back(p);
		data*& q = ptrs[rand() % ptrs.size()];
		p = q;
		q = nullptr;
		if (p != nullptr) {
			trunks->Delete(p);
		}
	}

	getchar();
	for (size_t n = 0; n < ptrs.size(); n++) {
		data* p = ptrs[n];
		if (p != nullptr) {
			trunks->Delete(p);
		}
	}
	getchar();
	return true;
}

void Memory::Summary() {
}


TObject<IReflect>& Memory::operator ()(IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}