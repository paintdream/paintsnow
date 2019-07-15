#include "Interfaces.h"

using namespace PaintsNow;

Interfaces::Interfaces(IArchive& parchive, IAudio& paudio, IDatabase& pdatabase, 
	IFilterBase& pfilterBase, IFontBase& pfontBase, IFrame& pframe, IImage& pimage, 
	INetwork& pnetwork, IRandom& prandom, IRender& prender,
	IScript& pscript, IScript& pnativeScript, IThread& pthread, ITimer& ptimer, ITunnel& ptunnel)
	: archive(parchive), audio(paudio), database(pdatabase), 
	filterBase(pfilterBase), fontBase(pfontBase),
	frame(pframe), image(pimage), network(pnetwork), random(prandom), render(prender), script(pscript),
	nativeScript(pnativeScript), thread(pthread), timer(ptimer),
	tunnel(ptunnel) {}