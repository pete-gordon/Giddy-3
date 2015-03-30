
struct enemy
{
  void (*initfunc)(void *);
  void (*animatefunc)(void *);
  void (*renderfunc)(void *);
  void *enemyarray;
};

struct wasp
{
  // Set
  int sx, ex, y;

  // Calculated
  int x, cy;
  int ang;
  BOOL returning;
};

struct spider
{
  // Set
  int x, sy, ey;

  // Calculated
  int y, timeout;
  BOOL returning;
};

struct shroom
{
  // Set
  int sx, ex, y;

  // Calculated
  int x;
  BOOL returning;
};

struct shootyf
{
  // Set
  int x, y;

  // Calculated
  int gy, ang, shoottime;
  BOOL grown, shot;
};

struct mine
{
  // Set
  int x, y;

  // Calculated
  BOOL attached, active;
  int ang, dm, hy;
};

struct fish
{
  // Set
  int sx, ex, y;

  // Calculated
  int x;
  BOOL returning;
};

struct steam
{
  // Set
  int x, y, dx, dy;

  // Calculated
  int count, nexts;
  int sx[4], sy[4];
  float scale[4];
};

struct barrel
{
  // Set
  int sx, sy, resety, itimeout;

  // Calculated
  int x, y, grav;
  int xm, timeout;
};

enum
{
  GRABS_NORMAL = 0,
  GRABS_LOWERING,
  GRABS_GRABBING,
  GRABS_RAISING,
  GRABS_NORMAL_DONTLOWER
};

struct grabber
{
  // Set
  int sx, ex, sy, gty, gby;

  // Calculated
  int x, y, gy, grabstate, gf;
  int timeout;
  BOOL returning;
};

struct minitank
{
  // Set
  int sx, lx, rx, y;

  // Calculated
  int x, shootcount;
  int flashcount;
  BOOL returning;
};

struct crusher
{
  // Set
  int x, sy, ey, itimeout;

  // Calculated
  int y, timeout;
  BOOL returning;
};

struct poop
{
  // Set
  int x, y;

  // Calculated
  BOOL active;
};

struct spike
{
  // Set
  int x, y;

  // Calculated
  int timeout, yo, ya;
};

struct pbird
{
  // Set
  int sx, lx, rx, y;

  // Calculated
  int x, shootcount, frame;
  BOOL returning;
};

struct grabbermagnet
{
  // Set
  int lx, rx, ty, by;

  // Calculated
  int x, y, state;
};

struct spacefrog
{
  // Set
  int x, ty, by;
  BOOL flipped;

  // Calculated
  int y, shootwait;
  BOOL returning;
};

struct zapper
{
  // Set
  int x, y;
  BOOL upzapper;

  // Calculated
  int state, timer;
  BOOL flipped;
};

struct fixedlaser
{
  // Set
  int x, y, w, wait, idelay;

  // Calculated
  int time, f1, f2;
};

struct uppydowny
{
  // Set
  int x, ty, by, sy, type;

  // Calculated
  int y, dy;
};

struct bigdrip
{
  // Set
  int x, sy, ey;

  // Calculated
  int y;
};

struct fireball
{
  // Set
  int x, iy, idy, idelay, bdelay;

  // Calculated
  int y, dy, delay;
};

void initenemies( struct enemy *e );
void animateenemies( struct enemy *e );
void renderenemies( struct enemy *e );

void rendershootyfmissiles( void );

BOOL giddydanger( GLuint wtex, int sprite, int x, int y, BOOL flipped );

int testspritedowncolis( GLuint wtex, int sprite, int x, int y, int *convey, int xb );
