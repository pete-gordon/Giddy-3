
#ifndef __RENDER_H__
#define __RENDER_H__

enum
{
  BLOCKTEX=0,
  GSPRITEX,
  SPRITEX,
  PSPRITEX,
  WGSPRITEX,
  WSPRITEX,
  WPSPRITEX,
  TVTEX,
  TEXTTEX,
  SCREENTEX,
  TTEXTTEX,
  TEX_LAST
};

struct btex
{
  float x;
  int fx;
  float y;
  int fy;
  float w;
  float hw;
  float hwx;
  int fw;
  int hfw;
  float h;
  float hh;
  float hhx;
  int fh;
  int hfh;
};

#define MAX_STARS 64
struct star
{
  int x, y;
  int dx, dy;
  int framecount, framespeed, frame;
};

enum
{
  LT_RETURN = 0,
  LT_BUBBLE,
  LT_TRIGGER,
  LT_FRETURN,
  LT_BTRIGGR,
  LT_GIRDER
};

struct lift
{
  // Defined parts
  int numstops;
  int stops[10];
  int speed;
  int wtex, sprite, surface, depth;
  float bob;
  int type;
  BOOL behind;
  int itimeout;
  int dipmax;
  int sndloop, sndstop;

  // Calculated parts
  int timeout;
  int cx, cy, dx, dy, bobo, dip;
  int fromstop, tostop;
  struct btex *s;
  Uint8 *src;
  float scale;
};

struct spring
{
  // Defined parts
  int x, y;
  int wtex;
  int frames[4];
  
  // Calculated parts
  int frame, rtime;
  Uint8 *src;
  struct btex *sl;
};

#define MAX_INCIDENTALS 32
struct incidental
{
  int x, y, xm, ym;
  int numframes, frame;
  int framecount, framespeed;
  int *frames;
  int wtex;
  struct btex *s;
};

struct mapsz
{
  int fgw, fgh;     // Foreground map dimensions
  int bgw, bgh;     // Background map dimensions
  int xwrap, ywrap;
  int yoff;
  int xdiv, ydiv;

  int imapx, imapy;
  int igidx, igidy;

  int splitpos, sply;
  int yscrolltoplimit;
};


struct invtext
{
  char *use;
  char *pickup;
};


BOOL render_init( void );
void render_shut( void );
Uint32 timing( Uint32 interval, void *dummy );
BOOL render( void );

void render_sprite_scaled( struct btex *tx, int x, int y, BOOL flipx, float rot, float scale );
void render_sprite( struct btex *tx, int x, int y, BOOL flipx, float rot );
void render_sprite_a( struct btex *tx, int x, int y, BOOL flipx, float rot, int alpha );
void render_sprite_tl( struct btex *tx, int x, int y, BOOL flipx );
void render_sprite_tl_clipy( struct btex *tx, int x, int y, int clipy, BOOL flipx );
void render_sprite_tl_stretch( struct btex *tx, int x, int y, float sw, float sh, BOOL flipx );
void render_sprite_offs( struct btex *tx, int x, int y, int xo, int yo, BOOL flipx, float rot );
void render_sprite_tl_clipyb( struct btex *tx, int x, int y, int clipy, BOOL flipx );
BOOL startincidental( int x, int y, int xm, int ym, int wtex, int *frames, int numframes, int speed );
BOOL startbgincidental( int x, int y, int xm, int ym, int wtex, int *frames, int numframes, int speed );
void giddyhit( void );
void thingy_bounds( struct thingy *t, struct btex *sl, int *left, int *top, int *right, int *bot );
void giddy_say( char *what );
void render_sprite_scaleda( struct btex *tx, int x, int y, BOOL flipx, float rot, float scale, int alpha );
void you_now_have( char *what );
void render_foreground_zone( int fx, int fy, int fw, int fh );
void render_tvborders( void );
BOOL load_sprites( char *filename );
void render_fade( void );
BOOL start_game( void );
BOOL load_globalsprites( void );

#endif // __RENDER_H__
