
#ifndef __GIDDY3_H__
#define __GIDDY3_H__

#include <SDL_opengl.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef WIN32
#ifndef BOOL
typedef SDL_bool BOOL;
#endif
#endif

#define WINWIDTH 640
#define WINHEIGHT 480

#define TARGET_FPS 60
#define FPSTIME (1000/TARGET_FPS)

enum
{
  NOT_ON_TARDIS = 0,
  ON_TARDIS_STEP_OUTSIDE,
  ON_TARDIS_STEP_INSIDE,
  INSIDE_TARDIS
};

struct what_is_giddy_doing
{
  int x, y, px, py;
  Sint16 movedisp;
  int framestep, framedelay;
  int rot;
  int jumpcount, jumping, jumpstep;
  BOOL jump, jumplatch;
  BOOL dazed;
  BOOL usemode;
  BOOL allowjump;
  BOOL nogmxgrav;
  BOOL teleport;
  BOOL flipped;
  BOOL stopuntilfade;
  BOOL stargatewarp;
  BOOL watchwarp;
  int  wwa, wwl;
  float wwob;

  float sgwsc;
  Uint16 def;
  Sint16 mainchryoffset;

  int onspring;

  int coins, lives, energy;
  int onlift, lox, loy;

  int speakxo, speakw, speakh;
  float speakfw, speakfh;
  int speakc, speakstate;
  float speaksc, speaka;

  int flashing, dy;
  float redang;
  BOOL dieanim, spoonedit;
  int spooneditstate, spooneditfade, spooneditwait;
  float spooneditwobblefactor, spooneditwobble;

  BOOL fallpuffs;
  int ontardis;

  int lockx, locky;

  BOOL hardhat;
  BOOL savedtheday;
};


struct what_is_everyone_else_doing
{
  BOOL bossmode;
  int  bossmodeleft, bossmoderight;
  int  quake;
};

#define THB_BLOCKFALL 0
#define THF_BLOCKFALL (1L<<THB_BLOCKFALL)
#define THB_BLOCKWALK 1
#define THF_BLOCKWALK (1L<<THB_BLOCKWALK)
#define THB_BLOCKJUMP 2
#define THF_BLOCKJUMP (1L<<THB_BLOCKJUMP)
#define THB_COLLECTABLE 3
#define THF_COLLECTABLE (1L<<THB_COLLECTABLE)
#define THB_BEHIND 4
#define THF_BEHIND (1L<<THB_BEHIND)
#define THB_CENTREPOS 5
#define THF_CENTREPOS (1L<<THB_CENTREPOS)
#define THB_HOP 6
#define THF_HOP (1L<<THB_HOP)
#define THB_FLASH 7
#define THF_FLASH (1L<<THB_FLASH)
#define THB_CONVEYLEFT 8
#define THF_CONVEYLEFT (1L<<THB_CONVEYLEFT)
#define THB_CONVEYRIGHT 9
#define THF_CONVEYRIGHT (1L<<THB_CONVEYRIGHT)
#define THB_DEADLY 10
#define THF_DEADLY (1L<<THB_DEADLY)
#define THB_AFTERENEMIES 11
#define THF_AFTERENEMIES (1L<<THB_AFTERENEMIES)

struct thingy
{
  // Set
  int ix, iy;
  int numframes, frame, frametime;
  int frames[10];
  BOOL iactive;
  BOOL flipped;
  Uint32 flags;

  // Calculated
  int x, y;
  int framecount;
  BOOL active;
};

#define MAX_THINGIES 256

struct infotext
{
  // Set
  int x, y, w, h;
  BOOL iactive;
  char *txt;

  // Calculated
  BOOL active;
};

enum
{
  ST1_TREE_1UP = 0,
  ST1_SLUG_MOVE,
  ST1_SLUG_BATHING,
  ST1_COLIN_CHEERED_UP,
  ST1_HOSE_PLACED,
  ST1_BOSS_BEATEN,
  ST1_BOOT_COLLECTED,
  ST1_BOMB_DUG,
  ST1_BOMB_COLLECTED,

  ST2_JCS_BOOTED,
  ST2_SLUDGE_RELEASED,
  ST2_PIPE_SEALED,
  ST2_RECYCLOTRON_REPAIRED,
  ST2_CAMERA_COLLECTED,
  ST2_PLUG_GRABBED,
  ST2_PHOTO_PRINTED,
  ST2_PHOTO_COLLECTED,

  ST3_BALLOON_POPPED,
  ST3_GIVEN_BALLOON,
  ST3_PLANK_PLACED,
  ST3_SEESAW_SEESAWED,
  ST3_BLOCKS_FALLEN,
  ST3_TORCHES_LIT,
  ST3_DRAGON_HONKED,
  ST3_DRAGON_DRUNK,
  ST3_CANDLE_LIT,
  ST3_CANDLE_COLLECTED,
  ST3_DIAMOND_PLACED,
  ST3_GUM_BOUGHT,
  ST3_GUM_COLLECTED,

  ST4_PHONEBOX_FIXED,
  ST4_DETONATOR_PLACED,
  ST4_FACTORY_BLOWN,
  ST4_ATEAM_CALLED,
  ST4_CARBON_PLACED,
  ST4_DIAMOND_COLLECTED,
  ST4_POLE_1UP,

  ST5_BATTERY_PLACED,
  ST5_PHOTO_TAKEN,
  ST5_CCTV_DISABLED,
  ST5_GREEN_PRESSED,
  ST5_RED_PRESSED,
  ST5_BLUE_PRESSED,
  ST5_INDIGESTION_CURED,
  ST5_BATTERY_CHARGED,
  ST5_BATTERY_COLLECTED,
  ST5_MIRROR_PLACED,
  ST5_BOMB_PLACED,

  ST_LAST
};

enum
{
  INV_COINS = 0,
  INV_AIRHORN,
  INV_BARREL,
  INV_CD,
  INV_HOSEPIPE,
  INV_CATAPULT,
  INV_SPADE,
  INV_LARD,
  INV_BUBBLEGUM,
  INV_CONTROLBOX,
  INV_CAMERAWITHPHOTO,
  INV_LARGECOG,
  INV_BOOT,
  INV_PLANK,
  INV_SCISSORS,
  INV_TURPS,
  INV_CANDLESTICK,
  INV_DIAMOND,
  INV_LIGHTEDCANDLE,
  INV_BALLOONWRECKAGE,
  INV_LUMPOFCARBON,
  INV_ELECTRICALTOOLKIT,
  INV_ATEAMPHONENO,
  INV_DETONATOR,
  INV_HARDHAT,
  INV_MIRROR,
  INV_INDIGESTIONPILLS,
  INV_CHARGEDBATTERY,
  INV_FLATBATTERY,
  INV_DIGITALCAMERA,
  INV_PRINTEDPHOTO,
  INV_BOMB,
  INV_TELEPORTERWATCH,
  INV_SCOTCHMIST,
  INV_LAST
};

enum
{
  WAWD_TITLES=0,
  WAWD_MENU,
  WAWD_DEFINE_A_KEY,
  WAWD_GAME,
  WAWD_ENDING
};


void save_options( void );
void load_options( void );

#ifdef __APPLE__
char *query_resource_directory( void );
#endif

#endif  // ___GIDDY3_H__
