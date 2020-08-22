#include "Zipper.h"
#include "../../Core/Interface/IStreamBase.h"

using namespace PaintsNow;

Zipper::Zipper(IArchive* a, IStreamBase* stream) : archive(a), streamBase(stream) {}
Zipper::~Zipper() {
	archive->ReleaseDevice();
	streamBase->ReleaseObject();
}