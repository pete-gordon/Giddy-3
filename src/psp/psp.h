// ********************************************
// *** PSP specific stuff for Giddy3        ***
// ********************************************
// stingray, 22-04-2009
// $Id: psp.h 65 2009-04-22 20:18:48Z sting $

#ifndef PSP_H_INCLUDED
#define PSP_H_INCLUDED

#include <pspkernel.h>
#include <psppower.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <psprtc.h>

#include <math.h>
#include <string.h>


#define printf	pspDebugScreenPrintf
//#define PSP_SHOWFPS


PSP_MODULE_INFO("GIDDY3 PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR( THREAD_ATTR_USER );
PSP_HEAP_SIZE_KB( 16*1024 );


void    psp_init( );
void    psp_initfps( );
void    psp_showfps( );
void    psp_readmouse( );
void    psp_drawmouse( );
int     psp_SetupCallbacks( );
void    psp_main( );

void
psp_init( )
{
// setup callback so we can quit with 'HOME'
    psp_SetupCallbacks( );


// burn baby, burn (set pll/cpu/busspeed to 333/333/166 MHz)
//    scePowerSetClockFrequency( 333, 333, 166 );

    pspDebugScreenInit( );

#ifndef __NO_MOUSE__
    sceCtrlSetSamplingCycle( 0 );
    sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );
#endif
    pspDebugScreenSetXY( 0, 10 );
    printf( "Giddy 3 PSP, ported by StingRay" );
}

void
psp_main( )
{

#ifdef PSP_SHOWFPS
    psp_showfps( );
#endif

// no dimming of the backlight
    sceKernelPowerTick( 0 );

}


void
psp_showfps( )
{
    static u8 init = 0;
    static u32 res;
    static u64 lasttick, currtick;
    static u32 framecount = 0;
    static int fps = 0;

    if ( init == 0 )
    {
        sceRtcGetCurrentTick( &lasttick );
        res = sceRtcGetTickResolution( );
        init++;
    }

    framecount++;
    sceRtcGetCurrentTick( &currtick );

    if( ((currtick - lasttick)/((float)res)) >= 1.0f )
    {
        lasttick = currtick;
        fps = framecount;
        framecount = 0;
    }

    pspDebugScreenSetXY( 0, 0 );
    pspDebugScreenPrintf("FPS: %d", fps );
}



// Exit callback
int
psp_exit_callback( int arg1, int arg2, void *common )
{
    sceKernelExitGame();
    return 0;
}

// Callback thread
int
psp_CallbackThread( SceSize args, void *argp )
{
    int cbid;

    cbid = sceKernelCreateCallback("Exit Callback", (void *) psp_exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);

    sceKernelSleepThreadCB();

    return 0;
}

// Sets up the callback thread and returns its thread id
int
psp_SetupCallbacks( )
{
	int thid = 0;

	thid = sceKernelCreateThread( "update_thread", psp_CallbackThread, 0x11, 0xFA0, 0, 0 );
	if( thid >= 0 )
	{
		sceKernelStartThread( thid, 0, 0 );
	}

	return thid;
}

#endif // PSP_H_INCLUDED
