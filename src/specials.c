
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <SDL/SDL.h>
#ifdef _CRAPPED_UP_SDL_MIXER_INSTALL_
#include <SDL/SDL_mixer.h>
#elif defined(__APPLE__)
#include <SDL_mixer/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif

#ifdef HAVE_GLES
#include <GLES/gl.h>
#include <EGL/egl.h>
#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#endif

#include "giddy3.h"
#include "render.h"
#include "specials.h"
#include "enemies.h"
#include "samples.h"

#ifdef __amigaos4__
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/picasso96api.h>
//extern BOOL screenbodge;
//BOOL bodgeneedupdate = FALSE;
//extern struct Window *win;
//struct RenderInfo sbrinf;
#endif

#ifdef HAVE_GLES
// GLES defines to handle batched GL_QUADS drawing
#define GLES_SIZE	8192
extern GLfloat gles_vtx[3*GLES_SIZE];
extern GLfloat gles_tex[2*GLES_SIZE];
extern GLfloat gles_col[4*GLES_SIZE];
extern GLfloat curcol[4];
extern GLushort indices[6*GLES_SIZE];
extern int idx, ids;
extern BOOL gles_drawing;
extern BOOL gles_color;
extern GLenum what;
#define glOrtho		glOrthof
#define GL_QUADS	0x0007
#define ub2f		(1.0f/255.0f)
#define glBegin(a) \
	gles_drawing = TRUE; \
	gles_color = FALSE; \
	what = a; \
	ids = 0; \
	idx = 0

#define glEnd() \
	gles_drawing = FALSE; \
	glEnableClientState(GL_VERTEX_ARRAY); \
	glEnableClientState(GL_TEXTURE_COORD_ARRAY); \
	if (gles_color) glEnableClientState(GL_COLOR_ARRAY); \
	glVertexPointer(3, GL_FLOAT, 0, gles_vtx); \
	glTexCoordPointer(2, GL_FLOAT, 0, gles_tex); \
	glColorPointer(4, GL_FLOAT, 0, gles_col); \
	if (what==GL_QUADS) \
		glDrawElements(GL_TRIANGLES, ids, GL_UNSIGNED_SHORT, indices); \
	else \
		glDrawArrays(what, 0, idx); \
	glDisableClientState(GL_VERTEX_ARRAY); \
	glDisableClientState(GL_TEXTURE_COORD_ARRAY); \
	if (gles_color) glDisableClientState(GL_COLOR_ARRAY)

#define glColor4ub(r,g,b,a)	if (gles_drawing) {\
		curcol[0]=r*ub2f; curcol[1]=g*ub2f; curcol[2]=b*ub2f; curcol[3]=a*ub2f; \
		gles_color = TRUE; \
	} else { \
		glColor4f(r*ub2f, g*ub2f, b*ub2f, a*ub2f); \
	}

#define glVertex3f(a,b,c)	\
		gles_vtx[idx*3+0]=a; gles_vtx[idx*3+1]=b; gles_vtx[idx*3+2]=c; \
		if (gles_color) memcpy(gles_col+idx*4, curcol, 4*sizeof(GLfloat)); \
		idx++; \
		if (what==GL_QUADS) if (idx%4==0)  { \
			indices[ids++]=idx-4; indices[ids++]=idx-3; indices[ids++]=idx-2; \
			indices[ids++]=idx-2; indices[ids++]=idx-1; indices[ids++]=idx-4; \
		}

#define glTexCoord2f(a,b)	gles_tex[idx*2+0]=a; gles_tex[idx*2+1]=b
	
#endif

extern int sfxvol;
extern Uint8 screentex[];
extern Uint8 strig[];
extern Uint8 inv[];
extern Uint32 frame;
extern struct btex psprt1[], psprt2[], sprt2[], sprtg[], psprt4[], sprt4[], psprt3[], sprt3[], psprt5[], sprt5[], sprt1[];
extern GLuint tex[];
extern int bangframes[];
extern int fgx, fgy, bgx, bgy;
extern Uint8 fgmap[];
extern struct mapsz *mapi;
extern struct infotext infos1[], infos2[], infos3[], infos4[], infos5[];
extern struct what_is_giddy_doing gid;
extern struct what_is_everyone_else_doing stuff;
extern struct thingy p_things2[], n_things2[], p_things3[], p_things5[], n_things3[], n_things1[];
extern struct thingy p_things4[], n_things4[], g_things3[], g_things4[], g_things2[], p_things1[];
extern int winfo, clevel, llevel, fadea, fadeadd, fadetype;
extern struct btex bt[];
extern struct invtext ivtexts[];

void set_blocker( int x, int y, int w, int h, int block )
{
  int cx, cy;
  for( cy=0; cy<h; cy++ )
    for( cx=0; cx<w; cx++ )
      fgmap[(cy+y)*mapi->fgw+(cx+x)] = block;
}

/*************************** SLUG ****************************/

int slugx, slugy, slugyadd, slugstate;
int slugframe, slugspeak;
BOOL slugon;

int beery, beerbc;
int beerframes[] = { 13, 14, 15, 16 };

void initslug( void )
{
  beerbc = 0;
  if( strig[ST1_SLUG_BATHING] )
  {
    // Slug is already bathing
    slugx = 5044;
    slugy = 327<<8;
    slugstate = 4;
    slugspeak = 0;
    slugyadd = 0;
    slugframe = 3;
    beery = 372;
    set_blocker( 322, 18, 1, 4, 0 );
    infos1[3].active = FALSE;
    return;
  }

  set_blocker( 322, 18, 1, 4, 255 );

  if( strig[ST1_SLUG_MOVE] )
  {
    // Slug has already moved;
    slugx = 5152;
    slugy = 304<<8;
    slugstate = 1;
    slugspeak = 0;
    beery = 390;
    slugyadd = 0;
    infos1[3].active = TRUE;
    return;
  }

  slugx = 5152 + 64;
  slugy = 304<<8;
  slugstate = 0;
  slugyadd = 0;
  beery = 390;
  infos1[3].active = TRUE;
}

BOOL triggerslug( void )
{
  if( !slugon ) return FALSE;

  strig[ST1_SLUG_BATHING] = 1;
  inv[INV_BARREL] = 0;
  infos1[3].active = FALSE;
  actionsound( SND_HUZZAH, sfxvol );
  return TRUE;
}

void animateslug( void )
{
  // Need to reset slugface?
  if( ( strig[ST1_SLUG_MOVE] ) &&
      ( !strig[ST1_SLUG_BATHING] ) &&
      ( gid.px < (5152-480) ) )
  {
    slugx = 5152 + 64;
    slugy = 304<<8;
    slugstate = 0;
    slugyadd = 0;
    strig[ST1_SLUG_MOVE] = 0;
    return;
  }

  switch( slugstate )
  {
    case 0:  // Slug is waiting to move, or is moving
      slugframe = ((frame>>3)%3)+4;
      if( strig[ST1_SLUG_MOVE] )
      {
        if( slugx > 5152 )
        {
          slugx--;
        } else {
          slugstate++;
          slugspeak = 60;
        }
      }
      break;

    case 1: // Slug has stopped and needs beer!
      slugframe = 4;
      if( strig[ST1_SLUG_BATHING] )
        slugstate++;
      break;
    
    case 2: // Beer has been used, pond is filling
      if( beery > 372 )
      {
        if( frame&2 ) beery--;
        break;
      }
      slugstate++; 
      slugy = 296<<8;
      slugyadd = -(3<<8);
      slugframe = 3;
      set_blocker( 322, 18, 1, 4, 0 );
      break;

    case 3: // Pond is full, slug is jumping
      if( slugx > 5044 )
        slugx-=3;

      if( slugx < 5138 )
      {
        slugy += slugyadd;
        slugyadd += 1<<6;
        if( slugyadd > (8<<8) ) slugyadd = (8<<8);
        if( slugy > (327<<8) )
        {
          slugy = 327<<8;
          slugyadd = 0;
          slugstate++;
          strig[ST1_SLUG_BATHING] = 1;
          slugspeak = 60;
        }
      }
      break;
    
    case 4: // Slug is bathing, everyone is happy.
      break;
  }
}

void renderbeer( void )
{
  if( beery >= 390 ) return;

  if( ( 5024 > (fgx+320) ) ||
      ( (5024+96) < fgx ) ||
      ( beery >= (fgy+240) ) ||
      ( (beery+29) < fgy ) )
    return;

  render_sprite_tl( &psprt1[36], 5024-fgx, beery-fgy, FALSE );

  if( beerbc > 0 )
  {
    beerbc--;
    return;
  }

  if( beery <= 372 )
    startincidental( (rand()%74)+5040, (rand()%10)+372, 0, -2, PSPRITEX, beerframes, 4, 3 );
  beerbc = (rand()%5)+3;
}

void renderslug( void )
{
  int te;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  if( slugframe != 3 )
  {
    if( slugspeak > 0 )
      te = 277;
    else
      te = 240;

    if( ( slugx >= (fgx+342) ) ||
        ( slugx  < (fgx-79) ) ||
        ( slugy >= ((fgy+te)<<8) ) ||
        ( slugy  < ((fgy-53)<<8) ) )
    {
      slugon = FALSE;
      return;
    }

    slugon = TRUE;
    strig[ST1_SLUG_MOVE] = 1;
    render_sprite_tl( &psprt1[slugframe], slugx-fgx, (slugy>>8)-fgy, FALSE );
    render_sprite_tl( &psprt1[7], slugx+29-fgx, (slugy>>8)+17-fgy, FALSE );

    if( slugspeak > 0 )
    {
      slugspeak--;
      render_sprite_tl( &psprt1[32], (slugx-22)-fgx, ((slugy>>8)-37)-fgy, FALSE );
    }

    renderbeer();
    return;
  }

  render_sprite_tl( &psprt1[3], slugx-fgx, (slugy>>8)-fgy, FALSE );

  if( slugspeak > 0 )
  {
    slugspeak--;
    render_sprite_tl( &psprt1[37], (slugx-22)-fgx, ((slugy>>8)-37)-fgy, FALSE );
  }
  renderbeer();
}

/*********************** CONGER EEL **************************/

int eelx, eely, eelface, eelstate;
int eelwait;
float eela, eelscale;

void initeel( void )
{
  if( strig[ST1_COLIN_CHEERED_UP] )
  {
    eelx = 3700;
    infos1[2].active = FALSE;
    set_blocker( 292, 54, 1, 4, 0 );
    return;
  }

  eelstate = 0;
  eelscale = 1.0f;
  eelx     = 4654;
  eely     = 898;
  eelface  = 1;
  eela     = 0.0f;
  infos1[2].active = TRUE;
  set_blocker( 292, 54, 1, 4, 255 );
}

void animateeel( void )
{
  eela += 0.05f;
  while( eela > (3.14159265f*2.0f) ) eela -= (3.14159265f*2.0f);

  if( !strig[ST1_COLIN_CHEERED_UP] )
    return;

  if( eelx < 3800 )
    return;
  
  if( eelwait > 0)
  {
    eelwait--;
    if( eelwait == 0 )
    {
      giddy_say( "Nah, yer alright mate." );
      set_blocker( 292, 54, 1, 4, 0 );
    }
    return;
  }
  eelx -= 4;
  if( eelx < 4416 )
  {
    if( eelscale > 0.0f )
    {
      eelscale -= 0.08f;
      if( eelscale < 0.0f )
        eelscale = 0.0f;
    }
  }
}

void rendereel( void )
{
  int i;
  int xp, yp;
  float ang;

  if( eelx < 3800 )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );

  xp = eelx + 5*24+28;
  ang = eela;
  for( i=6; i>=0; i-- )
  {
    yp = eely - ((int)(sin(ang)*18.0f));
    render_sprite_scaled( &psprt1[i==0?eelface:2], xp-fgx, yp-fgy, FALSE, 0.0f, eelscale );
    ang -= 0.37f;
    if( i==1 )
      xp -= 28;
    else
      xp -= 24;
  }

  if( ( strig[ST1_COLIN_CHEERED_UP] ) &&
      ( eelwait > 0 ) )
    render_sprite_tl( &psprt1[28], (eelx-32)-fgx, (eely-64)-fgy, FALSE );
}

BOOL triggereel( void )
{
  if( ( clevel != 1 ) || ( winfo != 2 ) )
    return FALSE;

  strig[ST1_COLIN_CHEERED_UP] = 1;
  inv[INV_CD] = 0;
  eelwait = 180;
  eelface = 0;
  infos1[2].active = FALSE;
  actionsound( SND_HUZZAH, sfxvol );
  return TRUE;
}

/*********************** JUNK CHUTE SWITCHEROO ***************/
struct junkitem
{
  int x, y, dy;
  struct btex *s;
  float ang, anga;
};
#define MAX_JUNK 32
struct junkitem junk[MAX_JUNK];

void initjunkchute( void )
{
  int i, j;

  for( i=0; i<MAX_JUNK; i++ )
  {
    j = (rand()%6)+36;
    junk[i].s = &psprt2[j];
    junk[i].x = (rand()%(96-junk[i].s->fw))+3360+junk[i].s->hfw;
    junk[i].y = (rand()%288)+832;
    junk[i].dy = (rand()%3)+3;
    junk[i].ang = 0.0f;
    junk[i].anga = (float)((rand()%10)-5);
  }

  if( strig[ST2_JCS_BOOTED] )
  {
    p_things2[23].active = FALSE;   // Computer says yes
    n_things2[0].frames[0] = 22;
    infos2[1].active = FALSE;
    infos2[4].active = FALSE;
    set_blocker( 214, 59, 1, 7, 0 );
    set_blocker( 210, 68, 6, 1, 14 );
    set_blocker( 210, 69, 6, 1, 18 );
    return;
  }

  p_things2[23].active = TRUE;   // Computer says no
  n_things2[0].frames[0] = 21;
  infos2[1].active = TRUE;
  infos2[4].active = TRUE;
  set_blocker( 214, 59, 1, 7, 255 );
  set_blocker( 210, 68, 6, 1, 0 );
  set_blocker( 210, 69, 6, 1, 0 );
}

void animatejunkchute( void )
{
  int i, j;

  // Is the junk chute on?
  if( strig[ST2_JCS_BOOTED] )
    return;

  // Do it!
  for( i=0; i<MAX_JUNK; i++ )
  {
    junk[i].y += junk[i].dy;
    junk[i].ang += junk[i].anga;
    if( junk[i].y > 1200 )
    {
      j = (rand()%6)+36;
      junk[i].s = &psprt2[j];
      junk[i].x = (rand()%(96-junk[i].s->fw))+3360+junk[i].s->hfw;
      junk[i].y = 832;
      junk[i].dy = (rand()%3)+3;
      junk[i].ang = 0.0f;
      junk[i].anga = (float)((rand()%10)-5);
    }
  }
}

void renderjunkchute( void )
{
  int i;

  // Is it visible?
  if( ( fgy < 628 ) ||
      ( fgy > 1152 ) ||
      ( fgx > 3504 ) ||
      ( (fgx+320) < 3312 ) )
    return;

  // Is the junk chute on?
  if( strig[ST2_JCS_BOOTED] )
    return;

  if( (rand()%50) == 3 )
    incidentalsound( SND_JUNKCHUTE, sfxvol );

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; i<MAX_JUNK; i++ )
    render_sprite( junk[i].s, junk[i].x-fgx, junk[i].y-fgy, FALSE, junk[i].ang );
}

BOOL triggerjunkchuteswitcheroo( void )
{
  if( ( clevel != 2 ) || ( winfo != 1 ) )
    return FALSE;

  strig[ST2_JCS_BOOTED] = 1;
  inv[INV_BOOT] = 0;
  initjunkchute();   // Re-init
  giddy_say( "Huzzah!" );
  actionsound( SND_HUZZAH, sfxvol );
  return TRUE;
}

/*********************** PLUG GRABBER ************************/

int pgr_graby, pgr_plugy, pgr_watery, pgr_state, pgr_frame;

void initpluggrabber( void )
{
  if( strig[ST2_PLUG_GRABBED] )
  {
    pgr_plugy = 1460;
    pgr_graby = 1424;
    pgr_watery = 1932;
    pgr_state = 0;
    pgr_frame = 21;
    p_things2[106].active = TRUE;
    infos2[0].active = FALSE;
    set_blocker( 200, 94, 8, 2, 0 );
    set_blocker( 203, 96, 2, 1, 0 );
    pgr_state = 4;
    return;
  }

  pgr_plugy = 1536;
  pgr_graby = 1424;
  pgr_watery = 1488;
  pgr_state = 0;
  pgr_frame = 22;
  p_things2[106].active = FALSE;
  infos2[0].active = TRUE;
  set_blocker( 203, 96, 2, 1, 255 );
  set_blocker( 200, 94, 8, 2, 203 );
}

void animatepluggrabber( void )
{
  switch( pgr_state )
  {
    case 1:
      pgr_graby++;
      if( pgr_graby >= 1500 )
      {
        pgr_graby = 1500;
        pgr_frame = 21;
        pgr_state++;
      }
      break;
    
    case 2:
      pgr_graby--;
      if( pgr_graby <= 1424 )
      {
        pgr_graby = 1424;
        pgr_state++;
        break;
      }

      if( pgr_plugy < 1528 )
      {
        set_blocker( 203, 96, 2, 1, 0 );
        pgr_watery += 7;
      }

      pgr_plugy--;
      break;
    
    case 3:
      pgr_watery += 7;
      if( pgr_watery >= 1932 )
        pgr_state++;
      break;
  }
}

BOOL triggerpluggrabber( void )
{
  if( ( clevel != 2 ) || ( winfo != 0 ) )
    return FALSE;

  strig[ST2_PLUG_GRABBED] = 1;
  p_things2[106].active = TRUE;
  infos2[0].active = FALSE;
  inv[INV_CONTROLBOX] = 0;
  set_blocker( 200, 94, 8, 2, 0 );
  actionsound( SND_HUZZAH, sfxvol );
  pgr_state = 1;
  return TRUE;
}

void renderpluggrabberbg( void )
{
  if( pgr_watery >= 1932 )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  render_sprite_tl( &sprt2[27], 3200-fgx, pgr_watery-fgy, FALSE );

  glBindTexture( GL_TEXTURE_2D, tex[BLOCKTEX] );
  glLoadIdentity();
  glColor4ub(255, 255, 255, 255);
  glBegin( GL_QUADS );
    glTexCoord2f( 176.0f/256.0f, 192.0f/256.0f ); glVertex3f( 3200-fgx, pgr_watery+16-fgy, 0.0f );
    glTexCoord2f( 192.0f/256.0f, 192.0f/256.0f ); glVertex3f( 3328-fgx, pgr_watery+16-fgy, 0.0f );
    glTexCoord2f( 192.0f/256.0f, 208.0f/256.0f ); glVertex3f( 3328-fgx, pgr_watery+48-fgy, 0.0f );
    glTexCoord2f( 176.0f/256.0f, 208.0f/256.0f ); glVertex3f( 3200-fgx, pgr_watery+48-fgy, 0.0f );
  glEnd();
}

void renderpluggrabber( void )
{
  struct btex *tx;
  float ch, sw;

  tx = &psprt2[19];
  sw = tx->hwx;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  ch = (float)(pgr_graby-1406)/2.0f;
  if( ch > 9.0f )
  {
    glLoadIdentity();
    glTranslatef( 3264-fgx, 1406.0f + ch - fgy, 0.0f );
    glBegin( GL_QUADS );
      glColor4ub(255, 255, 255, 255);
      glTexCoord2f( tx->x,       tx->y       ); glVertex3f( -sw, -ch, 0.0f );
      glTexCoord2f( tx->x+tx->w, tx->y       ); glVertex3f(  sw, -ch, 0.0f );
      glTexCoord2f( tx->x+tx->w, tx->y+tx->h ); glVertex3f(  sw,  ch, 0.0f );
      glTexCoord2f( tx->x,       tx->y+tx->h ); glVertex3f( -sw,  ch, 0.0f );
    glEnd();
  }
  render_sprite( &psprt2[18], 3264-fgx, 1406-fgy,  FALSE, 0.0f );
  render_sprite( &psprt2[20], 3264-fgx, pgr_graby-fgy, FALSE, 0.0f );
  render_sprite( &psprt2[pgr_frame], 3264-fgx, pgr_graby+24-fgy, FALSE, 0.0f );

  if( ( (3264-19)      >= (fgx+336) ) ||
      ( (3264+19)      <  (fgx-16) ) ||
      ( (pgr_plugy+13) >= (fgy+256) ) ||
      ( (pgr_plugy-13) <  (fgy-16) ) )
    return;

  render_sprite( &psprt2[62], 3264-fgx, pgr_plugy-fgy, FALSE, FALSE );
}

/*********************** RECYCLOTRON 2000 ********************/

int rcbx, rcby;
int stuffdelay[7], stuffloop[7];
BOOL stufflooped;
struct thingy *stuffs[] = { &p_things2[78], &p_things2[79], &p_things2[80], &g_things2[7], &p_things2[81], &p_things2[82], &p_things2[83] };

int rcsndchan=-1, rcsndvol=0;

void initrecyclotron( void )
{
  int i;
  rcbx = 452;
  rcby = 760;

  rcsndchan = -1;
  rcsndvol = 0;

  if( strig[ST2_RECYCLOTRON_REPAIRED] )
  {
    infos2[2].active = FALSE;
    g_things2[7].active = !strig[ST2_CAMERA_COLLECTED];
    g_things2[7].x = 280;
    p_things2[78].active = TRUE;
    p_things2[78].x = 280;
    p_things2[79].active = TRUE;
    p_things2[79].x = 280;
    p_things2[80].active = TRUE;
    p_things2[80].x = 280;
    p_things2[81].active = TRUE;
    p_things2[81].x = 280;
    p_things2[82].active = TRUE;
    p_things2[82].x = 280;
    p_things2[83].active = TRUE;
    p_things2[83].x = 280;
    for( i=0; i<7; i++ )
    {
      stuffdelay[i] = i * 80;
      stuffloop[i] = strig[ST2_CAMERA_COLLECTED] ? -200 : -280;
    }
    stufflooped = strig[ST2_CAMERA_COLLECTED];
    return;
  }

  infos2[2].active = TRUE;
  g_things2[7].active = FALSE;
  p_things2[78].active = FALSE;
  p_things2[79].active = FALSE;
  p_things2[80].active = FALSE;
  p_things2[81].active = FALSE;
  p_things2[82].active = FALSE;
  p_things2[83].active = FALSE;
  stufflooped = FALSE;
}

void animaterecyclotron( void )
{
  int i;

  if( !strig[ST2_RECYCLOTRON_REPAIRED] )
  {
    if( rcsndvol != -1 )
      stopchannel( rcsndvol );
    return;
  }

  i = rcsndvol;
  rcsndvol = 1000-abs(gid.px-310)*2;
  if( rcsndvol < 0 ) rcsndvol = 0;
  if( ( gid.y < (928<<8) ) ||
      ( gid.y > (1088<<8) ) )
    rcsndvol = 0;

  rcsndvol = (rcsndvol*sfxvol)/1000;

  if( rcsndvol > 0 )
  {
    if( rcsndchan == -1 )
    {
      ambientloop( SND_RECYCLOTRON, rcsndvol, &rcsndchan );
    } else {
      if( i != rcsndvol ) setvol( rcsndchan, rcsndvol );
    }
  } else {
    if( rcsndchan != -1 )
      stopchannel( rcsndchan );
  }

//  if( strig[ST2_CAMERA_COLLECTED] )
//    ep = -200;
//  else
//    ep = -280;

  for( i=0; i<7; i++ )
  {
    if( !stuffs[i]->active )
      continue;

    if( stuffdelay[i] > 0 )
    {
      stuffdelay[i]--;
      continue;
    }

    if( stuffs[i]->x > stuffloop[i] )
    {
      stuffs[i]->x--;
      continue;
    }

    stuffs[i]->x = 280;
    if( ( stufflooped ) && ( stuffloop[i] == -280 ) )
    {
      stuffloop[i] = -200;
      continue;
    }

    if( ( i == 0 ) &&
        ( stuffloop[i] == -280 ) &&
        ( strig[ST2_CAMERA_COLLECTED] ) )
    {
      stuffloop[0] = -200;
      stuffloop[4] = -200;
      stuffloop[5] = -200;
      stuffloop[6] = -200;
      stufflooped = TRUE;
    }
  }

  if( rcby < 1012 )
  {
    rcby += 4;
    if( rcby > 1012 )
      rcby = 1012;
    return;
  }

  if( rcbx > 280 )
  {
    rcbx--;
    return;
  }

  rcbx = 452;
  rcby = 760;
}

void renderrecyclotron( void )
{
  float vl, vt, vr, vb;
  int fa, fb;

  vl =  280.0f - (float)fgx;
  vt = 1016.0f - (float)fgy;
  vr =  360.0f - (float)fgx;
  vb = 1040.0f - (float)fgy;

  if( strig[ST2_RECYCLOTRON_REPAIRED] )
  {
    glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
    render_sprite_tl( &psprt2[17], rcbx-fgx, rcby-fgy, FALSE );
    fa = 43 + ((frame>>2)&3);
    fb = 46 - ((frame>>2)&3);
  } else {
    fa = 43;
    fb = 43;
  }

  glBindTexture( GL_TEXTURE_2D, tex[GSPRITEX] );
  glLoadIdentity();
  glBegin( GL_QUADS );
    glColor4ub( 255, 255, 255, 255 );
    glTexCoord2f(         0.0f,         0.0f ); glVertex3f( vl, vt, 0.0f );
    glTexCoord2f( 56.0f/256.0f,         0.0f ); glVertex3f( vr, vt, 0.0f );
    glTexCoord2f( 56.0f/256.0f, 62.0f/256.0f ); glVertex3f( vr, vb, 0.0f );
    glTexCoord2f(         0.0f, 62.0f/256.0f ); glVertex3f( vl, vb, 0.0f );
  glEnd();
  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite( &psprt2[fa], 288-fgx, 1020-fgy, FALSE, 0.0f );
  render_sprite( &psprt2[fb], 310-fgx, 1034-fgy, FALSE, 0.0f );
  render_sprite( &psprt2[fb], 351-fgx, 1034-fgy, FALSE, 0.0f );
  if( strig[ST2_RECYCLOTRON_REPAIRED] )
    render_sprite( &psprt2[fa], 330-fgx, 1020-fgy, FALSE, 0.0f );
}

BOOL triggerrecyclotron( void )
{
  int i;

  if( ( clevel != 2 ) || ( winfo != 2 ) )
    return FALSE;

  inv[INV_LARGECOG] = 0;
  strig[ST2_RECYCLOTRON_REPAIRED] = 1;
  infos2[2].active = FALSE;

  g_things2[7].active = TRUE;
  g_things2[7].x = 280;
  p_things2[78].active = TRUE;
  p_things2[78].x = 280;
  p_things2[79].active = TRUE;
  p_things2[79].x = 280;
  p_things2[80].active = TRUE;
  p_things2[80].x = 280;
  p_things2[81].active = TRUE;
  p_things2[81].x = 280;
  p_things2[82].active = TRUE;
  p_things2[82].x = 280;
  p_things2[83].active = TRUE;
  p_things2[83].x = 280;
  for( i=0; i<7; i++ )
  {
    stuffdelay[i] = i * 80;
    stuffloop[i] = -280;
  }
  stufflooped = FALSE;
  actionsound( SND_HUZZAH, sfxvol );
  return TRUE;
}

/*********************** SLUDGEMONSTER ********************/

int sludgevalve, sludgy, sludgyadd, smonsx, smonsy, smonsyadd;
int smonsreturning;
float smonsbob;

void initsludgemonster( void )
{
  smonsx = 2000<<8;
  smonsreturning = TRUE;
  smonsbob = 0.0f;

  if( strig[ST2_SLUDGE_RELEASED] )
  {
    sludgevalve = 58;
    sludgy = 628<<8;
    smonsy = 666<<8;
    sludgyadd = 0;
    set_blocker( 122, 32, 1, 4, 0 );
    infos2[3].active = FALSE;
    p_things2[72].flags &= ~THF_DEADLY;
    p_things2[73].flags &= ~THF_DEADLY;
    p_things2[74].flags &= ~THF_DEADLY;
    p_things2[72].y = 610;
    p_things2[73].y = 610;
    p_things2[74].y = 610;
    return;
  }

  sludgevalve = 57;
  sludgy = 592<<8;
  smonsy = 564<<8;
  sludgyadd = 0;
  set_blocker( 122, 32, 1, 4, 255 );
  infos2[3].active = TRUE;
  p_things2[72].flags |= THF_DEADLY;
  p_things2[73].flags |= THF_DEADLY;
  p_things2[74].flags |= THF_DEADLY;
}

void animatesludgemonster( void )
{
  smonsbob += 0.06f;
  if( smonsbob > 6.2831853f ) smonsbob -= 6.2831853f;
  if( sludgyadd != 0 )
  {
    sludgy += sludgyadd;
    if( sludgy >= (628<<8) )
    {
      sludgy = (628<<8);
      sludgyadd = 0;
    }

    p_things2[72].y = (sludgy>>8)-18;
    p_things2[73].y = (sludgy>>8)-18;
    p_things2[74].y = (sludgy>>8)-18;
  }

  if( smonsyadd != 0 )
  {
    smonsy += smonsyadd;
    if( smonsy >= (666<<8) )
      smonsyadd = 0;
  }

  if( ( smonsy < (666<<8) ) && ( smonsyadd == 0 ) )
  {
    if( smonsreturning )
    {
      smonsx -= 0x80;
      if( smonsx < (1960<<8) )
      {
        smonsx = (1960<<8);
        smonsreturning = FALSE;
      }
    } else {
      smonsx += 0x80;
      if( smonsx > (2048<<8) )
      {
        smonsx = (2048<<8);
        smonsreturning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 24, smonsx>>8, smonsy>>8, !smonsreturning ) )
      giddyhit();
  }
}

void rendersludgemonster( void )
{
  if( smonsy >= (666<<8) )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  if( smonsyadd != 0 )
  {
    render_sprite( &sprt2[((frame>>3)&1)+24], (smonsx>>8)-fgx, (smonsy>>8)-fgy, !smonsreturning, 0.0f );
  } else {
    render_sprite( &sprt2[25], (smonsx>>8)-fgx, (smonsy>>8)+(sin(smonsbob)*5.0f)-fgy, !smonsreturning, 0.0f );
  }
}

BOOL triggersludgemonster( void )
{
  if( ( clevel != 2 ) || ( winfo != 3 ) )
    return FALSE;

  strig[ST2_SLUDGE_RELEASED] = 1;
  inv[INV_LARD] = FALSE;
  set_blocker( 122, 32, 1, 4, 0 );
  sludgevalve = 58;
  sludgyadd = 0x40;
  smonsyadd = 0x80;
  infos2[3].active = FALSE;
  p_things2[72].flags &= ~THF_DEADLY;
  p_things2[73].flags &= ~THF_DEADLY;
  p_things2[74].flags &= ~THF_DEADLY;
  giddy_say( "Huzzah!" );
  actionsound( SND_HUZZAH, sfxvol );
  incidentalsound( SND_SLUDGEMONSTERDIE, sfxvol );
  return TRUE;
}

void rendersludge( void )
{
  int x, y, px, py;
  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );

  for( y=0, py=(sludgy>>8)-fgy; y<2; y++, py+=16 )
  {
    for( x=0, px=1936-fgx; x<2; x++, px+=48 )
      render_sprite_tl_clipy( &psprt2[51], px, py, 626-fgy, FALSE );
    render_sprite_tl_clipy( &psprt2[56], px, py, 626-fgy, FALSE );
  }

  // Release valve
  render_sprite_tl( &psprt2[sludgevalve], 1868-fgx, 586-fgy, FALSE );
}

/*********************** BURST PIPE  **********************/

struct bpsteam
{
  int x, y, ys;
  float sc;
};
#define NUM_BSTEAM 12
struct bpsteam bstm[NUM_BSTEAM];

int bpsndchan = -1, bpsndvol = 0;

void initburstpipe( void )
{
  int i;
  for( i=0; i<NUM_BSTEAM; i++ )
    bstm[i].x = -i*3;

  if( strig[ST2_PIPE_SEALED] )
  {
    infos2[5].active = FALSE;
    set_blocker( 50, 5, 1, 4, 0 );
    return;
  }

  infos2[5].active = TRUE;
  set_blocker( 50, 5, 1, 4, 255 );
}

void animateburstpipe( void )
{
  int i;

  i = bpsndvol;
  if( strig[ST2_PIPE_SEALED] )
  {
    bpsndvol = 0;
  } else {
    bpsndvol = 1000-abs(gid.px-808)*2;
    if( bpsndvol < 0 ) bpsndvol = 0;
    if( fgy > 190 ) bpsndvol = 0;

    bpsndvol = (bpsndvol*sfxvol)/1000;
  }

  if( bpsndvol > 0 )
  {
    if( bpsndchan == -1 )
    {
      ambientloop( SND_BURSTPIPE, bpsndvol, &bpsndchan );
    } else {
      if( i != bpsndvol ) setvol( bpsndchan, bpsndvol );
    }
  } else {
    if( bpsndchan != -1 )
      stopchannel( bpsndchan );
  }

  for( i=0; i<NUM_BSTEAM; i++ )
  {
    if( bstm[i].x < 0 )
    {
      bstm[i].x++;
      continue;
    }

    if( bstm[i].x == 0 )
    {
      if( strig[ST2_PIPE_SEALED] )
        continue;

      bstm[i].x = 808;
      bstm[i].y = 146;
      bstm[i].ys = (rand()%3)+3;
      bstm[i].sc = 0.2f;
      continue;
    }

    bstm[i].y -= bstm[i].ys;
    bstm[i].sc += 0.06f;
    if( bstm[i].y <= 72 )
      bstm[i].x = -(rand()%3);
  }
}

void renderburstpipe( void )
{
  int i;

  if( ( 800  < (fgx+336) ) &&
      ( 816 >= (fgx-16) ) &&
      ( 144  < (fgy+256) ) &&
      ( 160 >= (fgy-16) ) )
  {
    if( strig[ST2_PIPE_SEALED] )
      render_sprite_tl( &psprt2[60], 800-fgx, 142-fgy, FALSE );
    else
      render_sprite_tl( &psprt2[59], 800-fgx, 144-fgy, FALSE );
  }

  glBindTexture( GL_TEXTURE_2D, tex[GSPRITEX] );
  for( i=0; i<NUM_BSTEAM; i++ )
  {
    if( bstm[i].x > 0 )
      render_sprite_scaleda( &sprtg[23], bstm[i].x-fgx, bstm[i].y-fgy, FALSE, 0.0f, bstm[i].sc, 160 );
  }
}

BOOL triggerburstpipe( void )
{
  if( ( clevel != 2 ) || ( winfo != 5 ) )
    return FALSE;

  strig[ST2_PIPE_SEALED] = 1;
  inv[INV_BUBBLEGUM] = FALSE;
  infos2[5].active = FALSE;
  set_blocker( 50, 5, 1, 4, 0 );
  actionsound( SND_HUZZAH, sfxvol );
  giddy_say( "Huzzah!" );

  return TRUE;
}

/*********************** TOXIC GAS ************************/

struct toxgas
{
  int x, y;
  float ang, anga, sc;
};
#define NUM_TGAS 64
struct toxgas tgas[NUM_TGAS];

int tgasstate, tgastimer, tgasgotcha;

void inittoxicgas( void )
{
  int i;

  tgasstate = 0;
  tgastimer = 120;
  tgasgotcha = 0;

  for( i=0; i<NUM_TGAS; i++ )
    tgas[i].x = -((i*3)%96);
}

void animatetoxicgas( void )
{
  int i;

  if( ( gid.x >= (3648<<8) ) &&
      ( gid.x < (3696<<8) ) &&
      ( gid.y < (688<<8) ) )
    tgasgotcha = 1;

  if( gid.dieanim )
    tgasgotcha = 0;

  if( tgasgotcha )
  {
    if( gid.mainchryoffset < 0x400 )
      gid.mainchryoffset += 0x100;
    if( gid.x < (3660<<8) )
    {
      gid.x += 0x500;
      gid.px += 5;
    }
    if( gid.x >= (3684<<8) )
    {
      gid.x -= 0x500;
      gid.px -= 5;
    }
  }

  if( tgastimer > 0 )
  {
    tgastimer--;
  } else {
    tgasstate ^= 1;
    if( tgasstate)
    {
      tgastimer = 80;
    } else {
      if( ( fgx > 3200 ) &&
          ( fgy < 660 ) )
        incidentalsound( SND_TOXICGASVENTMEDO, sfxvol );
      tgastimer = 120;
    }
  }

  for( i=0; i<NUM_TGAS; i++ )
  {
    if( tgas[i].x < 0 )
    {
      if( tgasstate == 1 ) continue;
      tgas[i].x++;
      continue;
    }

    if( tgas[i].x == 0 )
    {
      if( tgasstate == 0 )
      {
        tgas[i].x = (rand()%32) + 3664;
        tgas[i].y = 688;
        tgas[i].ang = 0.0f;
        tgas[i].anga = (rand()%10)-5;
        tgas[i].sc = 0.3f;
      }
      continue;
    }

    tgas[i].y -= 4;
    tgas[i].sc += 0.02f;
    tgas[i].ang += tgas[i].anga;

    if( giddydanger( PSPRITEX, 61, tgas[i].x, tgas[i].y, FALSE ) )
      giddyhit();

    if( tgas[i].y < 320 )
    {
      if( tgasstate )
        tgas[i].x = -((i*3)%96);
      else
        tgas[i].x = -1;
    }
  }
}

void rendertoxicgas( void )
{
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; i<NUM_TGAS; i++ )
  {
    if( tgas[i].x > 0 )
      render_sprite_scaleda( &psprt2[61], tgas[i].x-fgx, tgas[i].y-fgy, FALSE, tgas[i].ang, tgas[i].sc, 128 );
  }
}

/************************ TARDIS **************************/

int trd_x, trd_y, trd_boostercutoff;
int trd_yadd;

void inittardis( void )
{
  trd_yadd = 0;
  switch( clevel )
  {
    case 3:
      trd_x             = 112<<8;
      trd_y             = 352<<8;
      trd_boostercutoff = 352;
      break;

    case 4:
      trd_x             = 2928<<8;
      trd_y             =  432<<8;
      trd_boostercutoff =  432;
      break;
  }
}

void animatetardis( void )
{
  if( gid.ontardis != INSIDE_TARDIS )
  {
    trd_yadd = 0;
    return;
  }

  trd_yadd -= 0x08;
  if( trd_yadd < -0x400 )
    trd_yadd = -0x400;
  trd_y += trd_yadd;

  if( ( trd_y < (300<<8) ) &&
      ( clevel == llevel ) )
  {
    switch( clevel )
    {
      case 4:  llevel = 3;        break;
      case 3:  llevel = 0x8004;   break;
      default: llevel = clevel+1; break;
    }

    fadea   = 0;
    fadeadd = 8;
    gid.stopuntilfade = TRUE;
  }
}

void rendertardis( void )
{
  int tx, ty, xo;

  tx = (trd_x>>8)-fgx;
  ty = (trd_y>>8)-fgy;

  glBindTexture( GL_TEXTURE_2D, tex[GSPRITEX] );
  if( gid.ontardis != INSIDE_TARDIS )
  {
    render_sprite_tl( &sprtg[55], tx+27, ty-100, FALSE );
  } else {
    int f;
    f = ((frame>>1)&1)+57;
    xo = -sprtg[f].hfw;

    render_sprite_tl( &sprtg[((frame>>4)&1)+55], tx+27, ty-100, FALSE );
    render_sprite_tl_clipy( &sprtg[f], tx+8+xo, ty+8, trd_boostercutoff-fgy, FALSE );
    render_sprite_tl_clipy( &sprtg[f], tx+30+xo, ty+8, trd_boostercutoff-fgy, FALSE );
    render_sprite_tl_clipy( &sprtg[f], tx+48+xo, ty+8, trd_boostercutoff-fgy, FALSE );
  }

  render_sprite_tl( &sprtg[54], tx   , ty- 92, FALSE );
  render_sprite_tl( &sprtg[53], tx+ 2, ty- 73, FALSE );
  render_sprite_tl( &sprtg[60], tx+32, ty- 73, FALSE );
  render_sprite_tl( &sprtg[52], tx+ 2, ty- 44, FALSE );
  render_sprite_tl( &sprtg[59], tx+32, ty- 44, FALSE );
  render_sprite_tl( &sprtg[52], tx+ 2, ty- 28, FALSE );
  render_sprite_tl( &sprtg[59], tx+32, ty- 28, FALSE );
  render_sprite_tl_clipy( &sprtg[8], tx, ty-12, trd_boostercutoff-fgy, FALSE );

  if( gid.ontardis != INSIDE_TARDIS )
  {
    render_sprite_tl_stretch( &sprtg[51], tx+32, ty-75, 19, 67, FALSE );
    render_sprite_tl( &sprtg[61], tx+32, ty- 73, FALSE );
    render_sprite_tl( &sprtg[62], tx+32, ty- 56, FALSE );
    render_sprite_tl( &sprtg[62], tx+32, ty- 41, FALSE );
    render_sprite_tl( &sprtg[62], tx+32, ty- 26, FALSE );
  }
}

void redrawtardisdoor( void )
{
  int tx, ty;

  if( gid.ontardis != ON_TARDIS_STEP_INSIDE )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[GSPRITEX] );
  tx = (trd_x>>8)-fgx;
  ty = (trd_y>>8)-fgy;
  render_sprite_tl( &sprtg[53], tx+2, ty-73, FALSE );
  render_sprite_tl( &sprtg[52], tx+2, ty-44, FALSE );
  render_sprite_tl( &sprtg[52], tx+2, ty-28, FALSE );
  render_sprite_tl_clipy( &sprtg[8], tx, ty-12, trd_boostercutoff-fgy, FALSE );
}


/************************* MR. T **************************/

int mrtbubtimer, mrtstate;
int atv_x, atv_honker;

void initmrt( void )
{
  if( strig[ST4_ATEAM_CALLED] )
  {
    mrtstate = 2;
    infos4[0].active = FALSE;
    infos4[2].active = FALSE;
    infos4[3].active = FALSE;
    set_blocker( 55, 22, 1, 5, 0 );
    return;
  }

  mrtstate = 0;
  mrtbubtimer = -1;
  atv_x = -1;
  atv_honker = -1;
  infos4[0].active = TRUE;
  infos4[2].active = TRUE;
  infos4[3].active = TRUE;
  set_blocker( 55, 22, 1, 5, 255 );
}

void animatemrt( void )
{
  switch( mrtstate )
  {
    case 0:
      if( mrtbubtimer > 0 )
      {
        mrtbubtimer--;
      } else if( mrtbubtimer == 0 ) {
        if( winfo != 0 )
          mrtbubtimer = -1;
      }
      break;
    
    case 1:
      if( atv_honker < 0 )
      {
        if( (atv_x-bgx) > 0 )
        {
          actionsound( SND_VANHONK, sfxvol );
          atv_honker = 0;
        }
      }

      if( (atv_x-bgx) >= 330 )
      {
        mrtstate++;
        atv_x = -1;
        break;
      }

      atv_x += 3;
      break;
  }
}

void triggermrtbubble( void )
{
  if( mrtstate != 0 ) return;
  if( mrtbubtimer != -1 ) return;

  mrtbubtimer = 120;
}

void renderateamvan( void )
{
  int wf;

  if( atv_x == -1 )
    return;

  wf = ((frame>>3)&1)+29;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl( &psprt4[28], atv_x-bgx, 114-bgy, FALSE );
  render_sprite_tl( &psprt4[wf], atv_x+12-bgx, 163-bgy, FALSE );
  render_sprite_tl( &psprt4[wf], atv_x+98-bgx, 163-bgy, FALSE );
}

void rendermrt( void )
{
  if( mrtstate != 0 ) return;

  if( mrtbubtimer > 0 )
  {
    glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
    render_sprite_tl( &sprt4[11], 848-fgx, 326-fgy, FALSE );
  }

  if( ( 870 >= (fgx+336) ) ||
      ( 897  < (fgx-16) ) ||
      ( 371 >= (fgy+256) ) ||
      ( 432  < (fgy-16) ) )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl( &psprt4[2], 870-fgx, 371-fgy, FALSE );
}

BOOL triggerateamvan( void )
{
  if( ( clevel != 4 ) || ( winfo != 3 ) )
    return FALSE;

  if( gid.coins < 200 )
  {
    giddy_say( "I haven't got enough. \nIt requires 200 coins." );
    return TRUE;
  }

  atv_x = 1462;

  gid.coins -= 200;
  if( gid.coins == 0 ) inv[INV_COINS] = 0;
  strig[ST4_ATEAM_CALLED] = 1;
  inv[INV_ATEAMPHONENO] = 0;
  mrtstate = 1;
  infos4[0].active = FALSE;
  infos4[2].active = FALSE;
  infos4[3].active = FALSE;
  set_blocker( 55, 22, 1, 5, 0 );
  actionsound( SND_HUZZAH, sfxvol );
  return TRUE;
}

/*********************** PHONE BOX ************************/

void initphonebox( void )
{
  if( strig[ST4_ATEAM_CALLED] )
  {
    atv_x = -1;
    mrtstate = 2;
    n_things4[8].active = TRUE;
    p_things4[30].x = 3456;
    p_things4[30].y =  434;
    p_things4[30].frames[0] = 17;
    infos4[2].active = FALSE;
    infos4[3].active = FALSE;
    set_blocker( 55, 22, 1, 5, 0 );
    return;
  }

  set_blocker( 55, 22, 1, 5, 255 );

  if( strig[ST4_PHONEBOX_FIXED] )
  {
    n_things4[8].active = TRUE;
    p_things4[30].x = 3456;
    p_things4[30].y =  434;
    p_things4[30].frames[0] = 17;
    infos4[2].active = FALSE;
    infos4[3].active = TRUE;
    return;
  }

  n_things4[8].active = FALSE;
  p_things4[30].x = 3454;
  p_things4[30].y =  418;
  p_things4[30].frames[0] = 16;
  infos4[2].active = TRUE;
  infos4[3].active = FALSE;
}

BOOL triggerphonebox( void )
{
  if( ( clevel != 4 ) || ( winfo != 2 ) || ( strig[ST4_PHONEBOX_FIXED] != 0 ) )
    return FALSE;

  strig[ST4_PHONEBOX_FIXED] = 1;
  inv[INV_ELECTRICALTOOLKIT] = 0;
  initphonebox();
  actionsound( SND_HUZZAH, sfxvol );
  giddy_say( "Huzzah!" );

  return TRUE;
}

/************************* FACTORY **************************/

int detcount, facstate, facdrop, facbang;
int plunge;

struct factorybits
{
  int x, y;
  int sprite;
  float ang,anga;
};
#define NUM_FACTORYBITS 16
struct factorybits facbits[NUM_FACTORYBITS];

//                128 129 130 131 132 133 134 135 136 137 138 139 140
Uint8 facmap[] = { 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
                   46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
                    1,  5,  5,  5,  5,  5,  1,  5,  5,  5,  5,  5,  1,
                    1,  2,  3,  3,  3,  3,  1,  2,  3,  3,  3,  3,  1,
                    1,  2, 45,  3, 11,  3,  1,  2, 11,  3, 45,  3,  1,
                    1,  2,  3,  3,  3,  3,  1,  2,  3,  3,  3,  3,  1,
                    1,  5,  5,  5,  5,  5,  1,  5,  5,  5,  5,  5,  1,
                    1,  2,  3,  3,  3,  3,  1,  2,  3,  3,  3,  3,  1,
                    1,  2, 11,  3, 45,  3,  1,  2, 45,  3, 11,  3,  1 };

Uint8 facmap2[] = { 1,  2, 11,  3,  3,  3,  1,  2,  3,  3,  3,  3,  1 };

void initfactory( void )
{
  if( strig[ST4_FACTORY_BLOWN] )
  {
    infos4[4].active = FALSE;
    infos4[6].active = FALSE;
    p_things4[40].y      = 403;
    p_things4[40].flags &= ~THF_BLOCKFALL;
    p_things4[40].active = TRUE;
    p_things4[41].active = TRUE;
    detcount = 3;
    facstate = 5;
    return;
  }

  if( strig[ST4_DETONATOR_PLACED] )
  {
    infos4[4].active = FALSE;
    infos4[6].active = TRUE;
    p_things4[40].y      = 393;
    p_things4[40].flags |= THF_BLOCKFALL;
    p_things4[40].active = TRUE;
    p_things4[41].active = TRUE;
    p_things4[42].active = FALSE;
    detcount = 0;
    facstate = 0;
    facdrop  = 0;
    return;
  }

  infos4[4].active = TRUE;
  infos4[6].active = FALSE;
  p_things4[40].y      = 393;
  p_things4[40].flags |= THF_BLOCKFALL;
  p_things4[40].active = FALSE;
  p_things4[41].active = FALSE;
  p_things4[42].active = FALSE;
  detcount = 0;
  facstate = 0;
  facdrop = 0;
}

void animatefactory( void )
{
  BOOL dropem;
  int i;

  dropem = FALSE;
  switch( facstate )
  {
    case 1:
      if( plunge < (10<<8) )
      {
        plunge += 0x80;
        gid.locky += 0x80;
        p_things4[40].y = 393+(plunge>>8);
        break;
      }

      p_things4[40].flags &= ~THF_BLOCKFALL;
      gid.lockx = -1;
      facstate++;
      facbang = 60;
      for( i=0; i<NUM_FACTORYBITS; i++ )
      {
        facbits[i].x = (rand()%192)+4064;
        facbits[i].y = 80-(rand()%120);
        facbits[i].sprite = ((i&3)==0)?18:19;
        facbits[i].ang = (float)(rand()%360);
        facbits[i].anga = (float)((rand()%20)-10);
      }
      p_things4[42].y = 0;
      p_things4[42].active = TRUE;
      break;
    
    case 2:
      if( facbang > 0 )
      {
        facbang--;
        if( (frame&1) == 0 )
        {
          startbgincidental( (rand()%(13*16))+128*16, 168, 0, -1, GSPRITEX, bangframes, 5, 2 );
          if( (rand()%5) == 0 )
            incidentalsound( SND_EXPLOS, sfxvol );
        }
        break;
      }
      
      facstate++;

    case 3:
      dropem = TRUE;

      if( facdrop < 144 )
      {
        facdrop++;
        if( (frame&1) == 0 )
        {
          startbgincidental( (rand()%(13*16))+128*16, 168, 0, -1, GSPRITEX, bangframes, 5, 2 );
          if( (rand()%5) == 0 )
            incidentalsound( SND_EXPLOS, sfxvol );
        }
        break;
      }
      
      facstate++;
      break;
    
    case 4:
      dropem = TRUE;
      break;

  }

  if( dropem )
  {
    int lowest;

    lowest = 0;
    for( i=0; i<NUM_FACTORYBITS; i++ )
    {
      facbits[i].y += 4;
      facbits[i].ang += facbits[i].anga;
      if( facbits[i].ang < 0.0f ) facbits[i].ang += 360.0f;
      if( facbits[i].ang >= 360.0f ) facbits[i].ang -= 360.0f;
      if( facbits[i].y > lowest ) lowest = facbits[i].y;
    }

    if( p_things4[42].y < 403 )
    {
      p_things4[42].y += 4;
      if( p_things4[42].y > 403 )
        p_things4[42].y = 403;
    } else {
      if( lowest > 600 ) facstate = 5;
    }
  }
}

void renderfactory( void )
{
  int x, y, xo, yo;
  BOOL wobblr;
  struct btex *tx;

  if( facstate >= 4 ) return;

  glBindTexture( GL_TEXTURE_2D, tex[BLOCKTEX]);
  glColor4ub(255, 255, 255, 255);

  glLoadIdentity();
  glBegin( GL_QUADS );

  if( ( facstate == 2 ) || ( facstate == 3 ) )
    wobblr = TRUE;
  else
    wobblr = FALSE;

  yo = facdrop+16-bgy;
  for( y=0; y<9; y++, yo+=16 )
  {
    if( yo >= 154 ) break;
    xo = (128*16)-bgx;
    if( wobblr ) xo += ((rand()%4)-2);

    for( x=0; x<13; x++, xo+=16 )
    {
      tx = &bt[facmap[y*13+x]];
      glTexCoord2f( tx->x,       tx->y       ); glVertex3f( (float)(xo   ), (float)(yo   ), 0.0f );
      glTexCoord2f( tx->x+tx->w, tx->y       ); glVertex3f( (float)(xo+16), (float)(yo   ), 0.0f );
      glTexCoord2f( tx->x+tx->w, tx->y+tx->h ); glVertex3f( (float)(xo+16), (float)(yo+16), 0.0f );
      glTexCoord2f( tx->x,       tx->y+tx->h ); glVertex3f( (float)(xo   ), (float)(yo+16), 0.0f );
    }
  }

  yo = 160-bgy;
  xo = (128*16)-bgx;
  if( wobblr ) xo += ((rand()%4)-2);

  for( x=0; x<13; x++, xo+=16 )
  {
    tx = &bt[facmap2[x]];
    glTexCoord2f( tx->x,       tx->y       ); glVertex3f( (float)(xo   ), (float)(yo   ), 0.0f );
    glTexCoord2f( tx->x+tx->w, tx->y       ); glVertex3f( (float)(xo+16), (float)(yo   ), 0.0f );
    glTexCoord2f( tx->x+tx->w, tx->y+tx->h ); glVertex3f( (float)(xo+16), (float)(yo+16), 0.0f );
    glTexCoord2f( tx->x,       tx->y+tx->h ); glVertex3f( (float)(xo   ), (float)(yo+16), 0.0f );
  }

  glEnd();
}

void renderfactorybits( void )
{
  int i;
  if( ( facstate != 3 ) && ( facstate != 4 ) )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX]);
  for( i=0; i<NUM_FACTORYBITS; i++ )
    render_sprite( &sprt4[facbits[i].sprite], facbits[i].x-fgx, facbits[i].y-fgy, FALSE, facbits[i].ang );
}

BOOL placedetonator( void )
{
  if( ( clevel != 4 ) || ( winfo != 4 ) )
    return FALSE;

  strig[ST4_DETONATOR_PLACED] = 1;
  inv[INV_DETONATOR] = 0;
  infos4[4].active = FALSE;
  infos4[6].active = TRUE;
  p_things4[40].y      = 393;
  p_things4[40].flags |= THF_BLOCKFALL;
  p_things4[40].active = TRUE;
  p_things4[41].active = TRUE;
  return TRUE;
}

void triggerfactory( void )
{
  strig[ST4_FACTORY_BLOWN] = 1;
  p_things4[40].y      = 393;
  p_things4[40].flags |= THF_BLOCKFALL;
  p_things4[40].active = TRUE;
  p_things4[41].active = TRUE;
  infos4[6].active = FALSE;
  plunge = 0;
  gid.lockx = gid.x;
  gid.locky = gid.y;
  facstate = 1;
  playsound( C_AMBIENT1, SND_BIGBANG, sfxvol );
}

/************************ BUILDER **************************/

int bldrbubtimer;

void initbuilder( void )
{
  if( gid.hardhat )
  {
    infos4[5].active = FALSE;
    set_blocker( 320, 16, 1, 11, 0 );
    return;
  }

  bldrbubtimer = -1;
  set_blocker( 320, 16, 1, 11, 255 );
  infos4[5].active = TRUE;
}

void animatebuilder( void )
{
  if( gid.hardhat ) return;
  if( bldrbubtimer > 0 )
  {
    bldrbubtimer--;
  } else if( bldrbubtimer == 0 ) {
    if( winfo != 5 )
      bldrbubtimer = -1;
  }
}

void triggerbuilderbubble( void )
{
  if( gid.hardhat ) return;
  if( bldrbubtimer != -1 ) return;

  bldrbubtimer = 120;
}

void renderbuilder( void )
{
  if( ( !gid.hardhat ) && ( bldrbubtimer > 0 ) )
  {
    glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
    render_sprite_tl( &sprt4[17], 5076-fgx, 326-fgy, FALSE );
  }

  if( ( 5060 >= (fgx+336) ) ||
      ( 5116  < (fgx-16) ) ||
      ( 326 >= (fgy+256) ) ||
      ( 432  < (fgy-16) ) )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  if( gid.hardhat )
  {
    render_sprite_tl( &psprt4[23], 5108-fgx, 369-fgy, FALSE );
    render_sprite_tl( &psprt4[24], 5108-fgx, 412-fgy, FALSE );
    return;
  }

  render_sprite_tl( &psprt4[22], 5087-fgx, 369-fgy, FALSE );
  render_sprite_tl( &psprt4[24], 5108-fgx, 412-fgy, FALSE );
}

BOOL triggerbuilder( void )
{
  if( ( clevel != 4 ) || ( winfo != 5 ) )
    return FALSE;

  gid.hardhat = TRUE;
  set_blocker( 320, 16, 1, 11, 0 );
  infos4[5].active = FALSE;
  set_blocker( 50, 74, 1, 2, 0 );
  actionsound( SND_HUZZAH, sfxvol );
  inv[INV_HARDHAT] = 0;
  return TRUE;
}

/*********************** CRUSHER ***************************/
int crusherstate, crushx, crushw;

void initcrusher( void )
{
  crushx = 0;
  if( strig[ST4_CARBON_PLACED] )
  {
    crusherstate = 4;
    g_things4[3].active = !strig[ST4_DIAMOND_COLLECTED];
    return;
  }

  crusherstate = 0;
}

void animatecrusher( void )
{
  switch( crusherstate )
  {
    case 1:
      if( crushx < (24<<8) )
      {
        crushx+=0x80;
        break;
      }
      crusherstate++;
      break;
    
    case 2:
      if( crushx > -(12<<8) )
      {
        crushx -= (6<<8);
        if( crushx <= -(12<<8) )
        {
          startincidental( 410, 382, -2, -2, GSPRITEX, bangframes, 5, 2 );
          startincidental( 410, 382,  2, -2, GSPRITEX, bangframes, 5, 2 );
          startincidental( 410, 382,  2,  2, GSPRITEX, bangframes, 5, 2 );
          startincidental( 410, 382, -2,  2, GSPRITEX, bangframes, 5, 2 );
          incidentalsound( SND_EXPLOS, sfxvol );
          incidentalsound( SND_EXPLOS, sfxvol );
          crushx = -(12<<8);
        }
        break;
      }

      if( crushw > 0 )
      {
        crushw--;
        break;
      }

      g_things4[3].active = TRUE;
      crusherstate++;
      break;
    
    case 3:
      if( crushx < 0 )
      {
        crushx += 0x80;
        break;
      }
      crusherstate++;
      break;
      
  }   
}

void rendercrusher( void )
{
  switch( crusherstate )
  {
    case 1:
    case 2:
      glBindTexture( GL_TEXTURE_2D, tex[GSPRITEX] );
      render_sprite_tl( &sprtg[80], 388-fgx, 382-fgy, FALSE );
      break;
  }

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl( &psprt4[31], (320-(crushx>>8))-fgx, 373-fgy, FALSE );
  render_sprite_tl( &psprt4[31], (320-(crushx>>8))-fgx, 392-fgy, FALSE );
  render_sprite_tl( &psprt4[34], (318-(crushx>>8))-fgx, 371-fgy, FALSE );
  render_sprite_tl( &psprt4[34], (318-(crushx>>8))-fgx, 390-fgy, FALSE );
  render_sprite_tl( &psprt4[32], (375-(crushx>>8))-fgx, 362-fgy, FALSE );

  render_sprite_tl( &psprt4[31], (412+(crushx>>8))-fgx, 373-fgy, FALSE );
  render_sprite_tl( &psprt4[31], (412+(crushx>>8))-fgx, 392-fgy, FALSE );
  render_sprite_tl( &psprt4[35], (476+(crushx>>8))-fgx, 371-fgy, FALSE );
  render_sprite_tl( &psprt4[35], (476+(crushx>>8))-fgx, 390-fgy, FALSE );
  render_sprite_tl( &psprt4[33], (410+(crushx>>8))-fgx, 362-fgy, FALSE );
}

BOOL triggercrusher( void )
{
  if( ( clevel != 4 ) || ( winfo != 7 ) )
    return FALSE;

  inv[INV_LUMPOFCARBON] = 0;
  strig[ST4_CARBON_PLACED] = 1;
  crusherstate = 1;
  crushw = 40;
  actionsound( SND_HUZZAH, sfxvol );
  incidentalsound( SND_STEAM, sfxvol );
  return TRUE;
}

/****************** MULDOON AND SKELLY *********************/

int mnsstate;
int mnstimer;

void initmuldoonandskelly( void )
{
  if( strig[ST3_GIVEN_BALLOON] )
  {
    mnsstate = 2;
    mnstimer = 0;
    infos3[1].active = FALSE;
    return;
  }

  mnsstate = 0;
  mnstimer = -1;
  infos3[1].active = TRUE;
}

void animatemuldoonandskelly( void )
{
  if( mnsstate > 1 )
    return;

  if( mnsstate == 1 )
  {
    if( mnstimer <= 0 )
      return;

    mnstimer--;
    return;
  }

  if( mnstimer > 0 )
  {
    mnstimer--;
  } else if( mnstimer == 0 ) {
    if( winfo != 1 )
      mnstimer = -1;
  }
}

void triggermuldoonandskellybubble( void )
{
  if( mnstimer != -1 ) return;

  mnstimer = 150;
}

void rendermuldoonandskelly( void )
{
  if( mnsstate > 1 )
    return;

  if( mnstimer > 0 )
  {
    switch( mnsstate )
    {
      case 0:
        glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
        render_sprite_tl( &psprt3[2], 600-fgx, 240-fgy, FALSE ); // It was a UFO
        if( mnstimer <= 120 )
        {
          glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
         render_sprite_tl( &sprt3[7], 660-fgx, 260-fgy, FALSE ); // Hmm
        }
        break;
      
      case 1:
        glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
        render_sprite_tl( &psprt3[10], 600-fgx, 240-fgy, FALSE ); // Wow! Thanks!
        render_sprite_tl( &psprt3[11], 600-fgx, 264-fgy, FALSE );
        break;
    }
  }

  if( ( 630 >= (fgx+336) ) ||
      ( 688  < (fgx-16) ) ||
      ( 288 >= (fgy+256) ) ||
      ( 400  < (fgy-16) ) )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl( &psprt3[0], 630-fgx, 288-fgy, FALSE );
  render_sprite_tl( &psprt3[1], 662-fgx, 292-fgy, FALSE );
}

BOOL triggermuldoonandskelly( void )
{
  if( ( clevel != 3 ) || ( winfo != 1 ) )
    return FALSE;

  infos3[1].active = FALSE;
  inv[INV_BALLOONWRECKAGE] = 0;
  inv[INV_ATEAMPHONENO] = 1;
  you_now_have( ivtexts[INV_ATEAMPHONENO].pickup );
  actionsound( SND_ITEMGET, sfxvol );
  mnsstate = 1;
  mnstimer = 180;
  strig[ST3_GIVEN_BALLOON] = 1;
  return TRUE;
}


/************************ BOULDER **************************/

int bldx, bldy;
int bldya;
float bldang;

void initboulder( void )
{
  bldx = -1;
  bldang = 0.0f;
}

void animateboulder( void )
{
  int i, dummy;
  if( bldx == -1 )
  {
    if( ( gid.px < 1024 ) ||
        ( gid.px > 1152 ) )
      return;

    bldx = 912;
    bldy = 312<<8;
    bldya = 0x80;
    return;
  }

  bldx += 4;

  if( bldy >= (1120<<8) )
  {
    bldx = -1;
    return;
  }

  bldang -= 4.0f;
  if( bldang < 0.0f ) bldang += 360.0f;

  bldy += bldya;
  bldya += 0x40;
  if( bldya > 0xa00 ) bldya = 0xa00;

  if( bldx < 1632 )
  {
    i = testspritedowncolis( SPRITEX, 11, bldx, bldy>>8, &dummy, 0 );
    if( i > 0 )
    {
      bldy -= i;
      bldya = -0x300;
      if( ( (bldx-fgx) > -40 ) &&
          ( (bldx-fgx) < 360 ) &&
          ( ((bldy>>8)-fgy) > -40 ) &&
          ( ((bldy>>8)-fgy) < 264 ) )
        incidentalsound( SND_EXPLOS, sfxvol );
    }
  }

  if( giddydanger( SPRITEX, 11, bldx, bldy>>8, FALSE ) )
    giddyhit();
}

void renderboulder( void )
{
  if( bldx == -1 ) return;
  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  render_sprite( &sprt3[11], bldx-fgx, (bldy>>8)-fgy, FALSE, bldang );
}


/************************ DRAGON ***************************/

struct dragonsnore
{
  int x, y, alph;
  float ang;
  float sc;
};
#define NUM_DSNORE 5
struct dragonsnore dsnore[NUM_DSNORE];

struct dragonflame
{
  int x, y, d;
  float sc;
};
#define NUM_DFLAME 13
struct dragonflame dflame[NUM_DFLAME];

int dhead, dbody;
int dragonmode, dflametime;

void initdragon( void )
{
  int i;
  for( i=0; i<NUM_DSNORE; i++ )
    dsnore[i].x = -i*64;
  dragonmode = 0;

  g_things3[11].active = FALSE;
  g_things3[12].active = FALSE;
  g_things3[13].active = FALSE;

  if( strig[ST3_CANDLE_LIT] )
  {
    g_things3[12].active = !strig[ST3_CANDLE_COLLECTED];
    g_things3[13].active = !strig[ST3_CANDLE_COLLECTED];
    dragonmode = 3;
    infos3[2].active = FALSE;
    infos3[7].active = FALSE;
    infos3[8].active = FALSE;
    dhead = 3*8;
    dbody = 4;
    return;
  }

  if( strig[ST3_DRAGON_DRUNK] )
  {
    infos3[2].active = FALSE;
    infos3[7].active = FALSE;
    infos3[8].active = TRUE;
    dhead = 3*8;
    dbody = 4;
    return;
  }

  if( strig[ST3_DRAGON_HONKED] )
  {
    infos3[2].active = FALSE;
    infos3[7].active = TRUE;
    infos3[8].active = FALSE;
    dhead = 3*8;
    dbody = 3;
    return;
  }

  infos3[2].active = TRUE;
  infos3[7].active = FALSE;
  infos3[8].active = FALSE;
  dhead = 0;
  dbody = 3;
}

void animatedragon( void )
{
  int i, j;

  if( strig[ST3_CANDLE_LIT] )
  {
    switch( dragonmode )
    {
      case 0:
        for( i=0; i<NUM_DFLAME; i++ )
        {
          dflame[i].x = 1574;
          dflame[i].y = 92<<8;
          dflame[i].d = i*2;
          dflame[i].sc = 0.01f;
        }
        dragonmode = 1;
        dflametime = 120;
      
      case 1:
        if( dflametime > 0 )
        {
          dflametime--;
        } else {
          dragonmode = 2;
          g_things3[11].active = FALSE;
          g_things3[12].active = TRUE;
          g_things3[13].active = TRUE;
        }

        for( i=0; i<NUM_DFLAME; i++ )
        {
          if( dflame[i].d > 0 )
          {
            dflame[i].d--;
            continue;
          }
          dflame[i].x += 3;
          dflame[i].y -= 0xc0;
          dflame[i].sc += 0.04f;
          if( dflame[i].x > 1652 )
          {
            dflame[i].x = 1574;
            dflame[i].y = 92<<8;
            dflame[i].sc = 0.01f;
          }
        }
        break;
      
      case 2:
        j = 0;
        for( i=0; i<NUM_DFLAME; i++ )
        {
          if( dflame[i].d > 0 )
            continue;

          j++;
          dflame[i].x += 3;
          dflame[i].y -= 0xc0;
          dflame[i].sc += 0.04f;
          if( dflame[i].x > 1652 )
            dflame[i].d = 1;
        }

        if( j == 0 )
          dragonmode = 3;
        break;
    }
  }

  if( strig[ST3_DRAGON_HONKED] )
  {
    if( dhead < (3*8) )
      dhead++;
  }

  for( i=0; i<NUM_DSNORE; i++ )
  {
    if( dsnore[i].x < 0 )
    {
      dsnore[i].x++;
      continue;
    }

    if( dsnore[i].x == 0 )
    {
      if( strig[ST3_DRAGON_HONKED] )
        continue;

      dsnore[i].x = 1588;
      dsnore[i].y = 100<<8;
      dsnore[i].ang = 0.0f;
      dsnore[i].alph = 255;
      dsnore[i].sc = 0.1f;
      continue;
    }

    dsnore[i].ang += 0.025f;
    if( dsnore[i].ang >= 6.2831853f )
      dsnore[i].ang -= 6.2831853f;
    if( dsnore[i].sc < 0.5f )
    {
      dsnore[i].sc += 0.007f;
      if( dsnore[i].sc > 0.5f ) dsnore[i].sc = 0.5f;
    } else if( dsnore[i].alph > 0 ) {
      if( strig[ST3_DRAGON_HONKED] )
        dsnore[i].alph -= 4;
      else
        dsnore[i].alph--;
      if( dsnore[i].alph < 0 ) dsnore[i].alph = 0;
    } else {
      dsnore[i].x = 0;
    }
    dsnore[i].y-=0x40;
  }
}

void renderdragon( void )
{
  int i, xo;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; i<NUM_DSNORE; i++ )
  {
    if( dsnore[i].x <= 0 )
      continue;

    xo = (sin(dsnore[i].ang)*24.0f);
    render_sprite_scaleda( &psprt3[9], (dsnore[i].x+xo)-fgx, (dsnore[i].y>>8)-fgy, FALSE, 0.0f, dsnore[i].sc, dsnore[i].alph  );
  }

  switch( dragonmode )
  {
    case 1:
    case 2:
      for( i=0; i<NUM_DFLAME; i++ )
        render_sprite_scaled( &psprt3[28+(frame&1)], dflame[i].x-fgx, (dflame[i].y>>8)-fgy, FALSE, 0.0f, dflame[i].sc );
      break;
  }

  render_sprite_tl( &psprt3[5+(dhead/8)], 1558-fgx, 76-fgy, FALSE );
  render_sprite_tl( &psprt3[dbody], 1536-fgx, (dbody==3)?(97-fgy):(98-fgy), FALSE );
}

BOOL honkatdragon( void )
{
  if( ( clevel != 3 ) || ( winfo != 2 ) )
    return FALSE;

  strig[ST3_DRAGON_HONKED] = 1;
  inv[INV_AIRHORN] = 0;
  infos3[2].active = FALSE;
  infos3[7].active = TRUE;
  infos3[8].active = FALSE;
  incidentalsound( SND_AIRHORN, sfxvol );
  return TRUE;
}

BOOL drinkfordragon( void )
{
  if( ( clevel != 3 ) || ( winfo != 7 ) )
    return FALSE;

  strig[ST3_DRAGON_DRUNK] = 1;
  inv[INV_TURPS] = 0;
  infos3[2].active = FALSE;
  infos3[7].active = FALSE;
  infos3[8].active = TRUE;
  dbody = 4;
  actionsound( SND_HUZZAH, sfxvol );
  return TRUE;
}

BOOL triggerdragon( void )
{
  if( ( clevel != 3 ) || ( winfo != 8 ) )
    return FALSE;

  actionsound( SND_HUZZAH, sfxvol );
  incidentalsound( SND_DRAGONFLAME, sfxvol );
  strig[ST3_CANDLE_LIT] = 1;
  inv[INV_CANDLESTICK] = 0;
  infos3[2].active = FALSE;
  infos3[7].active = FALSE;
  infos3[8].active = FALSE;
  g_things3[11].active = TRUE;
  return TRUE;
}

/********************* WEATHER BALLOON *********************/

int wbstate, wby;
float wbbob;

int wbpx[8], wbpy[8];
float wbpscale;

void initballoon( void )
{
  if( strig[ST3_BALLOON_POPPED] )
  {
    wbstate = 3;
    infos3[3].active = FALSE;
  } else {
    wbstate = 0;
    infos3[3].active = TRUE;
  }
  wby = 416;
  wbbob = 0.0f;
}

void animateballoon( void )
{
  switch( wbstate )
  {
    case 0:
      wbbob += 0.08f;
      if( wbbob >= 6.2831853f )
        wbbob -= 6.2831853f;
      break;
    
    case 1:
      if( wbpscale > 0.1f )
      {
        wbpscale -= 0.1f;
      } else {
        wbstate++;
      }
    
    case 2:
      if( wby < 546 )
      {
        wby += 4;
        break;
      }

      if( wbstate == 2 )
      {
        wbstate++;
        g_things3[6].active = TRUE;
        incidentalsound( SND_EXPLOS, sfxvol );
      }
      break;
  }

}

void renderballoon( void )
{
  int i, yo;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  switch( wbstate )
  {
    case 0:
      yo = sin( wbbob ) * 8.0f;
      render_sprite_tl( &sprt3[27], 3136-fgx, (wby-39)+yo-fgy, FALSE );
      render_sprite_tl( &sprt3[28], 3147-fgx, (wby+10)+yo-fgy, FALSE );
      break;

    case 1:
      for( i=0; i<8; i++ )
        render_sprite_scaled( &sprt3[30], wbpx[i]-fgx, wbpy[i]-fgy, FALSE, 0.0f, wbpscale );

    case 2:
      render_sprite_tl( &sprt3[29], 3147-fgx, wby-fgy, FALSE );
      break;
  }
}

BOOL triggerballoon( void )
{
  int i, yo;
  float ang;

  if( ( clevel != 3 ) || ( winfo != 3 ) )
    return FALSE;

  yo = sin( wbbob ) * 8.0f;
  wby = wby+yo+10;

  for( i=0, ang=0.0f; i<8; i++, ang+=(3.14159265f/4.0f) )
  {
    wbpx[i] = 3159 + ((int)(sin(ang)*18.0f));
    wbpy[i] = ((wby-11)+yo) + ((int)(cos(ang)*18.0f));
  }
  wbpscale = 1.1f;

  infos3[3].active = FALSE;
  strig[ST3_BALLOON_POPPED] = 1;
  inv[INV_CATAPULT] = 0;
  wbstate = 1;
  incidentalsound( SND_WEATHERBALLOONPOP, sfxvol );

  return TRUE;
}

/************************* SEE SAW *************************/

// Overall state
int   ss_state;

// block
int   ss_bx, ss_by, ss_ax, ss_ay;
float ss_bang;

// Plank
float pk_ang;

// Hook and Weight
int   hk_y, wgt_y;

void initseesaw( void )
{
  if( strig[ST3_SEESAW_SEESAWED] )
  {
    ss_bx    = 4144<<8;
    ss_by    =  592<<8;
    ss_bang  = 0.0f;
    pk_ang   = -51.0f;
    wgt_y    = 588;
    ss_state = 4;
    infos3[4].active = FALSE;
    set_blocker( 251, 36, 2, 2,   0 );
    set_blocker( 258, 36, 2, 2, 255 );
    return;
  }

  infos3[4].active = TRUE;
  set_blocker( 251, 36, 2, 2, 255 );
  set_blocker( 258, 36, 2, 2,   0 );

  if( strig[ST3_PLANK_PLACED] )
  {
    pk_ang   = -95.0f;
    ss_bx    = 4032<<8;
    ss_by    =  584<<8;
    ss_bang  = -20.0f;
    wgt_y    = 510;
    hk_y     = 461;
    ss_ax    = 0;
    ss_ay    = 0;
    ss_state = 1;
    return;
  }

  ss_state = 0;
  ss_bx    = 4032<<8;
  ss_by    =  592<<8;
  ss_bang  = 0.0f;
  wgt_y    = 510;
  hk_y     = 461;
  ss_ax    = 0;
  ss_ay    = 0;
}

void animateseesaw( void )
{
  switch( ss_state )
  {
    case 2:
      if( wgt_y < 588 )
      {
        wgt_y += 4;
        if( wgt_y > 588 ) wgt_y = 588;
      }

      hk_y += 4;
      if( hk_y >= 550 )
      {
        incidentalsound( SND_SPRING, sfxvol );
        ss_state++;
      }

      if( wgt_y >= 524 )
      {
        pk_ang += 2.2f;
        ss_ax =  0x200;
        ss_ay = -0x400;
      }

      ss_bx += ss_ax;
      ss_by += ss_ay;
      ss_ay += 0x80;
      if( ss_ay > 0x600 ) ss_ay = 0x600;
      if( ss_bang > -360.0f ) ss_bang -= 6.0f;
      break;
    
    case 3:
      ss_bx += ss_ax;
      ss_by += ss_ay;
      ss_ay += 0x80;
      if( ss_ay > 0x600 ) ss_ay = 0x600;
      if( ss_bx > (4144<<8) )
      {
        ss_bx = 4144<<8;
        ss_ax = 0;
      }

      if( ss_by > (592<<8) )
      {
        ss_by = 592<<8;
        ss_ay = 0;
      }

      if( ss_bang > -360.0f )
      {
        ss_bang -= 6.0f;
        if( ss_bang < -360.0f ) ss_bang = -360.0f;
      }

      if( ( ss_bx   == (4144<<8) ) &&
          ( ss_by   ==  (592<<8) ) &&
          ( ss_bang ==  -360.0f ) )
      {
        set_blocker( 258, 36, 2, 2, 255 );
        incidentalsound( SND_SPUDBOSSLAND, sfxvol );
        ss_state++;
      }
      break;
  }
}

void renderseesawbg( void )
{
  if( ss_state > 0 )
  {
    glBindTexture( GL_TEXTURE_2D, tex[GSPRITEX] );
    render_sprite_offs( &sprtg[42], 3948-fgx, 594-fgy,   1, -82, FALSE, pk_ang );
    render_sprite_offs( &sprtg[43], 3948-fgx, 594-fgy,  -2, -64, FALSE, pk_ang );
    render_sprite_offs( &sprtg[43], 3948-fgx, 594-fgy,  -6, -48, FALSE, pk_ang );
    render_sprite_offs( &sprtg[43], 3948-fgx, 594-fgy, -10, -32, FALSE, pk_ang );
    render_sprite_offs( &sprtg[43], 3948-fgx, 594-fgy, -14, -16, FALSE, pk_ang );
    render_sprite_offs( &sprtg[43], 3948-fgx, 594-fgy, -18,   0, FALSE, pk_ang );
    render_sprite_offs( &sprtg[43], 3948-fgx, 594-fgy, -22,  16, FALSE, pk_ang );
    render_sprite_offs( &sprtg[43], 3948-fgx, 594-fgy, -26,  32, FALSE, pk_ang );
    render_sprite_offs( &sprtg[43], 3948-fgx, 594-fgy, -30,  48, FALSE, pk_ang );
    render_sprite_offs( &sprtg[43], 3948-fgx, 594-fgy, -34,  64, FALSE, pk_ang );
    render_sprite_offs( &sprtg[44], 3948-fgx, 594-fgy, -37,  80, FALSE, pk_ang );
  }
}

void renderseesaw( void )
{
  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  switch( ss_state )
  {
    case 0:
    case 1:
      render_sprite( &psprt3[17], 3883-fgx, wgt_y-fgy, FALSE, 0.0f );
      render_sprite_tl( &psprt3[16], 3878-fgx, hk_y-fgy, FALSE );
      break;

    case 2:
      render_sprite_tl( &psprt3[16], 3878-fgx, hk_y-fgy, FALSE );
    case 3:
    case 4:
      render_sprite( &psprt3[17], 3883-fgx, wgt_y-fgy, FALSE, 0.0f );
      break;
  }

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite( &psprt3[12], (ss_bx>>8)-fgx, (ss_by>>8)-fgy, FALSE, ss_bang );

  render_sprite_tl( &psprt3[15], 3880-fgx, 419-fgy, FALSE );
  render_sprite_tl( &psprt3[14], 3876-fgx, 414-fgy, FALSE );
}

BOOL placeplank( void )
{
  if( ( clevel != 3 ) || ( winfo != 4 ) )
    return FALSE;

  ss_state = 1;
  inv[INV_PLANK] = 0;
  strig[ST3_PLANK_PLACED] = 1;
  pk_ang  = -95.0f;
  ss_by   =  584<<8;
  ss_bang = -20.0f;
  return TRUE;
}

BOOL triggerseesaw( void )
{
  if( ( clevel != 3 ) || ( winfo != 4 ) )
    return FALSE;
  if( ss_state != 1 )
    return FALSE;

  inv[INV_SCISSORS] = 0;
  strig[ST3_SEESAW_SEESAWED] = 1;
  infos3[4].active = FALSE;
  ss_state = 2;
  set_blocker( 251, 36, 2, 2,   0 );
  actionsound( SND_HUZZAH, sfxvol );
  incidentalsound( SND_WHOOSH, sfxvol );
  return TRUE;
}


/********************** FALLING BLOCKS *********************/

struct fblock
{
  int x, y;
  float a;
};
struct fblock fb_top[3], fb_bot[4];
int fb_state;

void initfallingblocks( void )
{
  int i;

  if( strig[ST3_BLOCKS_FALLEN] )
  {
    fb_state = 2;
    set_blocker( 297, 40, 6, 1, 0 );
    return;
  }

  for( i=0; i<3; i++ ) { fb_top[i].x = 4768+i*32; fb_top[i].y = 648; fb_top[i].a = 0.0f; }
  for( i=0; i<4; i++ ) { fb_bot[i].x = 4752+i*32; fb_bot[i].y = 664; fb_bot[i].a = 0.0f; }
  fb_state = 0;
  set_blocker( 297, 40, 6, 1, 255 );
}

void animatefallingblocks( void )
{
  int i;

  if( fb_state != 1 ) return;

  if( fb_top[0].a > -24.0f ) fb_top[0].a -= 2.0f;
  if( fb_top[1].a > -10.0f ) fb_top[1].a -= 2.0f;
  if( fb_top[2].a < 26.0f )  fb_top[2].a += 2.0f;

  if( fb_bot[0].a > -22.0f ) fb_bot[0].a -= 2.0f;
  if( fb_bot[1].a > -8.0f ) fb_bot[1].a -= 2.0f;
  if( fb_bot[2].a < 12.0f ) fb_bot[2].a += 2.0f;
  if( fb_bot[3].a < 24.0f )  fb_bot[3].a += 2.0f;

  for( i=0; i<3; i++ )
  {
    if( fb_top[i].y < 860 )
    {
      fb_top[i].y += ((i&1)+3);
      if( ( fb_top[i].y >= 808 ) && ( fb_top[i].y < 812 ) ) incidentalsound( SND_EXPLOS, sfxvol );
    }
  }
  for( i=0; i<4; i++ ) { if( fb_bot[i].y < 860 ) fb_bot[i].y += ((i&1)+3); }
  if( fb_top[0].y >= 860 )
    fb_state++;
}

void triggerfallingblocks( void )
{
  switch( clevel )
  {
    case 3:
      if( fb_state != 0 ) break;

      if( ( gid.x > (4762<<8) ) &&
          ( gid.x < (4836<<8) ) &&
          ( gid.y > (620<<8) ) )
      {
        fb_state = 1;
        strig[ST3_BLOCKS_FALLEN] = 1;
        set_blocker( 297, 40, 6, 1, 0 );
        incidentalsound( SND_EXPLOS, sfxvol );
      }
      break;
  }
}

void renderfallingblocks( void )
{
  int i;

  if( fb_state > 1 )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; i<3; i++ ) render_sprite( &psprt3[18], fb_top[i].x-fgx, fb_top[i].y-fgy, FALSE, fb_top[i].a );
  for( i=0; i<4; i++ ) render_sprite( &psprt3[19], fb_bot[i].x-fgx, fb_bot[i].y-fgy, FALSE, fb_bot[i].a );
}

void hidefallingblocks( void )
{
  if( fb_state != 1 ) return;

  render_foreground_zone( 299, 48, 2, 3 );
  render_foreground_zone( 298, 51, 5, 1 );
  render_foreground_zone( 295, 51, 2, 1 );
  render_foreground_zone( 295, 52, 9, 3 );
}

/*********************** STARGATE ********************/

float sgt_ang;

void initstargate( void )
{
  sgt_ang = 0.0f;
}

void animatestargate( void )
{
  sgt_ang += 0.03f;
  if( sgt_ang > 6.2831853f ) sgt_ang -= 6.2831853f;

  if( ( gid.stargatewarp == FALSE ) &&
      ( gid.x >= (6634<<8) ) &&
      ( gid.x <  (6674<<8) ) &&
      ( gid.y >= (114<<8) ) &&
      ( gid.y <  (156<<8) ) )
  {
    gid.jumpcount = 0;
    gid.jumping = 0;
    gid.stargatewarp = TRUE;
    gid.sgwsc = 1.0f;
    gid.def = 1;
    incidentalsound( SND_TELEPORT_OUT, sfxvol );
  }
}

#define WOBPTS 10
#define WBW (WOBPTS+1)

void renderstargate( void )
{
  int x, y;
  float px, py, pxa, pya, tx, ty, txa, tya;
  float wx[WBW*WBW], wy[WBW*WBW];
 
  if( gid.x < (6416<<8) )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );

  glLoadIdentity();
  glBegin( GL_QUADS );
  glColor4ub(255, 255, 255, 255);

  px = sgt_ang;
  for( y=0; y<WBW; y++ )
  {
    for( x=0; x<WBW; x++ )
    {
      wx[y*WBW+x] = sin( px ) * 8.0f / 256.0f;
      wy[y*WBW+x] = cos( px ) * 8.0f / 256.0f;
      px += 0.9f;
    }
    px -= 0.24f;
  }

  py = 109.0f - fgy;
  pya = 54.0f / WOBPTS;
  pxa = 50.0f / WOBPTS;

  ty = 53.0f/256.0f;
  tya = (64.0f/256.0f)/WOBPTS;
  txa = (59.0f/256.0f)/WOBPTS;

  for( y=0; y<WOBPTS; y++ )
  {
    px = 6630.0f - fgx;
    tx = 163.0f/256.0f;
    for( x=0; x<WOBPTS; x++ )
    {
      glTexCoord2f( tx+wx[y*WBW+x]          , ty+wy[y*WBW+x]           ); glVertex3f( px    , py    , 0.0f );
      glTexCoord2f( tx+wx[y*WBW+x+1]+txa    , ty+wy[y*WBW+x+1]         ); glVertex3f( px+pxa, py    , 0.0f );
      glTexCoord2f( tx+wx[(y+1)*WBW+x+1]+txa, ty+wy[(y+1)*WBW+x+1]+tya ); glVertex3f( px+pxa, py+pya, 0.0f );
      glTexCoord2f( tx+wx[(y+1)*WBW+x]      , ty+wy[(y+1)*WBW+x]+tya   ); glVertex3f( px    , py+pya, 0.0f );
      px += pxa;
      tx += txa;
    }
    py += pya;
    ty += tya;
  }
  glEnd();

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  render_sprite_tl( &sprt3[33], 6620-fgx, 154-fgy, FALSE );
  render_sprite_tl( &sprt3[34], 6616-fgx, 122-fgy, FALSE );
  render_sprite_tl( &sprt3[35], 6673-fgx, 122-fgy, FALSE );
  render_sprite_tl( &sprt3[36], 6620-fgx, 100-fgy, FALSE );
  render_sprite_tl( &sprt3[37], 6653-fgx, 100-fgy, FALSE );
}

/******************** SPECIAL FADE *******************/

BOOL specialfadecancel;

void initspecialfade( void )
{
  specialfadecancel = FALSE;

  if( strig[ST3_TORCHES_LIT] )
  {
    infos3[6].active = FALSE;
    p_things3[1].frames[0] = 22;
    p_things3[1].numframes = 6;
    p_things3[2].frames[0] = 22;
    p_things3[2].numframes = 6;
    set_blocker( 347, 46, 1, 3, 100 );
    set_blocker( 347, 50, 1, 3, 100 );
    return;
  }

  infos3[6].active = TRUE;
  p_things3[1].frames[0] = 21;
  p_things3[1].numframes = 1;
  p_things3[1].frame = 0;
  p_things3[2].frames[0] = 21;
  p_things3[2].numframes = 1;
  p_things3[2].frame = 0;
  set_blocker( 347, 46, 1, 3, 86 );
  set_blocker( 347, 50, 1, 3, 86 );
}

void dospecialfade( void )
{
  int dark;

  if( ( clevel != 3 ) ||
      ( strig[ST3_TORCHES_LIT] ) ||
      ( winfo != 6 ) )
  {
    if( specialfadecancel )
    {
      fadea = 0;
      specialfadecancel = FALSE;
    }
    return;
  }

  dark = ((gid.px-5200)*9)/8;
  if( dark < 0 ) dark = 0;
  if( dark > 255 ) dark = 255;
  fadea = dark;
  fadeadd = 0;
  specialfadecancel = TRUE;
}

BOOL triggertorches( void )
{
  if( ( clevel != 3 ) || ( winfo != 6 ) )
    return FALSE;

  strig[ST3_TORCHES_LIT] = 1;
  inv[INV_LIGHTEDCANDLE] = 0;
  actionsound( SND_HUZZAH, sfxvol );
  infos3[6].active = FALSE;
  p_things3[1].frames[0] = 22;
  p_things3[1].numframes = 6;
  p_things3[2].frames[0] = 22;
  p_things3[2].numframes = 6;
  set_blocker( 347, 46, 1, 3, 100 );
  set_blocker( 347, 50, 1, 3, 100 );
  return TRUE;
}

/************************ BOULDER 2 ************************/

int bld2x, bld2y;
int bld2ya;
float bld2ang;

void initboulder2( void )
{
  bld2x = -1;
  bld2ang = 0.0f;
}

void animateboulder2( void )
{
  int i, dummy;
  if( bld2x == -1 )
  {
    if( ( gid.px < 6096) ||
        ( gid.px > 6208 ) )
      return;

    bld2x = 6324;
    bld2y = 160<<8;
    bld2ya = 0x80;
    return;
  }

  bld2x -= 4;

  if( bld2y >= (800<<8) )
  {
    bld2x = -1;
    return;
  }

  bld2ang += 4.0f;
  if( bld2ang >= 360.0f ) bld2ang -= 360.0f;

  bld2y += bld2ya;
  bld2ya += 0x40;
  if( bld2ya > 0xa00 ) bld2ya = 0xa00;

  if( bld2x > 6016 )
  {
    i = testspritedowncolis( SPRITEX, 11, bld2x, bld2y>>8, &dummy, 0 );
    if( i > 0 )
    {
      bld2y -= i;
      bld2ya = -0x300;
      if( ( (bld2x-fgx) > -40 ) &&
          ( (bld2x-fgx) < 360 ) &&
          ( ((bld2y>>8)-fgy) > -40 ) &&
          ( ((bld2y>>8)-fgy) < 264 ) )
        incidentalsound( SND_EXPLOS, sfxvol );
    }
  }

  if( giddydanger( SPRITEX, 11, bld2x, bld2y>>8, FALSE ) )
    giddyhit();
}

void renderboulder2( void )
{
  if( bld2x == -1 ) return;
  if( gid.py >= 800 ) return;
  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  render_sprite( &sprt3[11], bld2x-fgx, (bld2y>>8)-fgy, FALSE, bld2ang );
}


/********************* DOOR WIRING *************************/

struct tripledoor
{
  int x, y, o;
};

struct tripledoor tdrs[] = { {  802, 1151, 0 },
                             {  834, 1151, 0 },
                             {  866, 1151, 0 },
                             { 1986, 1951, 0 },
                             { 2018, 1951, 0 },
                             { 2050, 1951, 0 },
                             { 1282, 1375, 0 },
                             { 1314, 1375, 0 },
                             { 1346, 1375, 0 },
                             { 1634, 1375, 0 },
                             { 1666, 1375, 0 },
                             { 1698, 1375, 0 },
                             { 2290,  543, 0 },
                             { 2434,  543, 0 },
                             { 2674,  543, 0 },
                             { -1, } };

void inittripledoors( void )
{
  if( strig[ST5_BATTERY_PLACED] )
  {
    set_blocker( 50, 74, 1, 2, 0 );
    infos5[0].active = FALSE;
    return;
  }

  set_blocker( 50, 74, 1, 2, 255 );
  infos5[0].active = TRUE;
}

void animatetripledoors( void )
{
  int i;
  for( i=0; tdrs[i].x!=-1; i++ )
  {
    if( ( i == 0 ) && ( !strig[ST5_BATTERY_PLACED] ) )
    {
      tdrs[i].o = 0;
      continue;
    }

    if( ( i < 3 ) && ( strig[ST5_BATTERY_PLACED] ) )
    {
      if( ( i > 0 ) && ( tdrs[i-1].o < 32 ) )
        continue;

      if( tdrs[i].o == 0 )
        incidentalsound( SND_POWERDOOR, sfxvol );

      if( tdrs[i].o < 48 )
        tdrs[i].o += 3;
      continue;
    }

    if( ( gid.py > (tdrs[i].y-16) ) &&
        ( gid.py < (tdrs[i].y+112) ) &&
        ( gid.px > (tdrs[i].x-48) ) &&
        ( gid.px < (tdrs[i].x+64) ) )
    {
      if( tdrs[i].o == 0 )
        incidentalsound( SND_POWERDOOR, sfxvol );
      if( tdrs[i].o < 48 )
        tdrs[i].o += 3;
    } else {
      if( tdrs[i].o > 0 )
        tdrs[i].o -= 3;
    }
  }
}

void rendertripledoorsbg( void )
{
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; tdrs[i].x!=-1; i++ )
  {
    render_sprite_tl( &psprt5[3], tdrs[i].x-fgx, (tdrs[i].y-tdrs[i].o/2)-fgy, FALSE );
    render_sprite_tl( &psprt5[2], tdrs[i].x-fgx, (tdrs[i].y+46+tdrs[i].o/2)-fgy, FALSE );
  }
}

void rendertripledoors( void )
{
  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );

  if( strig[ST5_BATTERY_PLACED] )
  {
    render_sprite_tl( &psprt5[58], 736-fgx, 1200-fgy, FALSE );
    return;
  }

  render_sprite_tl( &psprt5[1], 760-fgx, 1217-fgy, FALSE );
}

BOOL triggertripledoors( void )
{
  if( ( clevel != 5 ) || ( winfo != 0 ) )
    return FALSE;

  infos5[0].active = FALSE;
  inv[INV_CHARGEDBATTERY] = 0;
  strig[ST5_BATTERY_PLACED] = TRUE;
  set_blocker( 50, 74, 1, 2, 0 );
  actionsound( SND_HUZZAH, sfxvol );

  return TRUE;
}

/********************* BIG ASS FAN *************************/

int fanframe;
BOOL bigassfanactive;
int bafsndchan=-1, bafsndvol=0;

void initbigassfan( void )
{
  fanframe = 0;
  if( ( strig[ST5_RED_PRESSED] ) &&
      ( strig[ST5_GREEN_PRESSED] ) &&
      ( strig[ST5_BLUE_PRESSED] ) )
  {
    if( !bigassfanactive )
    {
      bigassfanactive = TRUE;
      actionsound( SND_HUZZAH, sfxvol );
    }
    set_blocker( 91, 102, 4, 8, 254 );
    infos5[1].active = FALSE;
    return;
  }

  set_blocker( 91, 102, 4, 8, 0 );
  bigassfanactive = FALSE;
  infos5[1].active = TRUE;
}

void animatebigassfan( void )
{
  int i;
  if( !bigassfanactive )
  {
    if( bafsndchan != -1 )
      stopchannel( bafsndchan );
    return;
  }
  
  i = bafsndvol;
  bafsndvol = 1000-abs(gid.px-1488)*4;
  if( bafsndvol < 0 ) bafsndvol = 0;
  if( ( gid.y < (1506<<8) ) ||
      ( gid.y > (1792<<8) ) )
    bafsndvol = 0;

  bafsndvol = (bafsndvol*sfxvol)/1000;

  if( bafsndvol > 0 )
  {
    if( bafsndchan == -1 )
    {
      ambientloop( SND_BIGASSFAN, bafsndvol, &bafsndchan );
    } else {
      if( i != bafsndvol ) setvol( bafsndchan, bafsndvol );
    }
  } else {
    if( bafsndchan != -1 )
      stopchannel( bafsndchan );
  }

  fanframe = (fanframe+1)&7;
}

void renderbigassfan( void )
{
  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl( &psprt5[(fanframe/2)+4], 1448-fgx, 1758-fgy, FALSE );
//  render_sprite_tl( &psprt5[8], 1394-fgx, 1713-fgy, FALSE );
}

/********************** LASER BEAM *************************/

int lasercut, lasertime, laserstate;
int lasbmsndchan=-1, lasbmsndvol=0;

void initlaserbeam( void )
{
  if( strig[ST5_MIRROR_PLACED] )
  {
    lasercut = 566;
    lasertime = 0;
    laserstate = 3;
    set_blocker( 99, 31, 1, 5, 0 );
    return;
  }

  lasercut = 700;
  lasertime = 0;
  laserstate = 0;
  set_blocker( 99, 31, 1, 5, 255 );
}

void animatelaserbeam( void )
{
  int i;

  if( laserstate < 3 )
  {
    i = lasbmsndvol;
    lasbmsndvol = 1000-abs(gid.px-1592)*4;
    if( lasbmsndvol < 0 ) lasbmsndvol = 0;
    if( gid.y > (600<<8) )
      lasbmsndvol = 0;

    lasbmsndvol = (lasbmsndvol*sfxvol)/1000;

    if( lasbmsndvol > 0 )
    {
      if( lasbmsndchan == -1 )
      {
        ambientloop( SND_LASERBARRIER, lasbmsndvol, &lasbmsndchan );
      } else {
        if( i != lasbmsndvol ) setvol( lasbmsndchan, lasbmsndvol );
      }
    } else {
      if( lasbmsndchan != -1 )
        stopchannel( lasbmsndchan );
    }
  } else {
    if( lasbmsndchan != -1 )
      stopchannel( lasbmsndchan );
  }

  switch( laserstate )
  {
    case 1:
      lasercut = 566;
      lasertime = 120;
      laserstate++;
      break;
    
    case 2:
      if( lasertime > 0 )
      {
        lasertime--;
        break;
      }

      startincidental( 1654, 501, -1, -1, GSPRITEX, bangframes, 5, 2 );
      startincidental( 1679, 501,  1, -1, GSPRITEX, bangframes, 5, 2 );
      startincidental( 1679, 512,  1,  1, GSPRITEX, bangframes, 5, 2 );
      startincidental( 1654, 512, -1,  1, GSPRITEX, bangframes, 5, 2 );
      incidentalsound( SND_EXPLOS, sfxvol );
      incidentalsound( SND_EXPLOS, sfxvol );
      set_blocker( 99, 31, 1, 5, 0 );
      laserstate++;
      break;
  }
}

void renderlaserbeam( void )
{
  int fn;

  fn = ((frame>>1)&1);

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );

  if( ( laserstate > 0 ) && ( laserstate < 3 ) )
  {
    render_sprite_tl( &psprt5[59+fn], 1594-fgx, 542-fgy, TRUE );
    render_sprite_tl( &psprt5[59+fn], 1620-fgx, 516-fgy, TRUE );
    render_sprite_tl_clipyb( &psprt5[59+fn], 1646-fgx, 490-fgy, 505-fgy, TRUE );
  }
  
  if( laserstate < 3 )
  {
    render_sprite_tl_clipy( &psprt5[15+fn], 1588-fgx, 532-fgy, lasercut-fgy, FALSE );
    render_sprite_tl( &psprt5[11+fn], 1587-fgx, 529-fgy, FALSE );
    render_sprite_tl( &psprt5[17+fn], 1606-fgx, 503-fgy, FALSE );
    render_sprite_tl( &psprt5[13+fn], 1638-fgx, 503-fgy, FALSE );
    render_sprite_tl( &psprt5[10],    1644-fgx, 496-fgy, FALSE );
  }
  render_sprite_tl( &psprt5[9],     1568-fgx, 496-fgy, FALSE );

  if( laserstate > 0 )
  {
    glBindTexture( GL_TEXTURE_2D, tex[GSPRITEX] );
    render_sprite_tl( &sprtg[75], 1583-fgx, 562-fgy, FALSE );

    if( laserstate < 3 )
    {
      glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
       render_sprite_tl( &psprt5[61+fn], 1587-fgx, 564-fgy, FALSE );
    }
  }
}

BOOL triggerlaserbeam( void )
{
  if( ( clevel != 5 ) || ( winfo != 2 ) )
    return FALSE;

  infos5[2].active = FALSE;
  inv[INV_MIRROR] = 0;
  strig[ST5_MIRROR_PLACED] = 1;
  laserstate = 1;
  actionsound( SND_HUZZAH, sfxvol );

  return TRUE;
}

/********************** BIG SCREEN *************************/

int bsdooroffs;

void initbigscreen( void )
{
/*
#ifdef __amigaos4__
  sbrinf.Memory      = screentex;
  sbrinf.BytesPerRow = 256*4;
  sbrinf.RGBFormat   = RGBFB_R8G8B8A8;
#endif
*/
  if( strig[ST5_CCTV_DISABLED] )
  {
    p_things5[2].active = TRUE;
    set_blocker( 40, 60, 1, 2, 0 );
    infos5[3].active = FALSE;
    return;
  }    

  p_things5[2].active = FALSE;
  bsdooroffs = 0;
  set_blocker( 40, 60, 1, 2, 255 );
  infos5[3].active = TRUE;
}

void animatebigscreen( void )
{
  if( ( strig[ST5_CCTV_DISABLED] ) ||
      ( gid.x > (768<<8) ) )
  {
    if( bsdooroffs < 48 )
      bsdooroffs+=3;
  } else {
    if( bsdooroffs > 0 )
      bsdooroffs-=3;
  }
}

// 812,908 -> 884,964   = 72,56
void renderbigscreen( void )
{
  int x, y;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl( &psprt5[3], 642-fgx, (927-bsdooroffs/2)-fgy, FALSE );
  render_sprite_tl( &psprt5[2], 642-fgx, (973+bsdooroffs/2)-fgy, FALSE );

  if( ( fgx > 884 ) ||
      ( fgx < (812-320) ) ||
      ( fgy > 964 ) ||
      ( fgy < (908-240) ) )
  {
//    glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
//    render_sprite_tl( &psprt5[15], 64, 64, FALSE );
    return;
  }

  x = 812-fgx;
  y = 908-fgy;

  if( strig[ST5_CCTV_DISABLED] )
  {
    glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
    render_sprite_tl( &sprt5[51], x, y, FALSE );
    return;
  }

  glBindTexture( GL_TEXTURE_2D, tex[SCREENTEX] );
/*
#ifdef __amigaos4__
  if( screenbodge )
  {
    glLoadIdentity();
    glBegin( GL_QUADS );
    glColor4ub(255, 255, 255, 255);
      glTexCoord2f(         0.0f,         0.0f ); glVertex3f( x   , y   , 0.0f );
      glTexCoord2f( 84.0f/128.0f,         0.0f ); glVertex3f( x+72, y   , 0.0f );
      glTexCoord2f( 84.0f/128.0f, 64.0f/128.0f ); glVertex3f( x+72, y+56, 0.0f );
      glTexCoord2f(         0.0f, 64.0f/128.0f ); glVertex3f( x   , y+56, 0.0f );
    glEnd();
    return;
  }
#endif
*/
  glLoadIdentity();
  glBegin( GL_QUADS );
  glColor4ub(255, 255, 255, 255);
    glTexCoord2f(         0.0f, 64.0f/128.0f ); glVertex3f( x   , y   , 0.0f );
    glTexCoord2f( 84.0f/128.0f, 64.0f/128.0f ); glVertex3f( x+72, y   , 0.0f );
    glTexCoord2f( 84.0f/128.0f,         0.0f ); glVertex3f( x+72, y+56, 0.0f );
    glTexCoord2f(         0.0f,         0.0f ); glVertex3f( x   , y+56, 0.0f );
  glEnd();
}

BOOL triggerbigscreen1( void )
{
  if( ( clevel != 5 ) || ( winfo != 3 ) )
    return FALSE;

  strig[ST5_PHOTO_TAKEN] = TRUE;
  inv[INV_DIGITALCAMERA] = 0;
  inv[INV_CAMERAWITHPHOTO] = 1;
  fadea = 255;
  fadeadd = -24;
  fadetype = 1;
  giddy_say( "Ive taken a photo\n"
             "of the scene..   " );
  actionsound( SND_CLICKYCLICK, sfxvol );
  return TRUE;
}

BOOL triggerbigscreen2( void )
{
  if( ( clevel != 5 ) || ( winfo != 3 ) )
    return FALSE;

  strig[ST5_CCTV_DISABLED] = TRUE;
  p_things5[2].active = TRUE;
  set_blocker( 40, 60, 1, 2, 0 );
  giddy_say( "Huzzah!" );
  actionsound( SND_HUZZAH, sfxvol );
  inv[INV_PRINTEDPHOTO] = 0;
  infos5[3].active = FALSE;
  return TRUE;
}

void updatebigscreentex( void )
{
  int wl, wt;

  if( clevel != 5 ) return;

  if( strig[ST5_CCTV_DISABLED] )
    return;

// 812,908 -> 884,964   = 72,56
  if( ( fgx > 884 ) ||
      ( fgx < (812-320) ) ||
      ( fgy > 964 ) ||
      ( fgy < (908-240) ) )
    return;
/*
#ifdef __amigaos4__
  if( screenbodge )
  {
    wl = ((gid.px-fgx)+win->BorderLeft)+84;
    wt = ((gid.py-fgy)+win->BorderTop)+24;
    IP96->p96ReadPixelArray( &sbrinf, 0, 0, win->RPort, wl, wt, 84*2, 64*2 );
    bodgeneedupdate = TRUE;
    return;
  }
#endif
*/
  wl = (gid.px-fgx)+84;
  wt = (gid.py-fgy)-24;

  glBindTexture( GL_TEXTURE_2D, tex[SCREENTEX] );
  glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, wl, wt, 84*2, 64*2 );
}
/*
#ifdef __amigaos4__
void updatebodge( void )
{
  if( !bodgeneedupdate ) return;
  bodgeneedupdate = FALSE;
  glBindTexture( GL_TEXTURE_2D, tex[SCREENTEX] );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, screentex );
}
#endif
*/
/********************* TELEPORTER BUBBLES *************************/

#define NUM_TPBUBS 8
struct tpbub
{
  int x, y, f, ys, fc, fs;
};

struct tpbub tpbubs[NUM_TPBUBS];

void resettpbub( int i )
{
  tpbubs[i].x = (rand()%14)+914;
  tpbubs[i].y = 2440<<8;
  tpbubs[i].ys = ((rand()%5)+1)<<7;
  tpbubs[i].f = 71;
  tpbubs[i].fs = (rand()%4)+4;
  tpbubs[i].fc = 0;
}

void inittpbubs( void )
{
  int i;

  for( i=0; i<NUM_TPBUBS; i++ )
    resettpbub( i );
}

void animatetpbubs( void )
{
  int i;

  for( i=0; i<NUM_TPBUBS; i++ )
  {
    tpbubs[i].y -= tpbubs[i].ys;
    tpbubs[i].fc++;
    if( tpbubs[i].fc > tpbubs[i].fs )
    {
      tpbubs[i].f++;
      tpbubs[i].fc = 0;
      if( tpbubs[i].f > 75 )
        resettpbub( i );
    }
  }
}

void rendertpbubs( void )
{
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; i<NUM_TPBUBS; i++ )
    render_sprite( &sprt5[tpbubs[i].f], tpbubs[i].x-fgx, (tpbubs[i].y>>8)-fgy, FALSE, 0.0f );
}

/************************** BILE DUDE *****************************/

#define NUM_BILE_BLOBS 16
struct bile
{
  int x, y, dx, dy, f, fc, d;
};

struct bile blb[NUM_BILE_BLOBS];

void resetbileblob( int i, int d )
{
  blb[i].d = d;
  blb[i].x = 1226<<8;
  blb[i].y = 2386<<8;
  blb[i].dx = ((rand()%6)+14)<<6;
  blb[i].dy = 0;
  blb[i].f = 32;
  blb[i].fc = 0;
}

void initbiledude( void )
{
  int i;

  if( strig[ST5_INDIGESTION_CURED] )
  {
    for( i=0; i<NUM_BILE_BLOBS; i++ )
      blb[i].d = -1;
    set_blocker( 74, 150, 1, 6, 0 );
    infos5[4].active = FALSE;
    return;
  }

  for( i=0; i<NUM_BILE_BLOBS; i++ )
    resetbileblob( i, i*2 );
  set_blocker( 74, 150, 1, 6, 255 );
  infos5[4].active = TRUE;
}

void animatebiledude( void )
{
  int i;

  for( i=0; i<NUM_BILE_BLOBS; i++ )
  {
    if( blb[i].d == -1 )
      continue;
    if( blb[i].d > 0 )
    {
      blb[i].d--;
      continue;
    }
    blb[i].x -= blb[i].dx;
    blb[i].y += blb[i].dy;
    if( blb[i].dx > 0 )
    {
      blb[i].dx -= 0x40;
      if( blb[i].dx < 0 ) blb[i].dx = 0;
    }
    if( blb[i].dy < 0x400 )
      blb[i].dy += 0x60;
    blb[i].fc++;
    if( blb[i].fc >= 4 )
    {
      blb[i].fc = 0;
      blb[i].f++;
      if( blb[i].f > 35 )
        blb[i].f = 32;
    }

    if( blb[i].y >= (2492<<8) )
    {
      if(( i&3 )==0)
        startincidental( blb[i].x>>8, blb[i].y>>8, 0, 0, GSPRITEX, bangframes, 5, 2 );
      if( strig[ST5_INDIGESTION_CURED] )
        blb[i].d = -1;
      else
        resetbileblob( i, 0 );
    }

    if( giddydanger( PSPRITEX, 32, blb[i].x>>8, blb[i].y>>8, FALSE ) )
      giddyhit();
  }
}

void renderbiledude( void )
{
  int i;
  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );

  for( i=0; i<NUM_BILE_BLOBS; i++ )
  {
    if( blb[i].d == -1 )
      continue;
    render_sprite( &psprt5[blb[i].f], (blb[i].x>>8)-fgx, (blb[i].y>>8)-fgy, FALSE, 0.0f );
  }

  if( strig[ST5_INDIGESTION_CURED] )
  {
    render_sprite_tl( &psprt5[31], 1212-fgx, 2370-fgy, FALSE );
    return;
  }

  render_sprite_tl( &psprt5[30], 1212-fgx, 2370-fgy, FALSE );
}

BOOL triggerbiledude( void )
{
  if( ( clevel != 5 ) || ( winfo != 4 ) )
    return FALSE;

  strig[ST5_INDIGESTION_CURED] = 1;
  inv[INV_INDIGESTIONPILLS] = 0;
  set_blocker( 74, 150, 1, 6, 0 );
  infos5[4].active = FALSE;
  giddy_say( "Huzzah!" );
  actionsound( SND_HUZZAH, sfxvol );
  return TRUE;
}

/************************* TIMER DOOR *****************************/

#define TIMERDOORTIMEOUT (13*60)
int timerdoortime;
int timerdoory;

void inittimerdoor( void )
{
  timerdoortime = 0;
  timerdoory    = 1056;

  p_things3[15].y = timerdoory+9;
  p_things3[16].y = timerdoory;
  p_things3[17].y = timerdoory;
  p_things3[18].y = timerdoory+32;

  psprt3[37].fy   = 131;
  psprt3[37].y    = 131.0f/256.0f;
}

void animatetimerdoor( void )
{
  if( timerdoortime > 0 )
  {
    timerdoortime--;
    if( timerdoory == 1056 )
      incidentalsound( SND_STONEMOVE, sfxvol );
    if( timerdoory < 1088 )
    {
      timerdoory++;
      p_things3[15].y = timerdoory+9;
      p_things3[16].y = timerdoory;
      p_things3[17].y = timerdoory;
      p_things3[18].y = timerdoory+32;
    }

    psprt3[37].fy = 131 + ((timerdoortime*15)/TIMERDOORTIMEOUT);
    psprt3[37].y = ((float)psprt3[37].fy)/256.0f;
    return;
  }

  if( timerdoory == 1088 )
    incidentalsound( SND_STONEMOVE, sfxvol );

  if( timerdoory > 1056 )
  {
    timerdoory--;
    p_things3[15].y = timerdoory+9;
    p_things3[16].y = timerdoory;
    p_things3[17].y = timerdoory;
    p_things3[18].y = timerdoory+32;
  }

  psprt3[37].fy   = 131;
  psprt3[37].y    = 131.0f/256.0f;
}

void triggertimerdoor( void )
{
  timerdoortime = TIMERDOORTIMEOUT;
  actionsound( SND_BUTTON, sfxvol );
}

/************************ LOCK BLOCK ZAPPER ***********************/

#define LBZ_TURNSPEED 4
int lbz_state, lbz_y, lbz_f, lbz_wait;

void initlockblockzapper( void )
{
  lbz_y = 1076;
  lbz_state = 0;
  lbz_f = 0;  

  if( strig[ST3_DIAMOND_PLACED] )
  {
    p_things3[24].frames[0] = 42;
    infos3[12].active = FALSE;
    p_things3[22].active = FALSE;
    set_blocker( 212, 63, 2, 2, 100 );
    set_blocker( 214, 63, 1, 1, 100 );
    return;
  }

  p_things3[24].frames[0] = 41;
  infos3[12].active = TRUE;
  p_things3[22].active = TRUE;
  set_blocker( 212, 63, 2, 2, 86 );
  set_blocker( 214, 63, 1, 1, 253 );
}

void animatelockblockzapper( void )
{
  switch( lbz_state )
  {
    case 1:
      if( lbz_y > 1008 )
      {
        lbz_y -= 2;
        break;
      }
      lbz_state++;
      break;
    
    case 2:
      if( lbz_f < (3*LBZ_TURNSPEED) )
      {
        lbz_f++;
        break;
      }
      lbz_state++;
      lbz_wait = 6;
      break;
    
    case 3:
      if( lbz_wait > 0 )
      {
        lbz_wait--;
        break;
      }
      startincidental( 3400, 1016, 0, 0, GSPRITEX, bangframes, 5, 2 );
      startincidental( 3416, 1016, 0, 0, GSPRITEX, bangframes, 5, 2 );
      startincidental( 3400, 1032, 0, 0, GSPRITEX, bangframes, 5, 2 );
      startincidental( 3416, 1032, 0, 0, GSPRITEX, bangframes, 5, 2 );
      p_things3[22].active = FALSE;
      set_blocker( 212, 63, 2, 2, 100 );
      set_blocker( 214, 63, 1, 1, 100 );
      incidentalsound( SND_EXPLOS, sfxvol );
      lbz_state++;
      break;
    
    case 4:
      if( lbz_f > 0 )
      {
        lbz_f--;
        break;
      }
      lbz_state++;
      break;
    
    case 5:
      if( lbz_y < 1076 )
      {
        lbz_y += 2;
        break;
      }
      lbz_state++;
      break;
  }
}

void renderlockblockzapper( void )
{
  if( lbz_y >= 1076 )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl_clipy( &psprt3[45+(lbz_f/LBZ_TURNSPEED)], 3302-fgx, lbz_y-fgy, 1073-fgy, FALSE );  
  render_sprite_tl_clipy( &psprt3[43], 3302-fgx, (lbz_y+31)-fgy, 1073-fgy, FALSE );  
  render_sprite_tl_clipy( &psprt3[44], 3302-fgx, (lbz_y+47)-fgy, 1073-fgy, FALSE );  
  render_sprite_tl_clipy( &psprt3[44], 3302-fgx, (lbz_y+63)-fgy, 1073-fgy, FALSE );  

  if( lbz_f == (3*LBZ_TURNSPEED) )
    render_sprite_tl_stretch( &psprt3[49], 3333-fgx, (lbz_y+12)-fgy, 59, 4, FALSE );
}

BOOL triggerlockblockzapper( void )
{
  if( ( clevel != 3 ) || ( winfo != 12 ) )
    return FALSE;

  inv[INV_DIAMOND] = 0;
  p_things3[24].frames[0] = 42;
  infos3[12].active = FALSE;
  actionsound( SND_HUZZAH, sfxvol );
  lbz_state = 1;
  return TRUE;
}

/************************ WALL STEPPING STONES ********************/

struct wsstime
{
  // Set
  int delay, ontime, offtime;

  // Calculated
  int dcount, count, state;
};

struct wsstime wsst[] = { { 0*60, 3*60, 2*60 },
                          { 2*60, 3*60, 2*60 },
                          { 4*60, 3*60, 2*60 },
                          { 0*60, 3*60, 2*60 },
                          { 2*60, 3*60, 3*60 } };

void initwallsteppingstones( void )
{
  int i;
  for( i=0; i<5; i++ )
  {
    wsst[i].dcount = wsst[i].delay;
    wsst[i].count  = 0;
    wsst[i].state  = 0;
    n_things3[i+18].frames[0] = 49;
    n_things3[i+18].flags &= ~THF_BLOCKFALL;
  }
}

void animatewallsteppingstones( void )
{
  int i, j;
  int wsnd;

  wsnd = 0;
  for( i=0; i<5; i++ )
  {
    if( wsst[i].dcount > 0 )
    {
      wsst[i].dcount--;
      continue;
    }

    j = n_things3[i+18].frames[0];
    if( wsst[i].state )
    {
      if( j > 44 )
      {
        j--;
        n_things3[i+18].frames[0] = j;
        if( j < 46 ) n_things3[i+18].flags |= THF_BLOCKFALL;
      }
    } else {
      if( j < 49 )
      {
        j++;
        n_things3[i+18].frames[0] = j;
        if( j > 45 ) n_things3[i+18].flags &= ~THF_BLOCKFALL;
      }
    }

    if( wsst[i].count > 0 )
    {
      wsst[i].count--;
      continue;
    }

    wsst[i].state ^= 1;
    wsst[i].count = wsst[i].state ? wsst[i].ontime : wsst[i].offtime;
    if( ( n_things3[i+18].x >= fgx ) &&
        ( n_things3[i+18].x < (fgx+342) ) &&
        ( n_things3[i+18].y >= fgy ) &&
        ( n_things3[i+18].y < (fgy+262) ) )
      wsnd = 1;
  }

  if( wsnd )
    incidentalsound( SND_WHOOSH, sfxvol );
}

/************************ GUM MACHINE *****************************/

#define NUMGUMBALLS 32
struct gumball
{
  int x, y, f;
  int dx, dy, bcd, del;
};

struct gumball gumb[NUMGUMBALLS];
int gumbstate, gumbtime;

void initgummachine( void )
{
  if( strig[ST3_GUM_BOUGHT] )
  {
    g_things3[20].active = !strig[ST3_GUM_COLLECTED];
    infos3[9].active = FALSE;
    gumbstate = 5;
    return;
  }

  g_things3[20].active = FALSE;
  infos3[9].active = TRUE;
  gumbstate = 0;
}

void resetgumball( int i )
{
  gumb[i].x = 6831<<8;
  gumb[i].y = 1072<<8;
  gumb[i].f = (rand()%7)+50;
  gumb[i].dx = ((rand()%5)-2) * 0x40;
  gumb[i].dy = 0;
  gumb[i].bcd = 0;
}

BOOL movegumballs( void )
{
  int i;
  BOOL any;

  any = FALSE;
  for( i=0; i<NUMGUMBALLS; i++ )
  {
    if( gumb[i].del > 0 )
    {
      if( gumbstate == 4 )
        continue;

      any = TRUE;
      gumb[i].del--;
      continue;
    }

    any = TRUE;
    gumb[i].x += gumb[i].dx;
    gumb[i].y += gumb[i].dy;
    gumb[i].dy += 0x40;
    if( !gumb[i].bcd )
    {
      if( gumb[i].y >= (1088<<8) )
      {
        gumb[i].y = (1088<<8);
        gumb[i].dy = -0x200;
        gumb[i].bcd = 1;
      }
    } else {
      if( gumb[i].y >= (1104<<8) )
      {
        if( gumbstate == 4 )
          gumb[i].del = 1;
        else
          resetgumball( i );
      }
    }
  }

  return any;
}

void animategummachine( void )
{
  int i;

  switch( gumbstate )
  {
    case 1:
      for( i=0; i<NUMGUMBALLS; i++ )
      {
        gumb[i].del = i;
        resetgumball( i );
      }
      gumbstate++;
      gumbtime = 60;
    
    case 2:
      movegumballs();
      if( gumbtime > 0 )
      {
        gumbtime--;
        break;
      }

      g_things3[20].active = TRUE;
      gumbstate++;
      gumbtime = 120;
      break;
      
    case 3:
      movegumballs();
      if( gumbtime > 0 )
      {
        gumbtime--;
        break;
      }
      gumbstate++;
      break;
    
    case 4:
      if( !movegumballs() )
        gumbstate++;
      break;
  }
}

void rendergummachine( void )
{
  int i;

  if( ( gumbstate < 1 ) || ( gumbstate > 4 ) )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; i<NUMGUMBALLS; i++ )
  {
    if( gumb[i].del > 0 ) continue;
    render_sprite( &psprt3[gumb[i].f], (gumb[i].x>>8)-fgx, (gumb[i].y>>8)-fgy, FALSE, 0.0f );
  }
}

BOOL triggergummachine( void )
{
  if( ( clevel != 3 ) || ( winfo != 9 ) )
    return FALSE;

  if( gid.coins < 250 )
  {
    giddy_say( "I haven't got enough. \nIt requires 250 coins." );
    return TRUE;
  }

  gid.coins -= 250;
  if( gid.coins == 0 ) inv[INV_COINS] = 0;

  gumbstate = 1;
  strig[ST3_GUM_BOUGHT] = 1;
  infos3[9].active = FALSE;
  actionsound( SND_HUZZAH, sfxvol );
  return TRUE;
}

/********************** CYCLING ALIEN *********************/

int cyalbubtimer, cyalstate;
int cycsndchan = -1, cycsndvol = 0;

void initcyclingalien( void )
{
  if( strig[ST5_BATTERY_CHARGED] )
  {
    cyalstate = 1;
    p_things5[18].active = !strig[ST5_BATTERY_COLLECTED];
    p_things5[19].active = !strig[ST5_BATTERY_COLLECTED];
    infos5[5].active = FALSE;
    return;
  }

  cyalstate = 0;
  cyalbubtimer = -1;
  p_things5[18].active = FALSE;
  p_things5[19].active = FALSE;
  infos5[5].active = TRUE;
}

void animatecyclingalien( void )
{
  int i;

  i = cycsndvol;
  if( fgy < 2280 )
  {
    cycsndvol = 0;
  } else {
    cycsndvol = 1000-abs(gid.px-2532)*2;
    if( cycsndvol < 0 ) cycsndvol = 0;

    cycsndvol = (cycsndvol*sfxvol)/1000;
  }

  if( cycsndvol > 0 )
  {
    if( cycsndchan == -1 )
    {
      ambientloop( SND_CYCLINGALIEN, cycsndvol, &cycsndchan );
    } else {
      if( i != cycsndvol ) setvol( cycsndchan, cycsndvol );
    }
  } else {
    if( cycsndchan != -1 )
      stopchannel( cycsndchan );
  }

  switch( cyalstate )
  {
    case 0:
      if( cyalbubtimer > 0 )
      {
        cyalbubtimer--;
      } else if( cyalbubtimer == 0 ) {
        if( winfo != 5 )
          cyalbubtimer = -1;
      }
      break;
  }
}

void triggercyclingalienbubble( void )
{
  if( cyalstate != 0 ) return;
  if( cyalbubtimer != -1 ) return;

  cyalbubtimer = 120;
}

void rendercyclingalien( void )
{
  if( cyalbubtimer > 0 )
  {
    glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
    render_sprite_tl( &psprt5[55], 2496-fgx, 2410-fgy, FALSE );
  }
}

BOOL triggercyclingalien( void )
{
  if( ( clevel != 5 ) || ( winfo != 5 ) )
    return FALSE;

  strig[ST5_BATTERY_CHARGED] = 1;
  p_things5[18].active = TRUE;
  p_things5[19].active = TRUE;
  infos5[5].active = FALSE;
  inv[INV_FLATBATTERY] = 0;
  actionsound( SND_HUZZAH, sfxvol );
  return TRUE;
}

/********************** SPRINKLER *********************/

#define NUMDROPLETS 256
int dp_xo[] = { 16, 15, 13, 10, 6, 3, 1, 3, 6, 10, 13, 15 };
int dp_state;

int spksndchan = -1, spksndvol = 0;

int bossx, bossy, bosshealth, bossmode, bosstimer, bossjump, bossdy, bossflash;
int bossdanger;

int bootx, booty, bootdy;

struct droplet
{
  int del;
  int x, y;
  int dx, dy;
};

struct droplet dplets[NUMDROPLETS];

void initsprinkler( void )
{
  if( strig[ST1_HOSE_PLACED] )
  {
    n_things1[70].active = TRUE;
    infos1[0].active = FALSE;
    infos1[1].active = FALSE;
    p_things1[2].numframes = 6;
    p_things1[1].active = !strig[ST1_BOSS_BEATEN];

    if( ( strig[ST1_BOOT_COLLECTED] ) || ( !strig[ST1_BOSS_BEATEN] ) )
    {
      p_things1[23].active = FALSE;
    } else {
      p_things1[23].active = TRUE;
      p_things1[23].x = 2840;
      p_things1[23].y = 392;
    }

    dp_state = 1;
    bossmode = 0;
    infos1[5].active = FALSE;
    return;
  }

  n_things1[70].active = FALSE;
  infos1[0].active = TRUE;
  infos1[1].active = TRUE;
  p_things1[2].numframes = 1;
  p_things1[1].active = TRUE;
  p_things1[23].active = FALSE;
  infos1[5].active = FALSE;
  dp_state = 0;
  bossmode = 0;
}

void resetdroplet( int i )
{
  int o;

  o = dp_xo[(p_things1[2].frame<<1)|((p_things1[2].framecount&2)>>1)];
  if( i&1 ) o = -o;

  dplets[i].x  = (2725+o)<<8;
  dplets[i].y  = 382<<8;
  dplets[i].dx = o*48;
  dplets[i].dy = -0x640;
}

void animatesprinkler( void )
{
  int i;

  switch( bossmode )
  {
    case 1:
      if( bossy > (306<<8) )
      {
        bossy -= 0x100;
        break;
      }
      bossmode = (gid.x < (bossx+(28<<8))) ? 2 : 3;
      bosstimer = 0;
      bossflash = 0;
      bossdanger = 0;
      break;
    
    case 2:
      bossx -= 0x80;

      if( bossx < (2800<<8) )
      {
        bossx = (2800<<8);
        bossjump = 0;
        bosstimer = 10;
        bossmode = 3;
        break;
      }

      if( bossjump )
      {
        bossy += bossdy;
        bossdy += 0x20;
        if( bossdy > 0x600 )
          bossdy = 0x600;
        if( bossy > (306<<8) )
        {
          bossy = (306<<8);
          bossjump = 0;
          bosstimer = 20;
          stuff.quake = 32;
          enemysound( SND_SPUDBOSSLAND, sfxvol );
        }
        break;
      }

      if( bosstimer > 0 )
      {
        bosstimer--;
        break;
      }

      bossjump = 1;
      bossdy = -0x400;
      break;

    case 3:
      bossx += 0x80;

      if( bossx > (2928<<8) )
      {
        bossx = (2928<<8);
        bossjump = 0;
        bosstimer = 10;
        bossmode = 2;
        break;
      }

      if( bossjump )
      {
        bossy += bossdy;
        bossdy += 0x20;
        if( bossdy > 0x600 )
          bossdy = 0x600;
        if( bossy > (306<<8) )
        {
          bossy = (306<<8);
          bossjump = 0;
          bosstimer = 20;
          stuff.quake = 32;
          enemysound( SND_SPUDBOSSLAND, sfxvol );
        }
        break;
      }

      if( bosstimer > 0 )
      {
        bosstimer--;
        break;
      }

      bossjump = 1;
      bossdy = -0x400;
      break;
    
    case 4:
      booty += bootdy;
      bootdy += 0x80;
      if( bootdy > 0x600 )
        bootdy = 0x600;
      if( booty > (392<<8) )
      {
        booty = 392<<8;
        bossmode++;
        p_things1[23].active = TRUE;
        p_things1[23].x = bootx>>8;
        p_things1[23].y = booty>>8;
      }
      break;
  }

  if( ( ( bossmode == 2 ) || ( bossmode == 3 ) ) &&
      ( bossflash == 0 ) )
  {
    if( giddydanger( PSPRITEX, 8, ((bossx>>8)+28), ((bossy>>8)+40), FALSE ) )
    {
      if( bossdanger )
      {
        giddyhit();
      } else {
        if( gid.y < (bossy+(10<<8)) )
        {
          bosshealth--;
          if( bosshealth == 0 )
          {
            bossmode = 4;
            stuff.bossmode = FALSE;
            strig[ST1_BOSS_BEATEN] = 1;
            infos1[5].active = FALSE;
            bootx = bossx + (28<<8);
            booty = bossy + (40<<8);
            bootdy = -0x400;
            startincidental( (bossx>>8)+16, (bossy>>8)+20, -1, -1, GSPRITEX, bangframes, 5, 2 );
            startincidental( (bossx>>8)+41, (bossy>>8)+20,  1, -1, GSPRITEX, bangframes, 5, 2 );
            startincidental( (bossx>>8)+41, (bossy>>8)+60,  1,  1, GSPRITEX, bangframes, 5, 2 );
            startincidental( (bossx>>8)+16, (bossy>>8)+60, -1,  1, GSPRITEX, bangframes, 5, 2 );
            enemysound( SND_EXPLOS, sfxvol );
            enemysound( SND_SPUDBOSSDIE, sfxvol );
          } else {
            enemysound( SND_EXPLOS, sfxvol );
          }
          bossflash=60;
        } else {
          giddyhit();
          bossdanger = 1;
        }
      }
    } else {
      bossdanger = 0;
    }
  }

  if( bossflash > 0 )
    bossflash--;

  i = spksndvol;
  if( dp_state == 0 )
  {
    spksndvol = 0;
  } else {
    spksndvol = 1000-abs(gid.px-2704)*2;
    if( spksndvol < 0 ) spksndvol = 0;
    if( gid.y > (440<<8) ) spksndvol = 0;

    spksndvol = (spksndvol*sfxvol)/1000;
  }

  if( spksndvol > 0 )
  {
    if( spksndchan == -1 )
    {
      ambientloop( SND_SPRINKLER, spksndvol, &spksndchan );
    } else {
      if( i != spksndvol ) setvol( spksndchan, spksndvol );
    }
  } else {
    if( spksndchan != -1 )
      stopchannel( spksndchan );
  }

  switch( dp_state )
  {
    case 1:
      for( i=0; i<NUMDROPLETS; i++ )
      {
        dplets[i].del = i/2;
        dplets[i].x = -1;
      }
      dp_state++;
    
    case 2:
      for( i=0; i<NUMDROPLETS; i++ )
      {
        if( dplets[i].del > 0 )
        {
          dplets[i].del--;
          continue;
        }

        if( dplets[i].x == -1 )
        {
          resetdroplet( i );
          continue;
        }

        dplets[i].x += dplets[i].dx;
        dplets[i].y += dplets[i].dy;
        if( dplets[i].dy < 0x600 )
          dplets[i].dy += 0x30;

        if( dplets[i].y > (406<<8) )
          resetdroplet( i );
      }
      break;
  }
}

void renderboss( void )
{
  int x, y, exo, wl, wr;

  if( bossmode < 1 )
    return;

  if( bossmode == 4 )
  {
    glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
    render_sprite( &psprt1[31], (bootx>>8)-fgx, (booty>>8)-fgy, FALSE, 0.0f );
    return;
  }

  if( bossmode > 4 )
    return;

  if( ( bossflash ) && ( frame&2 ) )
    return;

  x = (bossx>>8)-fgx;
  y = (bossy>>8)-fgy;

  exo = (gid.x-(bossx+(26<<8)))/2048;
  if( exo < -3 ) exo = -3;
  if( exo > 3 ) exo = 3;

  if( ( bossjump ) || ( bossmode < 2 ) )
  {
    wl = wr = 3;
  } else {
    wl = ((frame>>3)&1) * 5;
    wr = 5 - wl;
  }
  
  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl( &psprt1[9], x-10, y+50+wl, FALSE );
  render_sprite_tl( &psprt1[9], x+48, y+50+wr, TRUE );
  render_sprite_tl( &psprt1[10], x+1, y+72+wl, FALSE );
  render_sprite_tl( &psprt1[10], x+27, y+72+wr, TRUE );
  render_sprite_tl( &psprt1[8], x, y, FALSE );
  render_sprite_tl( &psprt1[33], x+exo+15, y+39, FALSE );
  render_sprite_tl( &psprt1[33], x+exo+33, y+39, FALSE );
}

void rendersprinkler( void )
{
  int i;
  if( strig[ST1_HOSE_PLACED] )
  {
    glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
    render_sprite_tl_stretch( &sprt1[84], 2464-fgx, 399-fgy, 240, 5, FALSE );

    glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
    for( i=0; i<NUMDROPLETS; i++ )
      render_sprite( &psprt1[38], (dplets[i].x>>8)-fgx, (dplets[i].y>>8)-fgy, FALSE, 0.0f );
  }
}

BOOL triggersprinkler( void )
{
  if( ( clevel != 1 ) || ( winfo != 0 ) )
    return FALSE;

  n_things1[70].active = TRUE;
  actionsound( SND_HUZZAH, sfxvol );
  inv[INV_HOSEPIPE] = 0;
  strig[ST1_HOSE_PLACED] = 1;
  infos1[0].active = FALSE;
  infos1[1].active = FALSE;
  p_things1[2].numframes = 6;
  dp_state = 1;
  return TRUE;
}

void triggerpotatoboss( void )
{
  stuff.bossmode = 1;
  stuff.bossmodeleft = 2672;
  stuff.bossmoderight = 3088;
  p_things1[1].active = FALSE;
  infos1[5].active = TRUE;
  bossx = 2878<<8;
  bossy = 392<<8;
  bosshealth = 6;
  bossmode = 1;
}

/************************ FLASHY BUTTONS *********************/

void initflashybuttons( void )
{
  p_things5[4].numframes = strig[ST5_GREEN_PRESSED] ? 2 : 1;
  p_things5[22].active = strig[ST5_GREEN_PRESSED];
  p_things5[6].numframes = strig[ST5_RED_PRESSED] ? 2 : 1;
  p_things5[21].active = strig[ST5_RED_PRESSED];
  p_things5[8].numframes = strig[ST5_BLUE_PRESSED] ? 2 : 1;
  p_things5[23].active = strig[ST5_BLUE_PRESSED];
}

/************************* PRINTER ***************************/

int paper_y, photo_y;
int printer_state;

void initprinter( void )
{
  if( strig[ST2_PHOTO_PRINTED] )
  {
    paper_y = 590<<8;
    photo_y = 612<<8;
    printer_state = 3;
    p_things2[107].active = !strig[ST2_PHOTO_COLLECTED];
    infos2[7].active = FALSE;
    return;
  }

  paper_y = 576<<8;
  photo_y = 572<<8;
  printer_state = 0;
  infos2[7].active = TRUE;
  p_things2[107].active = FALSE;
}

void animateprinter( void )
{
  switch( printer_state )
  {
    case 1:
      paper_y+=0x20;
      if( paper_y >= (590<<8) )
        paper_y = (590<<8);
      photo_y+=0x20;
      if( photo_y >= (600<<8) )
        printer_state++;
      break;
    
    case 2:
      photo_y += 0x100;
      if( photo_y >= (612<<8) )
      {
        photo_y = (612<<8);
        p_things2[107].active = TRUE;
        printer_state++;
      }
      break;
  }
}

void renderprinter( void )
{
  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl( &psprt2[70], 1040-fgx, 600-fgy, FALSE ); // Printer base
  render_sprite_tl( &psprt2[72], 1061-fgx, 573-fgy, FALSE ); // Paper holder
  render_sprite_tl( &psprt2[73], 1061-fgx, (paper_y>>8)-fgy, FALSE ); // Paper
  if( printer_state != 3 )
    render_sprite_tl_clipyb( &psprt2[74], 1061-fgx, (photo_y>>8)-fgy, 600-fgy, FALSE ); // Photo 
  render_sprite_tl( &psprt2[71], 1055-fgx, 590-fgy, FALSE ); // Printer
}

BOOL triggerprinter( void )
{
  if( ( clevel != 2 ) || ( winfo != 7 ) )
    return FALSE;

  inv[INV_CAMERAWITHPHOTO] = 0;
  printer_state = 1;
  actionsound( SND_HUZZAH, sfxvol );
  strig[ST2_PHOTO_PRINTED] = 1;
  infos2[7].active = FALSE;
  actionsound( SND_HUZZAH, sfxvol );

  incidentalloops( SND_PRINTER, sfxvol, 2 );
  return TRUE;
}

/***************** EGGSTERMINATOR PRODUCTION LINE ************/

struct bombblast
{
  int x, y, f;
};

int epl_state, epl_wait;
int epl_basey, epl_topy, epl_gframe, epl_bodyy;
int epl_armhx, epl_armx, epl_army;

int eplsndchan = -1, eplsndvol = 0;

struct bombblast bblasts[32];

void initeggsterminatorproductionline( void )
{
  epl_state  = 0;
  epl_basey  = 760;
  epl_topy   = 380;
  epl_gframe = 66;
  epl_bodyy  = 380;
  epl_armhx  = 68;
  epl_armx   = 68;
  epl_army   = 542;
}

void animateeggsterminatorproductionline( void )
{
  int i, psnd, hum;

  psnd = eplsndvol;

  eplsndvol = 0;
  hum = 0;

  switch( epl_state )
  {
    case 0:
      if( epl_basey > 548 )
      {
        if( ( epl_basey < (fgy+240) ) &&
            ( fgy < 800 ) )
          hum = 1;
        epl_basey -= 2;
        break;
      }

      if( ( fgx > 2080 ) &&
          ( fgy < 704 ) )
        incidentalsound( SND_WCLICK, sfxvol );
      epl_state++;
      epl_gframe = 66;
      break;
    
    case 1:
      if( epl_topy < 536 )
      {
        if( epl_topy > fgy ) hum = 1;
        epl_topy += 2;
        epl_bodyy += 2;
        break;
      }

      if( ( fgx > 2080 ) &&
          ( fgy < 704 ) )
        incidentalsound( SND_WCLICK2, sfxvol );
      epl_state++;
      epl_gframe = 67;
      break;
    
    case 2:
      if( epl_topy > 380 )
      {
        if( epl_topy > fgy ) hum = 1;
        epl_topy -= 2;
        break;
      }
      epl_state++;
      break;
    
    case 3:
      if( epl_armx > 18 )
      {
        epl_armx--;
        epl_armhx--;
        break;
      }
      epl_state++;
      if( ( fgx > 2080 ) &&
          ( fgy < 704 ) )
        incidentalsound( SND_WCLICK, sfxvol );
      break;
    
    case 4:
      if( epl_armhx < 68 )
      {
        epl_armhx++;
        break;
      }
      epl_state++;
      break;
    
    case 5:
      if( epl_basey < 760 )
      {
        if( fgy < 704 ) hum = 1;
        epl_basey += 2;
        epl_bodyy += 2;
        epl_army  += 2;
        break;
      }

      if( strig[ST5_BOMB_PLACED] )
      {
        actionsound( SND_BIGBANG, sfxvol );
        startincidental( 2560, 704, -2, -2, GSPRITEX, bangframes, 5, 5 );
        startincidental( 2560, 704, -1, -3, GSPRITEX, bangframes, 5, 5 );
        startincidental( 2560, 704,  0, -4, GSPRITEX, bangframes, 5, 5 );
        startincidental( 2560, 704,  1, -3, GSPRITEX, bangframes, 5, 5 );
        startincidental( 2560, 704,  2, -2, GSPRITEX, bangframes, 5, 5 );
        for( i=0; i<32; i++ )
          bblasts[i].x = -i;
        epl_state++;
        epl_wait = 60;
        break;
      }

      epl_state  = 0;
      epl_basey  = 760;
      epl_topy   = 380;
      epl_gframe = 66;
      epl_bodyy  = 380;
      epl_armhx  = 68;
      epl_armx   = 68;
      epl_army   = 542;
      break;
    
    case 6:
      if( epl_wait > 0 )
      {
        epl_wait--;
        break;
      }
      epl_state++;

    case 7:
      for( i=0; i<32; i++ )
      {
        if( bblasts[i].x < 0 )
        {
          bblasts[i].x++;
          continue;
        }

        if( ( bblasts[i].x == 0 ) || ( bblasts[i].f >= (5*4) ) )
        {
          bblasts[i].x = (rand()%320)+fgx;
          bblasts[i].y = (rand()%240)+fgy;
          bblasts[i].f = 0;
          if( ( rand()%5 ) == 0 )
            incidentalsound( SND_EXPLOS, sfxvol );
          continue;
        }

        bblasts[i].f++;
      }

      if( fadea < 255 )
      {
        if( frame&1 ) fadea++;
        break;
      }

      gid.savedtheday = TRUE;
      break;
  }

  if( hum )
  {
    eplsndvol = 1000-abs(gid.px-2704)*2;
    if( eplsndvol < 0 ) eplsndvol = 0;
    eplsndvol = (eplsndvol*sfxvol)/1000;
  }

  if( eplsndvol > 0 )
  {
    if( eplsndchan == -1 )
    {
      ambientloop( SND_LIFTLOOP, eplsndvol, &eplsndchan );
    } else {
      if( psnd != eplsndvol ) setvol( eplsndchan, eplsndvol );
    }
  } else {
    if( eplsndchan != -1 )
      stopchannel( eplsndchan );
  }

}

void rendereggsterminatorproductionlinebg( void )
{
  if( ( fgx < 2080 ) || 
      ( fgy > 704 ) )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl( &psprt5[63], 2536-fgx, epl_basey-fgy, FALSE );
  render_sprite_tl_clipy( &psprt5[64], 2547-fgx, (epl_basey+24)-fgy, 762-fgy, FALSE );
  render_sprite_tl_clipy( &psprt5[64], 2566-fgx, (epl_basey+24)-fgy, 762-fgy, FALSE );

  render_sprite( &psprt5[64], 2560-fgx, (epl_topy-96)-fgy, FALSE, 0.0f );
  render_sprite( &psprt5[65], 2560-fgx, epl_topy-fgy, FALSE, 0.0f );

  render_sprite( &psprt5[70], (2560-epl_armx)-fgx, epl_army-fgy, FALSE, 0.0f );
  render_sprite( &psprt5[71], (2560+epl_armx)-fgx, epl_army-fgy, FALSE, 0.0f );

  render_sprite( &psprt5[73], (2560-epl_armhx)-fgx, 502-fgy, FALSE, 0.0f );
  render_sprite( &psprt5[73], (2560+epl_armhx)-fgx, 502-fgy, FALSE, 0.0f );

  render_sprite( &psprt5[72], (2560-epl_armhx)-fgx, 522-fgy, FALSE, 0.0f );
  render_sprite( &psprt5[72], (2560+epl_armhx)-fgx, 522-fgy, FALSE, 0.0f );

  render_sprite( &psprt5[68], 2560-fgx, epl_bodyy-fgy, FALSE, 0.0f );
  
  render_sprite( &psprt5[epl_gframe], 2560-fgx, epl_topy-fgy, FALSE, 0.0f );

  if( !strig[ST5_BOMB_PLACED] )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  render_sprite( &sprt5[76], 2560-fgx, (epl_basey-2)-fgy, FALSE, 0.0f );
}

void rendereggsterminatorproductionline( void )
{
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite( &psprt5[69], (2560-epl_armhx)-fgx, 482-fgy, FALSE, 0.0f );
  render_sprite( &psprt5[69], (2560+epl_armhx)-fgx, 482-fgy, FALSE, 0.0f );

  if( epl_state != 7 )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );

  for( i=0; i<32; i++ )
  {
    if( bblasts[i].x <= 0 )
      continue;
    render_sprite( &sprt5[bblasts[i].f/4+31], bblasts[i].x-fgx, bblasts[i].y-fgy, FALSE, 0.0f );
  }
}

BOOL triggereggsterminatorproductionline( void )
{
  if( ( clevel != 5 ) || ( winfo != 6 ) )
    return FALSE;

  strig[ST5_BOMB_PLACED] = 1;
  actionsound( SND_HUZZAH, sfxvol );
  inv[INV_BOMB] = 0;

  return TRUE;
}

/****************** WHACKING GREAT BOMB **********************/

void initwhackinggreatbomb( void )
{
  if( strig[ST1_BOMB_DUG] )
  {
    p_things1[4].active = !strig[ST1_BOMB_COLLECTED];
    p_things1[4].flags  = THF_BEHIND|THF_FLASH|THF_COLLECTABLE;
    infos1[4].active = FALSE;
    fgmap[27*mapi->fgw+349] = 74;
    fgmap[27*mapi->fgw+350] = 0;
    fgmap[27*mapi->fgw+351] = 0;
    fgmap[27*mapi->fgw+352] = 70;
    fgmap[28*mapi->fgw+349] = 73;
    fgmap[28*mapi->fgw+350] = 74;
    fgmap[28*mapi->fgw+351] = 70;
    fgmap[28*mapi->fgw+352] = 71;
    fgmap[29*mapi->fgw+350] = 73;
    fgmap[29*mapi->fgw+351] = 71;
    return;
  }

  p_things1[4].active = TRUE;
  p_things1[4].flags  = THF_BEHIND;
  infos1[4].active = TRUE;
  set_blocker( 349, 27, 4, 1, 67 );
  set_blocker( 349, 28, 4, 2, 72 );
}

BOOL triggerwhackinggreatbomb( void )
{
  if( ( clevel != 1 ) || ( winfo != 4 ) )
    return FALSE;

  infos1[4].active = FALSE;
  strig[ST1_BOMB_DUG] = 1;
  inv[INV_SPADE] = 0;
  initwhackinggreatbomb();
  actionsound( SND_HUZZAH, sfxvol );

  return TRUE;
}

/*********************** NINJA *******************************/

int ninjastate, ninjay, ninjady, ninjaf, ninjacount;
BOOL ninjaface;

void initninja( void )
{
  ninjastate = 0;
  ninjay     = 406<<8;
}

void animateninja( void )
{
  int k;

  ninjaface = gid.x<(3232<<8) ? FALSE : TRUE;
  switch( ninjastate )
  {
    case 0:
      if( abs( gid.px-3232 ) < 160 )
      {
        ninjay  = 406<<8;
        ninjady = -0x700;
        ninjaf = 37;
        ninjastate++;
      }
      break;
    
    case 1:
    case 6:
      ninjay += ninjady;
      ninjady += 0x60;
      if( ninjady >= 0 )
        ninjastate++;
      break;
    
    case 2:
    case 7:
      ninjay += ninjady;
      ninjady += 0x60;
      if( ninjay >= (406<<8) )
      {
        ninjay = 406<<8;
        ninjacount = 9;
        ninjastate++;
      }
      break;
    
    case 3:
      if( giddydanger( SPRITEX, ninjaf, 3232, ninjay>>8, ninjaface ) )
        giddyhit();

      k = 38+((frame>>3)&1);
      if( k != ninjaf )
      {
        incidentalsound( SND_WHOOSH, sfxvol );
        ninjaf = k;
        ninjacount--;
        if( ninjacount <= 0 )
        {
          ninjaf = 40;
          ninjacount = 40;
          ninjastate++;
        }
      }
      break;
    
    case 4:
      if( ninjacount > 0 )
      {
        ninjacount--;
        break;
      }
      ninjaf = 41;
      ninjacount = 40;
      ninjastate++;
      break;

    case 5:
      if( ninjacount > 0 )
      {
        ninjacount--;
        break;
      }
      ninjaf = 37;
      ninjacount = 40;
      ninjady = -0x700;
      ninjastate++;
      break;

    case 8:
      if( abs( gid.px-3232 ) > 160 )
        ninjastate = 0;
      break;
  }
}

void renderninja( void )
{
  switch( ninjastate )
  {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
      render_sprite( &sprt4[ninjaf], 3232-fgx, (ninjay>>8)-fgy, ninjaface, 0.0f );
      break;
  }
}

void renderninjabg( void )
{
  switch( ninjastate )
  {
    case 1:
    case 7:
      glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
      render_sprite( &sprt4[ninjaf], 3232-fgx, (ninjay>>8)-fgy, ninjaface, 0.0f );
      break;
  }
}

/********************* SPECIAL BIN *******************************/

BOOL onthebin = FALSE;
int binstate, bincount, biny;

void initspecialbin( void )
{
  onthebin = FALSE;
  binstate = 0;
  bincount = 0;
  biny = 432;
  p_things4[8].y = 398;
}

void animatespecialbin( void )
{
  switch( binstate )
  {
    case 0:
      if( !onthebin )
      {
        bincount = 0;
        break;
      }

      bincount++;
      if( bincount >= 60 )
      {
        incidentalsound( SND_BINLIFT, sfxvol );
        binstate++;
      }
      break;
    
    case 1:
      biny -= 2;
      p_things4[8].y -= 2;
      if( p_things4[8].y <= 348 )
      {
        biny = 382;
        p_things4[8].y = 348;
        bincount = 80;
        binstate++;
        break;
      }
      if( onthebin ) gid.y -= 0x200;
      break;
    
    case 2:
      if( bincount > 0 ) bincount--;
      if( onthebin ) break;
      if( bincount == 0 )
      {
        incidentalsound( SND_BINLIFT, sfxvol );
        binstate++;
      }
      break;
    
    case 3:
      p_things4[8].y += 2;
      biny += 2;
      if( p_things4[8].y >= 398 )
      {
        p_things4[8].y = 398;
        biny = 432;
        binstate = 0;
      }
      break;
  }
}

void specialbinstand( void )
{
  onthebin = TRUE;
}

void specialbinleave( void )
{
  onthebin = FALSE;
}

void renderspecialbin( void )
{
  if( binstate == 0 )
    return;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  render_sprite_tl_stretch( &psprt4[38], 1836-fgx, biny-fgy, 9, 80, FALSE );
}
