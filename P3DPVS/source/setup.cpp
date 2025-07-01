#include "../include/common.h"
#include "../include/setup.h"


#ifdef GBA
    #include <gba.h>
    #include <gba_input.h>
    #include <gba_timers.h>

    #define REG_WAITCNT	*((vu16 *)(0x4000204))
#endif

Setup::Setup()
{

}

void Setup::DoSetup()
{
#ifdef GBA
    irqInit();

    //Set gamepak wait states and prefetch.
    REG_WAITCNT = 0x46DA;

    SetMode(MODE_4 | BG2_ENABLE | BIT(5));
#else

#endif
}
