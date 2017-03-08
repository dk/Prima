#include "apricot.h"
#include "Region.h"
#include <Region.inc>

#ifdef __cplusplus
extern "C" {
#endif

#undef  my
#define inherited CComponent
#define my  ((( PRegion) self)-> self)
#define var (( PRegion) self)

void
Region_init( Handle self, HV * profile)
{
	dPROFILE;
	inherited-> init( self, profile);
	CORE_INIT_TRANSIENT(Region);
}

#ifdef __cplusplus
}
#endif
