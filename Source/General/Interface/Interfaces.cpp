#include "Interfaces.h"

using namespace PaintsNow;

Interfaces::Interfaces(IArchive& parchive, IAudio& paudio, IDatabase& pdatabase, 
	IFilterBase& passetFilterBase, IFilterBase& paudioFilterBase, IFontBase& pfontBase, IFrame& pframe, IImage& pimage, 
	INetwork& pnetwork, IRandom& prandom, IRender& prender,
	IScript& pscript, IThread& pthread, ITimer& ptimer, ITunnel& ptunnel)
	: archive(parchive), audio(paudio), database(pdatabase), 
	assetFilterBase(passetFilterBase), audioFilterBase(paudioFilterBase), fontBase(pfontBase),
	frame(pframe), image(pimage), network(pnetwork), random(prandom), render(prender), script(pscript),
	thread(pthread), timer(ptimer),
	tunnel(ptunnel) {}