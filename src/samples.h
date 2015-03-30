/*
** Sample data
*/

// Sound channel allocation
enum
{
  C_ACTION1=1,    // User/giddy actions
  C_ACTION2,
  C_ACTIONLAST,
  C_ENEMY1,
  C_ENEMYLAST,
  C_AMBIENT1,
  C_AMBIENTLAST,
  C_INCIDENTAL1,
  C_INCIDENTALLAST,
  C_COIN,
  C_LAST
};

enum
{
  SND_JUMP = 0,
  SND_SPRINKLER,
  SND_WCLICK,               // eggsterminator boot rise end & arm move end
  SND_FX04,
  SND_HUZZAH,
  SND_PULLEYDROP,
  SND_AIRHORN,
  SND_SPRING,                // seesaw
  SND_EXPLOS,                // also weather balloon landing, boss damage
  SND_USEFAIL,
  SND_VANHONK,
  SND_WEATHERBALLOONPOP,
  SND_DRIPHIT,
  SND_CHOMP,
  SND_GIDDYHIT,
  SND_FX10,
  SND_WHOOSH,                // Bamboo spikes, scissor use, sliding blocks, ninja
  SND_BUTTON,                // stone timey thing
  SND_POOP,
  SND_PLANTSHOOT,
  SND_BIRDIE,
  SND_FX16,
  SND_FX17,
  SND_LASERSHOOT,
  SND_GIDDYSTEP1,
  SND_GIDDYSTEP2,
  SND_ITEMGET,
  SND_CRUSHERHIT,          // also mr. t's magnets
  SND_FX1D,
  SND_FX1E,
  SND_CLICKYCLICK,         // Camera, fan buttons
  SND_PRINTER,
  SND_BIGBANG,             // factory, bomb
  SND_SLUDGEMONSTERDIE,
  SND_TARDIS,
  SND_STONEMOVE,
  SND_JUNKCHUTE,
  SND_LASERHITWALL,
  SND_1UP,
  SND_SPACEFROGSHOOT,
  SND_POWERDOOR,
  SND_GIDDYHITECHO,
  SND_FIXEDLASERFIRE,
  SND_CYCLINGALIEN,
  SND_ROPELIFT,
  SND_FIREBALL,
  SND_FX2F,
  SND_WCLICK2,             // grabber grab, eggsterminator head drop end
  SND_SPUDBOSSLAND,        // also block land after seesaw, also barrel land
  SND_RECYCLOTRON,
  SND_TELEPORT_OUT,
  SND_USE,
  SND_DRAGONFLAME,
  SND_SPUDBOSSDIE,
  SND_EXPLOS2,
  SND_STEAM,              // also car crusher
  SND_TOXICGASVENTMEDO,
  SND_LIFTLOOP,
  SND_TELEPORT_IN,
  SND_FX3C,
  SND_BURSTPIPE,
  SND_BUBBLEPOP,
  SND_TANKSHOOT,
  SND_LOSELIFE,
  SND_MRSNAPPY,
  SND_LASERBARRIER,
  SND_BIGASSFAN,
  SND_FX44,
  SND_FX45,
  SND_WASP,
  SND_COIN,
  SND_BINLIFT,
  SND_SPEECHBUB,
  SND_BILE,
  SND_LIFTSTOP,
  SND_LAST
};

BOOL loadsounds( void );
void freesounds( void );
void playsound( int chan, int sound, int volume );
void enemysound( int sound, int volume );
void actionsound( int sound, int volume );
void lpactionsound( int sound, int volume );
void incidentalsound( int sound, int volume );
int incidentalloop( int sound, int volume );
int incidentalloops( int sound, int volume, int loops );
void ambientloop( int sound, int volume, int *chp );
void loopsound( int chan, int sound, int volume );
void stopchannel( int chan );
void stopallchannels( void );
void setvol( int chan, int volume );

