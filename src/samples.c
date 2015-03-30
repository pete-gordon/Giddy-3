/*
** Sample data
*/

#include <SDL/SDL.h>
#ifdef _CRAPPED_UP_SDL_MIXER_INSTALL_
#include <SDL/SDL_mixer.h>
#elif defined(__APPLE__)
#include <SDL_mixer/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif

#include "giddy3.h"
#include "samples.h"

extern BOOL audioavailable;
Mix_Chunk *snd[SND_LAST];

int musicvol = (MIX_MAX_VOLUME*5)/6, musicvolopt = 8;
int sfxvol   = (MIX_MAX_VOLUME*5)/6, sfxvolopt = 8;

BOOL loadsounds( void )
{
  int i;
  char mkstr[64];

  for( i=0; i<SND_LAST; i++ ) snd[i] = NULL;

  if( !audioavailable ) return FALSE;

  Mix_AllocateChannels( C_LAST );

  for( i=0; i<SND_LAST; i++ )
  {
    sprintf( mkstr, "hats/fx%02x.wav", i+1 );
    if( !( snd[i] = Mix_LoadWAV( mkstr ) ) )
      return FALSE;
  }

  return TRUE;
}

void freesounds( void )
{
  int i;

  if( !audioavailable ) return;

  for( i=0; i<SND_LAST; i++ )
    if( snd[i] ) Mix_FreeChunk( snd[i] );
}

int lastsound=-1;

void playsound( int chan, int sound, int volume )
{
  if( !audioavailable ) return;
  if( !snd[sound] ) return;
  if( volume < 1 ) return;

  lastsound = sound;
  Mix_Volume( chan, volume );
  Mix_PlayChannel( chan, snd[sound], 0 );
}

void loopsound( int chan, int sound, int volume )
{
  lastsound = sound;
  if( !audioavailable ) return;
  if( !snd[sound] ) return;
  if( volume < 1 )
    return;

  Mix_Volume( chan, volume );
  Mix_PlayChannel( chan, snd[sound], -1 );
}

void nloopsound( int chan, int sound, int volume, int loops )
{
  lastsound = sound;
  if( !audioavailable ) return;
  if( !snd[sound] ) return;
  if( volume < 1 )
    return;

  Mix_Volume( chan, volume );
  Mix_PlayChannel( chan, snd[sound], loops );
}

static int amsc = 0;
static int *amchp[2] = { NULL, NULL };
void ambientloop( int sound, int volume, int *chp )
{
  amsc = (amsc+1)%((C_AMBIENTLAST-C_AMBIENT1)+1);
  if( amchp[amsc] )
    *amchp[amsc] = -1;
  amchp[amsc] = chp;
  if( chp )
    *chp = C_AMBIENT1+amsc;
  loopsound( C_AMBIENT1+amsc, sound, volume );
}

void stopchannel( int chan )
{
  int i;

  if( !audioavailable ) return;

  for( i=C_AMBIENT1; i<=C_AMBIENTLAST; i++ )
    if( ( chan == i ) && ( amchp[i-C_AMBIENT1] ) ) { *amchp[i-C_AMBIENT1] = -1; amchp[i-C_AMBIENT1] = NULL; }

  Mix_HaltChannel( chan );
}

void stopallchannels( void )
{
  int i;
  for( i=0; i<=(C_AMBIENTLAST-C_AMBIENT1); i++ )
    if( amchp[i] ) { *amchp[i] = -1; amchp[i] = NULL; }

  Mix_HaltChannel( -1 );
}

void setvol( int chan, int volume )
{
  Mix_Volume( chan, volume );
}

static int esc = 0;
void enemysound( int sound, int volume )
{
  esc = (esc+1)%((C_ENEMYLAST-C_ENEMY1)+1);
  playsound( C_ENEMY1+esc, sound, volume );
}

static int asc = 0;
void actionsound( int sound, int volume )
{
  asc = (asc+1)%((C_ACTIONLAST-C_ACTION1)+1);
  playsound( C_ACTION1+asc, sound, volume );
}

void lpactionsound( int sound, int volume )
{
  int j;
  for( j=0; j<=(C_ACTIONLAST-C_ACTION1); j++ )
  {
    asc = (asc+1)%((C_ACTIONLAST-C_ACTION1)+1);
    if( Mix_Playing( C_ACTION1+asc ) == 0 )
      break;
  }

  if( j > (C_ACTIONLAST-C_ACTION1) ) return;
  playsound( C_ACTION1+asc, sound, volume );
}

static int isc = 0;
void incidentalsound( int sound, int volume )
{
  isc = (isc+1)%((C_INCIDENTALLAST-C_INCIDENTAL1)+1);
  playsound( C_INCIDENTAL1+isc, sound, volume );
}

int incidentalloop( int sound, int volume )
{
  isc = (isc+1)%((C_INCIDENTALLAST-C_INCIDENTAL1)+1);
  loopsound( C_INCIDENTAL1+isc, sound, volume );
  return C_INCIDENTAL1+isc;
}

int incidentalloops( int sound, int volume, int loops )
{
  isc = (isc+1)%((C_INCIDENTALLAST-C_INCIDENTAL1)+1);
  nloopsound( C_INCIDENTAL1+isc, sound, volume, loops );
  return C_INCIDENTAL1+isc;
}
