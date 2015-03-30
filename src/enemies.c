
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
#include "enemies.h"
#include "samples.h"

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
		if (what==GL_QUADS) if (idx%4==0) { \
			indices[ids++]=idx-4; indices[ids++]=idx-3; indices[ids++]=idx-2; \
			indices[ids++]=idx-2; indices[ids++]=idx-1; indices[ids++]=idx-4; \
		}

#define glTexCoord2f(a,b)	gles_tex[idx*2+0]=a; gles_tex[idx*2+1]=b
	
#endif

extern int sfxvol;
extern char bollocks[];
extern Uint32 frame;
extern struct btex *sprs;
extern int fgx, fgy, clevel;
extern GLuint tex[];
extern Sint16 sintab[];
extern struct what_is_giddy_doing gid;
extern struct btex *psprs, *sprs, sprtg[];
extern struct mapsz *mapi;
extern Uint8 blktrans[], fgmap[], blocks[], sprites[], gsprites[], psprites[];
extern int num_nthings, num_bnthings, num_pthings, num_bpthings, num_gthings, num_bgthings;
extern struct thingy *bn_things[]; // Normal things, behind the foreground
extern struct thingy *n_things[];  // Normal things
extern struct thingy *bp_things[]; // Puzzle things, behind the foreground
extern struct thingy *p_things[];  // Puzzle things
extern struct thingy *bg_things[]; // Global things, behind the foreground
extern struct thingy *g_things[];  // Global things

Uint8 encolarea[64*128*4*4];
int bangframes[] = { 23, 24, 25, 26, 27, 28 };
int shfbframes[] = { 12, 13, 14 };

BOOL makezonecolareay( int zx, int zy, int zw, int zh )
{
  int mapo, x, y, cw, bx, by, b, bpx, bpy;
  BOOL any;

  mapo = (zy>>4)*mapi->fgw + (zx>>4);
  cw = zw*16;

  any = FALSE;
  for( y=0; y<zh; y++ )
  {
    for( x=0; x<zw; x++ )
    {
      b = blktrans[fgmap[y*mapi->fgw+x+mapo]];
      if( b != 0 ) any = TRUE;
      bpy = (b/16) * 16 * 256 * 4;
      b = (b&15)*16*4+3;
      for( by=0; by<16; by++, bpy+=256*4 )
      {
        for( bx=0,bpx=b; bx<16; bx++, bpx+=4 )
          encolarea[((y*16)+by)*cw+(x*16+bx)] = (blocks[bpy+bpx]&0xff)?0xff:0x00;
      }
    }
  }

  return any;
}

BOOL spritecolis( GLuint wtex, int sprite, int x, int y, BOOL flipped )
{
  struct btex *s;
  Uint8 *td, *ss;
  int zw, zh, sx, sy, cx, cy, fx;

  switch( wtex )
  {
    case SPRITEX:  s=sprs;  td=sprites;  break;
    case GSPRITEX: s=sprtg; td=gsprites; break;
    case PSPRITEX: s=psprs; td=psprites; break;
    default: return FALSE;
  }

  zw = (s[sprite].fw+32)/16;
  zh = (s[sprite].fh+32)/16;
  cx = x-s[sprite].hfw;
  cy = y-s[sprite].hfh;

  if( !makezonecolareay( cx, cy, zw, zh ) ) return FALSE;

  cx &= 15;
  cy &= 15;
  zw *= 16;

  ss = &td[(s[sprite].fy*256+s[sprite].fx)*4+3];

  if( flipped )
  {
    for( sy=0; sy<s[sprite].fh; sy++ )
    {
      fx = (s[sprite].fw-1)*4;
      for( sx=0; sx<s[sprite].fw; sx++ )
      {
        if( ( ss[fx] > 0 ) && ( encolarea[(sy+cy)*zw+(sx+cx)] != 0 ) )
          return TRUE;
        fx -= 4;
      }
      ss += 256*4;
    }
    return FALSE;
  }

  for( sy=0; sy<s[sprite].fh; sy++ )
  {
    for( sx=0; sx<s[sprite].fw; sx++ )
    {
      if( ( *ss > 0 ) && ( encolarea[(sy+cy)*zw+(sx+cx)] != 0 ) )
        return TRUE;
      ss += 4;
    }
    ss += (256-s[sprite].fw)*4;
  }

  return FALSE;
}

void addthingytospritecolis( struct thingy *t, struct btex *sl, Uint8 *src, int cx, int cy, int cw, int ch, BOOL down )
{
  int tl, tt, tr, tb, xp, yp, xl, cpx, cpy, cpxl;
  Uint8 bm;
  struct btex *s;

  if( !down )
  {
    if( (t->flags&THF_BLOCKJUMP) == 0 )
      return;
  } else {
    if( (t->flags&THF_BLOCKFALL) == 0 )
      return;
  }

  thingy_bounds( t, sl, &tl, &tt, &tr, &tb );

  if( ( tl > (cx+cw) ) ||
      ( tt > (cy+ch) ) ||
      ( tr < cx ) ||
      ( tb < cy ) )
    return;

  s = &sl[t->frames[t->frame]];
  src += (s->fy*256+s->fx)*4;

  yp = 0;
  if( tt < cy ) yp = cy-tt;
  xl = 0;
  if( tl < cx ) xl = cx-tl;

  cpy = 0;
  if( tt > cy ) cpy = tt-cy;
  cpxl = 0;
  if( tl > cx ) cpxl = tl-cx;

  bm = 0xff;
  if( t->flags&THF_CONVEYLEFT  ) bm = 0xfd;
  if( t->flags&THF_CONVEYRIGHT ) bm = 0xfe;

  for( ; yp<s->fh; yp++, cpy++ )
  {
    if( cpy >= ch ) break;
    for( xp=xl, cpx=cpxl; xp<s->fw; xp++, cpx++ )
    {
      if( cpx >= cw ) break;
      if( src[(yp*256+xp)*4+3] != 0 )
        encolarea[cpy*cw+cpx] = bm;
    }
  }  
}

void makezonecolareaywiththingies( int zx, int zy, int zw, int zh, int xb )
{
  int mapo, i, x, y, cw, bx, by, b, bpx, bpy;

  mapo = (zy>>4)*mapi->fgw + (zx>>4);
  cw = zw*16;

  for( y=0; y<zh; y++ )
  {
    for( x=0; x<zw; x++ )
    {
      if( y+(zy>>4) >= mapi->fgh )
      {
        b = 0;
      } else {
        i = fgmap[y*mapi->fgw+x+mapo];
        if( i == xb )
          b = 0;
        else 
          b = blktrans[i];
      }
      bpy = (b/16) * 16 * 256 * 4;
      b = (b&15)*16*4+3;
      for( by=0; by<16; by++, bpy+=256*4 )
      {
        for( bx=0,bpx=b; bx<16; bx++, bpx+=4 )
          encolarea[((y*16)+by)*cw+(x*16+bx)] = (blocks[bpy+bpx]&0xff)?0xff:0x00;
      }
    }
  }

  zh*=16;
  zx&=~15;
  zy&=~15;

  // Add in any solid objects in the area
  for( i=0; i<num_nthings; i++ )
    addthingytospritecolis( n_things[i], sprs, sprites, zx, zy, cw, zh, TRUE );
  for( i=0; i<num_bnthings; i++ )
    addthingytospritecolis( bn_things[i], sprs, sprites, zx, zy, cw, zh, TRUE );
  for( i=0; i<num_pthings; i++ )
    addthingytospritecolis( p_things[i], psprs, psprites, zx, zy, cw, zh, TRUE );
  for( i=0; i<num_bpthings; i++ )
    addthingytospritecolis( bp_things[i], psprs, psprites, zx, zy, cw, zh, TRUE );
  for( i=0; i<num_gthings; i++ )
    addthingytospritecolis( g_things[i], sprtg, gsprites, zx, zy, cw, zh, TRUE );
  for( i=0; i<num_bgthings; i++ )
    addthingytospritecolis( bg_things[i], sprtg, gsprites, zx, zy, cw, zh, TRUE );
}

int testspritedowncolis( GLuint wtex, int sprite, int x, int y, int *convey, int xb )
{
  struct btex *s;
  Uint8 *td, *ss;
  int zw, zh, sx, sy, cx, cy, ov, bov;

  switch( wtex )
  {
    case SPRITEX:  s=sprs;  td=sprites;  break;
    case GSPRITEX: s=sprtg; td=gsprites; break;
    case PSPRITEX: s=psprs; td=psprites; break;
    default: return FALSE;
  }

  zw = (s[sprite].fw+32)/16;
  zh = (s[sprite].fh+32)/16;
  cx = x-s[sprite].hfw;
  cy = y-s[sprite].hfh;

  makezonecolareaywiththingies( cx, cy, zw, zh, xb );

  cx &= 15;
  cy &= 15;
  zw *= 16;

  ss = &td[(s[sprite].fy*256+s[sprite].fx)*4+3];

  bov = 0;
  *convey = 0;
  for( sx=0; sx<s[sprite].fw; sx++, ss+=4 )
  {
    // Find the bottom of this column
    sy=s[sprite].fh-1;
    while( ( ss[sy*256*4] == 0 ) && ( sy > 0 ) ) sy--;

    // Any pixels in this column?
    if( sy == 0 ) continue;

    ov = 0;

    // Yes, so scan for collision with the background
    while( ( encolarea[(sy+cy)*zw+(sx+cx)] != 0 ) )
    {
      switch( encolarea[(sy+cy)*zw+(sx+cx)] )
      {
        case 0xfd:
          *convey = 1;
          break;
        
        case 0xfe:
          *convey = 2;
          break;
      }

      if( sy == 0 ) break;
      ov++;
      sy--;
    }

    if( ov > bov ) bov = ov;
  }

  return bov;
}

Uint8 giddyhitzone[] = { 0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
                         0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,
                         0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
                         0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
                         0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
                         0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
                         0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
                         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                         0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
                         0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
                         0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
                         0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
                         0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
                         0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,
                         0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0 };

BOOL giddydanger( GLuint wtex, int sprite, int x, int y, BOOL flipped )
{
  int dx, dy, gx, gy;
  struct btex *s;
  Uint8 *td, *src;

  if( ( gid.stargatewarp ) ||
		  ( gid.watchwarp ) )
	  return FALSE;

  switch( wtex )
  {
    case SPRITEX:  s=sprs;  td=sprites;  break;
    case GSPRITEX: s=sprtg; td=gsprites; break;
    case PSPRITEX: s=psprs; td=psprites; break;
    default: return FALSE;
  }

  gx = gid.px - 11;
  gy = gid.py - 11;

  // Get top left
  x -= s[sprite].hfw;
  y -= s[sprite].hfh;

  // Get offset into giddy danger zone
  dx = x - gx;
  dy = y - gy;

  // Any overlap?
  if( ( dx >= 21 ) ||
      ( dy >= 21 ) ||
      ( (dx+s[sprite].fw) < 0 ) ||
      ( (dy+s[sprite].fh) < 0 ) )
    return FALSE;

  src = &td[(s[sprite].fy*256+s[sprite].fx)*4+3];

  if( flipped )
  {
    for( y=((dy<0)?0:dy); y<21; y++ )
    {
      if( y >= (dy+s[sprite].fh) ) break;
      for( x=((dx<0)?0:dx); x<21; x++ )
      {
        if( x >= (dx+s[sprite].fw) ) break;

        if( ( giddyhitzone[y*21+x] != 0) && 
            ( src[((y-dy)*256+( (s[sprite].fw-1)-(x-dx) ))*4] != 0 ) )
          return TRUE;
      }
    }
    return FALSE;
  }

  for( y=((dy<0)?0:dy); y<21; y++ )
  {
    if( y >= (dy+s[sprite].fh) ) break;
    for( x=((dx<0)?0:dx); x<21; x++ )
    {
      if( x >= (dx+s[sprite].fw) ) break;

      if( ( giddyhitzone[y*21+x] != 0) && 
          ( src[((y-dy)*256+(x-dx))*4] != 0 ) )
        return TRUE;
    }
  }

  return FALSE;
}

void initenemies( struct enemy *e )
{
  int i;
  if (!e) return;
  for( i=0; e[i].initfunc!=NULL; i++ )
    e[i].initfunc( e[i].enemyarray );
}

void animateenemies( struct enemy *e )
{
  int i;
  if (!e) return;
  for( i=0; e[i].initfunc!=NULL; i++ )
    e[i].animatefunc( e[i].enemyarray );
}

void renderenemies( struct enemy *e )
{
  int i;
  if (!e) return;
  for( i=0; e[i].initfunc!=NULL; i++ )
    e[i].renderfunc( e[i].enemyarray );
}

/**************** WASP ***********************/
int waspsndchan=-1, waspsndvol=0;

void initwasps( void *v )
{
  int i;
  struct wasp *w = (struct wasp *)v;

  for( i=0; w[i].sx!=-1; i++ )
  {
    w[i].x = ((w[i].ex-w[i].sx)/2)+w[i].sx;
    w[i].ang = 0;
    w[i].returning = FALSE;
  }
}

void animatewasps( void *v )
{
  int i, j, waspdist;
  struct wasp *w = (struct wasp *)v;

  waspdist = 300000<<8;
  for( i=0; w[i].sx!=-1; i++ )
  {
    if( w[i].returning )
    {
      w[i].x-=0x280;
      if( w[i].x <= w[i].sx )
      {
        w[i].x = w[i].sx;
        w[i].returning = FALSE;
      }
    } else {
      w[i].x+=0x280;
      if( w[i].x >= w[i].ex )
      {
        w[i].x = w[i].ex;
        w[i].returning = TRUE;
      }
    }

    j = abs( w[i].x - gid.x );
    if( j < waspdist ) waspdist = j;

    w[i].ang = (w[i].ang+3)%360;
    w[i].cy = w[i].y  + ((sintab[w[i].ang]*50)>>8);

    if( giddydanger( SPRITEX, 21, w[i].x>>8, w[i].cy>>8, !w[i].returning ) )
      giddyhit();
  }

  i = waspsndvol;
  waspsndvol = 1000-(waspdist>>6);
  if( waspsndvol < 0 ) waspsndvol = 0;
  if( gid.y > (480<<8) ) waspsndvol = 0;

  waspsndvol = (waspsndvol*sfxvol)/1000;

  if( waspsndvol > 0 )
  {
    if( waspsndchan == -1 )
    {
      ambientloop( SND_WASP, waspsndvol, &waspsndchan );
    } else {
      if( i != waspsndvol ) setvol( waspsndchan, waspsndvol );
    }
  } else {
    if( waspsndchan != -1 )
      stopchannel( waspsndchan );
  }
}

void renderwasps( void *v )
{
  struct wasp *w = (struct wasp *)v;
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; w[i].sx!=-1; i++ )
  {
    if( ( ((w[i].x>>8)-fgx) < -32 ) ||
        ( ((w[i].x>>8)-fgx) > 352 ) ||
        ( ((w[i].cy>>8)-fgy) < -32 ) ||
        ( ((w[i].cy>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[(frame&2)?21:22], (w[i].x>>8)-fgx, (w[i].cy>>8)-fgy, !w[i].returning, 0.0f );
  }
}


/**************** SPIDER *********************/
void initspiders( void *v )
{
  int i;
  struct spider *s = (struct spider *)v;

  for( i=0; s[i].x!=-1; i++ )
  {
    s[i].y = s[i].sy;
    s[i].returning = FALSE;
    s[i].timeout = (rand()%63);
  }
}

void animatespiders( void *v )
{
  int i;
  struct spider *s = (struct spider *)v;

  for( i=0; s[i].x!=-1; i++ )
  {
    if( s[i].returning )
    {
      s[i].y-=0x80;
      if( s[i].y <= s[i].sy )
      {
        s[i].y = s[i].sy;
        s[i].returning = FALSE;
        s[i].timeout = (rand()%63);
      }
    } else {
      if( s[i].timeout > 0 )
        s[i].timeout--;
      else
        s[i].y+=0x80;
      if( s[i].y >= s[i].ey )
      {
        s[i].y = s[i].ey;
        s[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 23, s[i].x>>8, s[i].y>>8, FALSE ) )
      giddyhit();
  }
}

void renderspiders( void *v )
{
  struct spider *s = (struct spider *)v;
  int i, vl, vt, vr, vb;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; s[i].x!=-1; i++ )
  {
    vl = ((s[i].x>>8)-1)-fgx;
    vt = (s[i].sy>>8)-fgy;
    vr = ((s[i].x>>8)+1)-fgx;
    vb = (s[i].y>>8)-fgy;

    glLoadIdentity();
    glBegin( GL_QUADS );
      glColor4ub(255, 255, 255, 255);
      glTexCoord2f( 187.0f/256.0f, 106.0f/256.0f ); glVertex3f( vl, vt, 0.0f );
      glTexCoord2f( 189.0f/256.0f, 106.0f/256.0f ); glVertex3f( vr, vt, 0.0f );
      glTexCoord2f( 189.0f/256.0f, 136.0f/256.0f ); glVertex3f( vr, vb, 0.0f );
      glTexCoord2f( 187.0f/256.0f, 136.0f/256.0f ); glVertex3f( vl, vb, 0.0f );
    glEnd();
    
    render_sprite( &sprs[(frame&8)?23:24], (s[i].x>>8)-fgx, (s[i].y>>8)-fgy, FALSE, 0.0f );
  }
}


/**************** SHROOM *********************/
void initshrooms( void *v )
{
  int i;
  struct shroom *s = (struct shroom *)v;

  for( i=0; s[i].sx!=-1; i++ )
  {
    s[i].x = s[i].sx;
    s[i].returning = FALSE;
  }
}

void animateshrooms( void *v )
{
  int i;
  struct shroom *s = (struct shroom *)v;

  for( i=0; s[i].sx!=-1; i++ )
  {
    if( s[i].returning )
    {
      s[i].x-=0xc0;
      if( s[i].x <= s[i].sx )
      {
        s[i].x = s[i].sx;
        s[i].returning = FALSE;
      }
    } else {
      s[i].x+=0xc0;
      if( s[i].x >= s[i].ex )
      {
        s[i].x = s[i].ex;
        s[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 16, s[i].x>>8, s[i].y>>8, !s[i].returning ) )
      giddyhit();
  }
}

void rendershrooms( void *v )
{
  struct shroom *s = (struct shroom *)v;
  int i, spr;

  switch( clevel )
  {
    case 3:
      spr = ((frame>>2)%3)+24;
      break;
     
    default:
      spr = ((frame>>2)%3)+16;
      break;
  }

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; s[i].sx!=-1; i++ )
  {
    if( ( ((s[i].x>>8)-fgx) < -32 ) ||
        ( ((s[i].x>>8)-fgx) > 352 ) ||
        ( ((s[i].y>>8)-fgy) < -32 ) ||
        ( ((s[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[spr], (s[i].x>>8)-fgx, (s[i].y>>8)-fgy, !s[i].returning, 0.0f );
  }
}

/**************** SHOOTY FLOWER *********************/
#define MAX_SHOOTYF_MISSILES 8
struct shootyf_missile
{
  int x, y;
  int dx, dy, tx;
};

struct shootyf_missile shootyfmsls[MAX_SHOOTYF_MISSILES];
int shootyfnextmsl = 0;

void initshootyf( void *v )
{
  int i;
  struct shootyf *s = (struct shootyf *)v;

  for( i=0; i<MAX_SHOOTYF_MISSILES; i++ )
    shootyfmsls[i].x = -1;

  for( i=0; s[i].x!=-1; i++ )
  {
    s[i].gy  = 0;
    s[i].ang = 0;
    s[i].grown = FALSE;
    s[i].shoottime = 0;
    s[i].shot = FALSE;
  }
}

void animateshootyf( void *v )
{
  int i;
  struct shootyf *s = (struct shootyf *)v;

  for( i=0; i<MAX_SHOOTYF_MISSILES; i++ )
  {
    if( shootyfmsls[i].x != -1 )
    {
      if( spritecolis( SPRITEX, 38, shootyfmsls[i].x>>8, shootyfmsls[i].y>>8, FALSE ) )
      {
        if( startincidental( shootyfmsls[i].x>>8, shootyfmsls[i].y>>8, 0, 0, GSPRITEX, bangframes, 5, 2 ) )
          incidentalsound( SND_EXPLOS, sfxvol );
        shootyfmsls[i].x = -1;
        continue;
      }

      shootyfmsls[i].x += shootyfmsls[i].dx;
      if( shootyfmsls[i].dx > 0 )
      {
        if( shootyfmsls[i].x > (shootyfmsls[i].tx-(4<<8)) )
          shootyfmsls[i].dx=(shootyfmsls[i].dx*7)/8;
      } else {
        if( shootyfmsls[i].x < (shootyfmsls[i].tx+(4<<8)) )
          shootyfmsls[i].dx=(shootyfmsls[i].dx*7)/8;
      }

      shootyfmsls[i].y += shootyfmsls[i].dy;
      shootyfmsls[i].dy += 0x20;
      if( shootyfmsls[i].dy > (6<<8) ) shootyfmsls[i].dy = (6<<8);

      if( giddydanger( SPRITEX, 38, shootyfmsls[i].x>>8, (shootyfmsls[i].y>>8)-4, FALSE ) )
        giddyhit();
    }
  }

  for( i=0; s[i].x!=-1; i++ )
  {
    if( s[i].shoottime > 0 )
      s[i].shoottime--;
    else
      s[i].shot = FALSE;

    if( s[i].grown )
    {
      // Make it grow!
      if( s[i].gy < (8<<8) ) s[i].gy+=(1<<8);
      if( s[i].gy > (8<<8) ) s[i].gy=(8<<8);

      // Out of "the zone" ?
      if( ( gid.x < (s[i].x-(220<<8)) ) ||
          ( gid.x > (s[i].x+(220<<8)) ) ||
          ( gid.y < (s[i].y-(120<<8)) ) ||
          ( gid.y > (s[i].y+(120<<8)) ) )
      {
        s[i].grown = FALSE;
      } else {
        // No, but is it on the screen?
        if( ( ((s[i].x>>8)-fgx) > -8 ) &&
            ( ((s[i].x>>8)-fgx) < 329 ) &&
            ( ((s[i].y>>8)-fgy) > 0 ) &&
            ( ((s[i].y>>8)-fgy) < 264 ) )
        {
          // Yes, so shoot at giddy
          if( s[i].shoottime == 0 )
          {
            s[i].shot = 1;
            s[i].shoottime = 60;

            shootyfmsls[shootyfnextmsl].x  = s[i].x;
            shootyfmsls[shootyfnextmsl].y  = s[i].y-(56<<8);
            shootyfmsls[shootyfnextmsl].dx = (gid.x-s[i].x)/100;
            shootyfmsls[shootyfnextmsl].dy = -5<<8;
            shootyfmsls[shootyfnextmsl].tx = gid.x;
            shootyfnextmsl = (shootyfnextmsl+1)%MAX_SHOOTYF_MISSILES;
            enemysound( SND_PLANTSHOOT, sfxvol );
          }
        }
      }
    } else {
      s[i].gy = 0;
      if( ( gid.x > (s[i].x-(65<<8)) ) &&
          ( gid.x < (s[i].x+(65<<8)) ) &&
          ( gid.y > (s[i].y-(56<<8)) ) &&
          ( gid.y < (s[i].y+(16<<8)) ) )
      {
        s[i].shoottime = 30;
        s[i].grown = TRUE;
      }
    }
    s[i].ang = (s[i].ang+5)%360;
  }
}

void rendershootyf( void *v )
{
  int i, j, y, a;
  struct shootyf *s = (struct shootyf *)v;

  for( i=0; s[i].x!=-1; i++ )
  {
    if( ( ( (s[i].x>>8)-fgx ) < -32 ) ||
        ( ( (s[i].x>>8)-fgx ) > 352 ) ||
        ( ( (s[i].y>>8)-fgy ) < -32 ) ||
        ( ( (s[i].y>>8)-fgy ) > 300 ) )
      continue;

    if( s[i].gy > (2<<8) )
    {
      for( j=0, y=(s[i].y-(3<<8))-(fgy<<8), a=s[i].ang; j<5; j++, y-=s[i].gy, a=(a+25)%360 )
        render_sprite( &sprs[36+(j&1)], (((sintab[a]>>5)+s[i].x)>>8)-fgx, y>>8, FALSE, 0.0f );
      if( ( s[i].shot ) && ( s[i].shoottime > 50 ) )
        render_sprite( &sprs[35], (((sintab[a]>>5)+s[i].x)>>8)-fgx, (y-s[i].gy)>>8, FALSE, 0.0f );
      else
        render_sprite( &sprs[34], (((sintab[a]>>5)+s[i].x)>>8)-fgx, (y-s[i].gy)>>8, FALSE, 0.0f );
    } else {
      render_sprite( &sprs[34], (s[i].x>>8)-fgx, ((s[i].y>>8)-4)-fgy, FALSE, 0.0f );
    }
  }
}

void rendershootyfmissiles( void )
{
  int i;
  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; i<MAX_SHOOTYF_MISSILES; i++ )
  {
    if( shootyfmsls[i].x != -1 )
      render_sprite( &sprs[((frame>>3)&1)+38], (shootyfmsls[i].x>>8)-fgx, ((shootyfmsls[i].y>>8)-4)-fgy, FALSE, 0.0f );
  }
}

/**************** MINE ***********************/
int mineang;

struct minimine
{
  int x, y;
  int dx, dy;
};

#define MAX_MINIMINES 8
struct minimine mmines[MAX_MINIMINES];
int nextmmine=0;

void initmines( void *v )
{
  int i;
  struct mine *m = (struct mine *)v;

  mineang = 0;
  nextmmine = 0;

  for( i=0; i<MAX_MINIMINES; i++ )
    mmines[i].x = -1;

  for( i=0; m[i].x!=-1; i++ )
  {
    m[i].attached = TRUE;
    m[i].active = TRUE;
    m[i].dm = -9;
    m[i].hy = m[i].y - 70;
  }
}

void setminimine( int x, int y, int dx, int dy )
{
  mmines[nextmmine].x = x;
  mmines[nextmmine].y = y;
  mmines[nextmmine].dx = dx;
  mmines[nextmmine].dy = dy;
  nextmmine = (nextmmine+1)%MAX_MINIMINES;
}

void animatemines( void *v )
{
  int i;
  struct mine *m = (struct mine *)v;

  mineang = (mineang+2)%360;

  for( i=0; i<MAX_MINIMINES; i++ )
  {
    if( mmines[i].x != -1 )
    {
      mmines[i].x += mmines[i].dx;
      mmines[i].y += mmines[i].dy;
      if( spritecolis( SPRITEX, 55, mmines[i].x>>8, mmines[i].y>>8, FALSE ) )
      {
        if( startincidental( mmines[i].x>>8, mmines[i].y>>8, 0, 0, GSPRITEX, bangframes, 5, 2 ) )
          incidentalsound( SND_EXPLOS, sfxvol );
        mmines[i].x = -1;
      } else {
        if( giddydanger( SPRITEX, 55, mmines[i].x>>8, mmines[i].y>>8, FALSE ) )
        {
          if( startincidental( mmines[i].x>>8, mmines[i].y>>8, 0, 0, GSPRITEX, bangframes, 5, 2 ) )
            incidentalsound( SND_EXPLOS, sfxvol );
          mmines[i].x = -1;
          giddyhit();
        }
      }
    }
  }

  for( i=0; m[i].x!=-1; i++ )
  {
    if( !m[i].active ) continue;
    if( m[i].attached )
    {
      if( giddydanger( SPRITEX, 25, m[i].x+(sintab[(mineang+6*30)%360]/6553), m[i].hy, FALSE ) )
        giddyhit();

      // Is the mine bit on the screen?
      if( ( ( m[i].x-fgx ) < -20 ) ||
          ( ( m[i].x-fgx ) > 340 ) ||
          ( ( m[i].hy-fgy ) < -20 ) ||
          ( ( m[i].hy-fgy ) > 244 ) )
        continue;
      
      // Is giddy over it?
      if( ( gid.px > m[i].x-15 ) &&
          ( gid.px < m[i].x+15 ) &&
          ( gid.py < m[i].hy+10 ) )
        m[i].attached = FALSE;
    } else {
      BOOL cac;

      if( spritecolis( SPRITEX, 55, m[i].x, m[i].hy, FALSE ) )  // Use mini mine for collision detection otherwise the one by the balloon lift and fish blows up too readily
      {
        float ta;
        int tc;
        m[i].active = FALSE;
        if( startincidental( m[i].x, m[i].hy, 0, 0, GSPRITEX, bangframes, 5, 2 ) )
          incidentalsound( SND_EXPLOS, sfxvol );
        for( tc=0, ta=-(3.14159265/5.0f)*1.5f; tc<4; tc++, ta+=(3.14159265/5.0f) )
          setminimine( m[i].x<<8, (m[i].hy+4)<<8, (int)(sin(ta)*512.0f), (int)(cos(ta)*512.0f) );
      } else {
        cac = TRUE;
        if( ( (m[i].hy-fgy) > -15 ) &&
            ( (m[i].hy-fgy) < 260 ) )
        {
          cac = FALSE;
          m[i].hy--;
        }

        if( m[i].dm < 3 )
        {
          cac = FALSE;
          m[i].dm++;
        }

        if( cac )
          m[i].active = FALSE;

        if( giddydanger( SPRITEX, 25, m[i].x, m[i].hy, FALSE ) )
          giddyhit();
      }
    }
  }
}

void rendermines( void *v )
{
  int i, j, ty, a;
  struct mine *m = (struct mine *)v;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );

  for( i=0; i<MAX_MINIMINES; i++ )
  {
    if( mmines[i].x != -1 )
    {
      if( ( ( (mmines[i].x>>8)-fgx ) < -20 ) ||
          ( ( (mmines[i].x>>8)-fgx ) > 340 ) ||
          ( ( (mmines[i].y>>8)-fgy ) < -20 ) ||
          ( ( (mmines[i].y>>8)-fgy ) > 264 ) )
        continue;

      render_sprite( &sprs[55], (mmines[i].x>>8)-fgx, (mmines[i].y>>8)-fgy, FALSE, 0.0f );
    }
  }

  for( i=0; m[i].x!=-1; i++ )
  {
    a = (mineang + 6*30)%360;
    if( m[i].active )
    {
      if( ( ( m[i].x-fgx ) >= -32 ) ||
          ( ( m[i].x-fgx ) < 352 ) ||
          ( ( m[i].hy-fgy ) >= -32 ) ||
          ( ( m[i].hy-fgy ) < 256 ) )
        render_sprite( &sprs[((frame>>4)&3)+25], m[i].x+(sintab[a]/6553)-fgx, m[i].hy-fgy, FALSE, 0.0f );
    }

    if( ( ( m[i].x-fgx ) < -32 ) ||
        ( ( m[i].x-fgx ) > 352 ) ||
        ( ( m[i].y-fgy ) < -32 ) ||
        ( ( m[i].y-fgy ) > 300 ) )
      continue;

    render_sprite( &sprs[29], m[i].x-fgx, m[i].y-fgy, FALSE, 0.0f );
    if( !m[i].active ) continue;
    a=0;
    if( m[i].dm < 3 )
    {
      for( j=0, a=mineang, ty=m[i].y-10; j<6; j++, a=(a+30)%360, ty+=m[i].dm )
        render_sprite( &sprs[48], m[i].x+(sintab[a]/6553)-fgx, ty-fgy, FALSE, 0.0f );
    }
  }
}

/**************** FISH *********************/
void initfish( void *v )
{
  int i;
  struct fish *f = (struct fish *)v;

  for( i=0; f[i].sx!=-1; i++ )
  {
    f[i].x = f[i].sx;
    f[i].returning = FALSE;
  }
}

void animatefish( void *v )
{
  int i;
  struct fish *f = (struct fish *)v;

  for( i=0; f[i].sx!=-1; i++ )
  {
    if( f[i].returning )
    {
      f[i].x-=0x100;
      if( f[i].x <= f[i].sx )
      {
        f[i].x = f[i].sx;
        f[i].returning = FALSE;
      }
    } else {
      f[i].x+=0x100;
      if( f[i].x >= f[i].ex )
      {
        f[i].x = f[i].ex;
        f[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 19, f[i].x>>8, f[i].y>>8, !f[i].returning ) )
      giddyhit();
  }
}

void renderfish( void *v )
{
  struct fish *f = (struct fish *)v;
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; f[i].sx!=-1; i++ )
  {
    if( ( ((f[i].x>>8)-fgx) < -32 ) ||
        ( ((f[i].x>>8)-fgx) > 352 ) ||
        ( ((f[i].y>>8)-fgy) < -32 ) ||
        ( ((f[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[((frame>>3)&1)+19], (f[i].x>>8)-fgx, (f[i].y>>8)-fgy, !f[i].returning, 0.0f );
  }
}

/**************** STEAM *********************/
void initsteam( void *v )
{
  int i, j;
  struct steam *s = (struct steam *)v;

  for( i=0; s[i].x!=-1; i++ )
  {
    for( j=0; j<4; j++ )
    {
      s[i].sx[j] = -1;
      s[i].scale[j] = 0.03f;
    }
    s[i].count = 120;
    s[i].nexts = 0;
  }
}

void animatesteam( void *v )
{
  int i,j,ssnd;
  struct steam *s = (struct steam *)v;

  ssnd = 0;
  for( i=0; s[i].x!=-1; i++ )
  {
    for( j=0; j<4; j++ )
    {
      if( s[i].sx[j] != -1 )
      {
        s[i].sx[j] += s[i].dx;
        s[i].sy[j] += s[i].dy;
        s[i].scale[j] += 0.06f;
        if( giddydanger( GSPRITEX, 23, s[i].sx[j]>>8, s[i].sy[j]>>8, FALSE ) )
          giddyhit();
      }
    }

    if( ( s[i].count & 3 ) == 0 )
    {
      s[i].sx[s[i].nexts] = (s[i].count>=60?s[i].x<<8:-1);
      s[i].sy[s[i].nexts] = s[i].y<<8;
      s[i].scale[s[i].nexts] = 0.03f;
      s[i].nexts = (s[i].nexts+1)&3;
    }

    if( s[i].count > 0 )
    {
      s[i].count--;
    } else {
      if( ( (s[i].x-fgx) > -16 ) &&
          ( (s[i].x-fgx) < 336 ) &&
          ( (s[i].y-fgy) > -16 ) &&
          ( (s[i].y-fgy) < 256 ) )
        ssnd = 1;

      s[i].count=120;
    }
  }

  if( ssnd )
    incidentalsound( SND_STEAM, sfxvol/2 );
}

void rendersteam( void *v )
{
  struct steam *s = (struct steam *)v;
  int i,j;

  glBindTexture( GL_TEXTURE_2D, tex[GSPRITEX] );
  for( i=0; s[i].x!=-1; i++ )
  {
    for( j=0; j<4; j++ )
    {
      if( s[i].sx[j] != -1 )
      {
        if( ( ((s[i].sx[j]>>8)-fgx) < -16 ) ||
            ( ((s[i].sx[j]>>8)-fgx) > 336 ) ||
            ( ((s[i].sy[j]>>8)-fgy) < -16 ) ||
            ( ((s[i].sy[j]>>8)-fgy) > 256 ) )
          continue;
        render_sprite_scaleda( &sprtg[23], (s[i].sx[j]>>8)-fgx, (s[i].sy[j]>>8)-fgy, FALSE, 0.0f, s[i].scale[j], 200 );
      }
    }
  }
}

/************ TOXIC WASTE BARREL ***********/
void initbarrels( void *v )
{
  int i;
  struct barrel *b = (struct barrel *)v;

  for( i=0; b[i].sx!=-1; i++ )
  {
    b[i].x = b[i].sx<<8;
    b[i].y = b[i].sy<<8;
    b[i].grav = 2;
    b[i].xm = 0;
    b[i].timeout = b[i].itimeout;
  }
}

void animatebarrels( void *v )
{
  int i, cd, cv;
  BOOL domom;
  struct barrel *b = (struct barrel *)v;

  for( i=0; b[i].sx!=-1; i++ )
  {
    domom = TRUE;

    if( b[i].timeout > 0 )
    {
      b[i].timeout--;
      continue;
    }

    // Gravity
    b[i].y    += b[i].grav;
    b[i].grav += 0x80;
    if( b[i].grav > 0x600 ) b[i].grav = 0x600;

    // Collision with ground
    cd = testspritedowncolis( PSPRITEX, 17, b[i].x>>8, b[i].y>>8, &cv, 76 );

    if( cd > 0 )
    {
      b[i].y = (b[i].y&0xffffff00)-(cd<<8);
      b[i].grav = 0x100;

      // Conveying?
      switch( cv )
      {
        case 1:
          b[i].x -= 0x100;
          b[i].xm = -0x180;
          domom = FALSE;
          break;
        
        case 2:
          b[i].x += 0x100;
          b[i].xm = 0x180;
          domom = FALSE;
          break;
      }
    }

    // Momentum
    if( ( domom ) && ( b[i].xm != 0 ) )
    {
      b[i].x += b[i].xm;
      if( b[i].xm < 0 ) { b[i].xm += 0x4; if( b[i].xm > 0 ) b[i].xm = 0; }
      if( b[i].xm > 0 ) { b[i].xm -= 0x4; if( b[i].xm < 0 ) b[i].xm = 0; }
    }

    // Fallen off the screen?
    if( b[i].y >= (b[i].resety<<8) )
    {
      b[i].x = b[i].sx<<8;
      b[i].y = b[i].sy<<8;
      b[i].xm = 0;
      b[i].grav = 0x80;
    }

    if( giddydanger( PSPRITEX, 17, b[i].x>>8, b[i].y>>8, FALSE ) )
      giddyhit();
  }
}

void renderbarrels( void *v )
{
  struct barrel *b = (struct barrel *)v;
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; b[i].sx!=-1; i++ )
  {
    if( ( ((b[i].x>>8)-fgx) < -32 ) ||
        ( ((b[i].x>>8)-fgx) > 352 ) ||
        ( ((b[i].y>>8)-fgy) < -32 ) ||
        ( ((b[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &psprs[17], (b[i].x>>8)-fgx, (b[i].y>>8)-fgy, FALSE, 0.0f );
  }
}

/**************** GRABBER ******************/

void initgrabbers( void *v )
{
  int i;
  struct grabber *g = (struct grabber *)v;

  for( i=0; g[i].sx!=-1; i++ )
  {
    g[i].x  = g[i].sx<<8;
    g[i].y  = g[i].sy<<8;
    g[i].gy = g[i].gty<<8;
    g[i].gf = 22;
    g[i].grabstate = GRABS_NORMAL;
    g[i].timeout   = 0;
    g[i].returning = FALSE;
  }
}

void animategrabbers( void *v )
{
  int i, j;
  struct grabber *g = (struct grabber *)v;

  for( i=0; g[i].sx!=-1; i++ )
  {
    switch( g[i].grabstate )
    {
      case GRABS_NORMAL:
        if( ( g[i].x <(gid.x+(16<<8)) ) &&
            ( g[i].x > (gid.x-(16<<8)) ) &&
            ( gid.py > (g[i].gty-10) ) &&
            ( gid.py < (g[i].gby+40) ) )
        {
          g[i].grabstate = GRABS_LOWERING;
          enemysound( SND_PULLEYDROP, sfxvol );
          break;
        }

      case GRABS_NORMAL_DONTLOWER:
        if( g[i].returning )
        {
          g[i].x-=0x100;
          if( g[i].x <= (g[i].sx<<8) )
          {
            g[i].x = (g[i].sx<<8);
            g[i].returning = FALSE;
            g[i].grabstate = GRABS_NORMAL;
          }
        } else {
          g[i].x+=0x100;
          if( g[i].x >= (g[i].ex<<8) )
          {
            g[i].x = (g[i].ex<<8);
            g[i].returning = TRUE;
            g[i].grabstate = GRABS_NORMAL;
          }
        }
        break;
      
      case GRABS_LOWERING:
        if( g[i].gy < (g[i].gby<<8) )
        {
          g[i].gy += 0x200;
          if( g[i].gy > (g[i].gby<<8) ) g[i].gy = g[i].gby<<8;
          break;
        }

        g[i].grabstate = GRABS_GRABBING;
        g[i].timeout   = 30;
        break;
      
      case GRABS_GRABBING:
        if( g[i].timeout > 0 )
        {
          g[i].timeout--;
          if( (g[i].timeout%8) == 0 )
          {
            enemysound( SND_WCLICK2, sfxvol );
            if( g[i].gf == 22 )
              g[i].gf = 21;
            else
              g[i].gf = 22;
          }
          break;
        }

        g[i].grabstate = GRABS_RAISING;
        break;
      
      case GRABS_RAISING:
        if( g[i].gy > (g[i].gty<<8) )
        {
          g[i].gy -= 0x200;
          if( g[i].gy < (g[i].gty<<8) ) g[i].gy = g[i].gty<<8;
          break;
        }

        g[i].grabstate = GRABS_NORMAL_DONTLOWER;
        break;

    }

    if( g[i].grabstate == GRABS_GRABBING )
      j = ((g[i].timeout%10)<5)?22:21;
    else
      j = 22;

    if( giddydanger( PSPRITEX, j, g[i].x>>8, (g[i].gy>>8)+24, FALSE ) )
      giddyhit();
  }
}

void rendergrabbers( void *v )
{
  struct grabber *g = (struct grabber *)v;
  struct btex *tx;
  int i;
  float ch, sw;

  tx = &psprs[19];
  sw = tx->hwx;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; g[i].sx!=-1; i++ )
  {
    ch = (float)((g[i].gy-g[i].y)>>8)/2.0f;
    if( ch > 9.0f )
    {
      glLoadIdentity();
      glTranslatef( (g[i].x>>8)-fgx, ((float)(g[i].y>>8)) + ch - fgy, 0.0f );
      glBegin( GL_QUADS );
        glColor4ub(255, 255, 255, 255);
        glTexCoord2f( tx->x,       tx->y       ); glVertex3f( -sw, -ch, 0.0f );
        glTexCoord2f( tx->x+tx->w, tx->y       ); glVertex3f(  sw, -ch, 0.0f );
        glTexCoord2f( tx->x+tx->w, tx->y+tx->h ); glVertex3f(  sw,  ch, 0.0f );
        glTexCoord2f( tx->x,       tx->y+tx->h ); glVertex3f( -sw,  ch, 0.0f );
      glEnd();
    }
    render_sprite( &psprs[18], (g[i].x>>8)-fgx, (g[i].y>>8)-fgy,  FALSE, 0.0f );
    render_sprite( &psprs[20], (g[i].x>>8)-fgx, (g[i].gy>>8)-fgy, FALSE, 0.0f );
    if( g[i].grabstate == GRABS_GRABBING )
      render_sprite( &psprs[g[i].gf], (g[i].x>>8)-fgx, (g[i].gy>>8)+24-fgy, FALSE, 0.0f );
    else
      render_sprite( &psprs[22], (g[i].x>>8)-fgx, (g[i].gy>>8)+24-fgy, FALSE, 0.0f );
  }
}

/**************** MINI TANK ****************/
#define MAX_MINITANK_MISSILES 8
struct minitankmissile
{
  int x, y;
  int dx, dy, tx;
};

struct minitankmissile mtmsls[MAX_MINITANK_MISSILES];
int minitanknextmsl = 0;

void initminitanks( void *v )
{
  int i;
  struct minitank *m = (struct minitank *)v;

  for( i=0; i<MAX_MINITANK_MISSILES; i++ )
    mtmsls[i].x = -1;

  for( i=0; m[i].sx!=-1; i++ )
  {
    m[i].x = m[i].sx;
    if( m[i].x == m[i].rx )
      m[i].returning = TRUE;
    else
      m[i].returning = FALSE;
  }
}

void animateminitanks( void *v )
{
  int i;
  struct minitank *m = (struct minitank *)v;

  for( i=0; i<MAX_MINITANK_MISSILES; i++ )
  {
    if( mtmsls[i].x != -1 )
    {
      if( spritecolis( SPRITEX, 11, mtmsls[i].x>>8, mtmsls[i].y>>8, FALSE ) )
      {
        if( startincidental( mtmsls[i].x>>8, mtmsls[i].y>>8, 0, 0, GSPRITEX, bangframes, 5, 2 ) )
          incidentalsound( SND_EXPLOS, sfxvol );
        mtmsls[i].x = -1;
        continue;
      }

      mtmsls[i].x += mtmsls[i].dx;
      mtmsls[i].y += mtmsls[i].dy;
      mtmsls[i].dy += 0x40;
      if( mtmsls[i].dy > 0x600 ) mtmsls[i].dy = 0x600;

      if( giddydanger( SPRITEX, 11, mtmsls[i].x>>8, mtmsls[i].y>>8, FALSE ) )
        giddyhit();
    }
  }

  for( i=0; m[i].sx!=-1; i++ )
  {
    if( m[i].shootcount <= 0 )
      m[i].shootcount = 120;
    else
      m[i].shootcount--;
    
    if( m[i].flashcount > 0 )
      m[i].flashcount--;

    switch( m[i].shootcount )
    {
      case 120:
      case 110:
      case 100:
        if( ( m[i].x < ((fgx-64)<<8) ) ||
            ( m[i].x > ((fgx+384)<<8) ) ||
            ( m[i].y < ((fgy-32)<<8) ) ||
            ( m[i].y > ((fgy+240)<<8) ) )
          break;
        if( m[i].returning )
        {
          mtmsls[minitanknextmsl].x  = m[i].x - 0x500;
          mtmsls[minitanknextmsl].dx = -0x400;
        } else {
          mtmsls[minitanknextmsl].x  = m[i].x + 0x500;
          mtmsls[minitanknextmsl].dx = 0x400;
        }
        mtmsls[minitanknextmsl].y  = m[i].y-0x200;
        mtmsls[minitanknextmsl].dy = -0x600;
        minitanknextmsl = (minitanknextmsl+1)%MAX_MINITANK_MISSILES;
        m[i].flashcount = 6;
        enemysound( SND_TANKSHOOT, sfxvol );
        break;
    }

    if( m[i].returning )
    {
      m[i].x-=0xc0;
      if( m[i].x <= m[i].lx )
      {
        m[i].x = m[i].lx;
        m[i].returning = FALSE;
      }
    } else {
      m[i].x+=0xc0;
      if( m[i].x >= m[i].rx )
      {
        m[i].x = m[i].rx;
        m[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 7, m[i].x>>8, m[i].y>>8, !m[i].returning ) )
      giddyhit();
  }
}

void renderminitanks( void *v )
{
  int i;
  struct minitank *m = (struct minitank *)v;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; i<MAX_MINITANK_MISSILES; i++ )
  {
    if( mtmsls[i].x != -1 )
      render_sprite( &sprs[((frame>>3)&1)+10], (mtmsls[i].x>>8)-fgx, (mtmsls[i].y>>8)-fgy, FALSE, 0.0f );
  }

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; m[i].sx!=-1; i++ )
  {
    if( ( ((m[i].x>>8)-fgx) < -32 ) ||
        ( ((m[i].x>>8)-fgx) > 352 ) ||
        ( ((m[i].y>>8)-fgy) < -32 ) ||
        ( ((m[i].y>>8)-fgy) > 256 ) )
      continue;
    if( m[i].flashcount > 0 )
    {
      if( m[i].returning )
        render_sprite( &sprs[9], ((m[i].x>>8)-11)-fgx, ((m[i].y>>8)-12)-fgy, FALSE, 0.0f );
      else
        render_sprite( &sprs[9], ((m[i].x>>8)+11)-fgx, ((m[i].y>>8)-12)-fgy, TRUE, 0.0f );
    }
    render_sprite( &sprs[((frame>>1)&1)+7], (m[i].x>>8)-fgx, (m[i].y>>8)-fgy, !m[i].returning, 0.0f );
  }
}

/**************** CRUSHER ******************/
void initcrushers( void *v )
{
  int i;
  struct crusher *c = (struct crusher *)v;

  for( i=0; c[i].x!=-1; i++ )
  {
    c[i].y = c[i].sy+(18<<8);
    c[i].timeout = c[i].itimeout + (rand()&7)*10;
    c[i].returning = TRUE;
  }
}

void animatecrushers( void *v )
{
  int i;
  struct crusher *c = (struct crusher *)v;

  for( i=0; c[i].x!=-1; i++ )
  {
    if( c[i].timeout > 0 ) 
      c[i].timeout--;

    if( c[i].returning )
    {
      c[i].y-=0x400;
      if( c[i].y <= (c[i].sy+(18<<8)) )
      {
        c[i].y = (c[i].sy+(18<<8));
        if( c[i].timeout == 0 )
        {
          c[i].returning = FALSE;
          c[i].timeout = c[i].itimeout;
        }
      }
    } else {
      c[i].y+=0x400;
      if( c[i].y >= c[i].ey )
      {
        c[i].y = c[i].ey;
        c[i].returning = TRUE;

        if( ( ((c[i].x>>8)-fgx)  > -32 ) &&
            ( ((c[i].x>>8)-fgx)  < 352 ) &&
            ( ((c[i].sy>>8)-fgy) > -32 ) &&
            ( ((c[i].y>>8)-fgy)  < 256 ) )
          enemysound( SND_CRUSHERHIT, sfxvol );
      }
    }

    if( giddydanger( PSPRITEX, 25, c[i].x>>8, c[i].y>>8, FALSE ) )
      giddyhit();
  }
}

void rendercrushers( void *v )
{
  struct crusher *c = (struct crusher *)v;
  struct btex *tx;
  int i;
  float ch, sw;

  tx = &psprs[24];
  sw = tx->hwx;

  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; c[i].x!=-1; i++ )
  {
    if( ( ((c[i].x>>8)-fgx)  < -32 ) ||
        ( ((c[i].x>>8)-fgx)  > 352 ) ||
        ( ((c[i].sy>>8)-fgy) < -32 ) ||
        ( ((c[i].y>>8)-fgy)  > 256 ) )
      continue;
    ch = (float)((c[i].y-c[i].sy)>>8)/2.0f;
    if( ch > 9.0f )
    {
      glLoadIdentity();
      glTranslatef( (c[i].x>>8)-fgx, ((float)(c[i].sy>>8)) + ch - fgy, 0.0f );
      glBegin( GL_QUADS );
        glColor4ub(255, 255, 255, 255);
        glTexCoord2f( tx->x,       tx->y       ); glVertex3f( -sw, -ch, 0.0f );
        glTexCoord2f( tx->x+tx->w, tx->y       ); glVertex3f(  sw, -ch, 0.0f );
        glTexCoord2f( tx->x+tx->w, tx->y+tx->h ); glVertex3f(  sw,  ch, 0.0f );
        glTexCoord2f( tx->x,       tx->y+tx->h ); glVertex3f( -sw,  ch, 0.0f );
      glEnd();
    }
    render_sprite( &psprs[23], (c[i].x>>8)-fgx, (c[i].sy>>8)-fgy, FALSE, 0.0f );
    render_sprite( &psprs[25], (c[i].x>>8)-fgx, (c[i].y>>8)-fgy,  FALSE, 0.0f );
  }
}

/************* LITTLE GREEN DUDE ***********/

// Uses fish structure, and setup code
void animatedudes( void *v )
{
  int i;
  struct fish *f = (struct fish *)v;

  for( i=0; f[i].sx!=-1; i++ )
  {
    if( f[i].returning )
    {
      f[i].x-=0x100;
      if( f[i].x <= f[i].sx )
      {
        f[i].x = f[i].sx;
        f[i].returning = FALSE;
      }
    } else {
      f[i].x+=0x100;
      if( f[i].x >= f[i].ex )
      {
        f[i].x = f[i].ex;
        f[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 16, f[i].x>>8, f[i].y>>8, !f[i].returning ) )
      giddyhit();
  }
}

void renderdudes( void *v )
{
  struct fish *f = (struct fish *)v;
  int i, ff, lf;

  ff = ((frame>>3)&1)+16;
  lf = ((frame>>3)%3)+18;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; f[i].sx!=-1; i++ )
  {
    if( ( ((f[i].x>>8)-fgx) < -32 ) ||
        ( ((f[i].x>>8)-fgx) > 352 ) ||
        ( ((f[i].y>>8)-fgy) < -32 ) ||
        ( ((f[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[lf], (f[i].x>>8)-fgx, ((f[i].y>>8)+16)-fgy, !f[i].returning, 0.0f );
    render_sprite( &sprs[ff], (f[i].x>>8)-fgx, (f[i].y>>8)-fgy, !f[i].returning, 0.0f );
  }
}

/**************** DOG **********************/

// Uses fish structure, and setup code
void animatedogs( void *v )
{
  int i;
  struct fish *f = (struct fish *)v;

  for( i=0; f[i].sx!=-1; i++ )
  {
    if( f[i].returning )
    {
      f[i].x-=0x100;
      if( f[i].x <= f[i].sx )
      {
        f[i].x = f[i].sx;
        f[i].returning = FALSE;
      }
    } else {
      f[i].x+=0x100;
      if( f[i].x >= f[i].ex )
      {
        f[i].x = f[i].ex;
        f[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, ((frame>>2)&3)+7, f[i].x>>8, f[i].y>>8, !f[i].returning ) )
      giddyhit();
  }
}

void renderdogs( void *v )
{
  struct fish *f = (struct fish *)v;
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; f[i].sx!=-1; i++ )
  {
    if( ( ((f[i].x>>8)-fgx) < -32 ) ||
        ( ((f[i].x>>8)-fgx) > 352 ) ||
        ( ((f[i].y>>8)-fgy) < -32 ) ||
        ( ((f[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[((frame>>2)&3)+7], (f[i].x>>8)-fgx, (f[i].y>>8)-fgy, !f[i].returning, 0.0f );
  }
}

/**************** POOP *********************/
void initpoops( void *v )
{
  int i;
  struct poop *p = (struct poop *)v;

  for( i=0; p[i].x!=-1; i++ )
    p[i].active = TRUE;
}

void animatepoops( void *v )
{
  int i;
  struct poop *p = (struct poop *)v;

  for( i=0; p[i].x!=-1; i++ )
  {
    if( !p[i].active ) continue;
    if( giddydanger( SPRITEX, 13, p[i].x, p[i].y, FALSE ) )
    {
      giddyhit();
      p[i].active = FALSE;
      startincidental( p[i].x, p[i].y, 0, 0, GSPRITEX, bangframes, 5, 2 );
    }
  }
}

void renderpoops( void *v )
{
  int i;
  struct poop *p = (struct poop *)v;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; p[i].x!=-1; i++ )
  {
    if( !p[i].active )
      continue;

    if( ( (p[i].x-fgx) < -32 ) ||
        ( (p[i].x-fgx) > 352 ) ||
        ( (p[i].y-fgy) < -32 ) ||
        ( (p[i].y-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[((frame>>2)%3)+13], p[i].x-fgx, p[i].y-fgy, FALSE, 0.0f );
  }
}

/************** GRABBER MAGNETS **************/

void initgrabmags( void *v )
{
  int i;
  struct grabbermagnet *g = (struct grabbermagnet *)v;

  for( i=0; g[i].lx!=-1; i++ )
  {
    g[i].x = g[i].lx;
    g[i].y = g[i].ty;
    g[i].state = 0;
  }
}

void animategrabmags( void *v )
{
  int i;
  BOOL snd;
  struct grabbermagnet *g = (struct grabbermagnet *)v;

  for( i=0; g[i].lx!=-1; i++ )
  {
    switch( g[i].state )
    {
      case 0:
        if( gid.px > g[i].x )
        {
          g[i].x++;
          if( g[i].x > g[i].rx ) g[i].x = g[i].rx;
        } else if( gid.px < g[i].x ) {
          g[i].x--;
          if( g[i].x < g[i].lx ) g[i].x = g[i].lx;
        }

        if( ( g[i].x > (gid.px-4) ) &&
            ( g[i].x < (gid.px+4) ) )
          g[i].state++;
        break;
      
      case 1:
        g[i].y += 6;
        // Collision with ground
        if( g[i].y >= g[i].by )
        {
          g[i].y = g[i].by;

          snd  = startincidental( g[i].x, g[i].y, -2, -4, GSPRITEX, bangframes, 5, 2 );
          snd |= startincidental( g[i].x, g[i].y,  2, -4, GSPRITEX, bangframes, 5, 2 );

          if( snd ) enemysound( SND_CRUSHERHIT, sfxvol );

          g[i].state++;
        }
        break;
      
      case 2:
        g[i].y -= 2;
        if( g[i].y <= g[i].ty )
        {
          g[i].ty = g[i].y;
          g[i].state = 0;
        }
        break;
    }

    if( giddydanger( SPRITEX, 20, g[i].x, g[i].y, FALSE ) )
      giddyhit();
  }
}

void rendergrabmags( void *v )
{
  struct grabbermagnet *g = (struct grabbermagnet *)v;
  int i, y;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; g[i].lx!=-1; i++ )
  {
    if( ( (g[i].x-fgx) < -32 ) ||
        ( (g[i].x-fgx) > 352 ) )
      continue;
    glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
    for( y=((g[i].y-28)-fgy); y>-33; y-=33 )
      render_sprite( &sprs[21], g[i].x-fgx, y, FALSE, 0.0f );
    render_sprite( &sprs[20], g[i].x-fgx, g[i].y-fgy, FALSE, 0.0f );
  }
}


/**************** BAMBOO SPIKES **************/
void initspikes( void *v )
{
  int i;
  struct spike *s = (struct spike *)v;

  for( i=0; s[i].x!=-1; i++ )
  {
    s[i].yo = 0;
    s[i].timeout = 0;
    s[i].ya = -4;
  }
}

void animatespikes( void *v )
{
  int i;
  struct spike *s = (struct spike *)v;

  for( i=0; s[i].x!=-1; i++ )
  {
    if( s[i].timeout > 0 )
    {
      s[i].timeout--;
      if( ( s[i].timeout == 0 ) &&
          ( s[i].ya == -4 ) &&
          ( (s[i].x-fgx) > -16 ) &&
          ( (s[i].x-fgx) < 336 ) &&
          ( (s[i].y-fgy) > -20 ) &&
          ( (s[i].y-fgy) < 244 ) )
        enemysound( SND_WHOOSH, sfxvol );

      if( s[i].yo == 0 )
        continue;
    } else {
      if( s[i].ya < 0 )
      {
        s[i].yo += s[i].ya;
        if( s[i].yo <= -28 )
        {
          s[i].yo = -28;
          s[i].ya = 1;
          s[i].timeout = 60;
        }
      } else {
        s[i].yo += s[i].ya;
        if( s[i].yo >= 0 )
        {
          s[i].yo = 0;
          s[i].ya = -4;
          s[i].timeout = 100;
        }
      }
    }

    if( giddydanger( SPRITEX, 10, s[i].x, s[i].y+s[i].yo, FALSE ) )
      giddyhit();
  }
}

void renderspikes( void *v )
{
  struct spike *s = (struct spike *)v;
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; s[i].x!=-1; i++ )
  {
    if( ( (s[i].x-fgx) < -32 ) ||
        ( (s[i].x-fgx) > 352 ) ||
        ( ((s[i].y+s[i].yo)-fgy) < -32 ) ||
        ( ((s[i].y+s[i].yo)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[10], s[i].x-fgx, (s[i].y+s[i].yo)-fgy, TRUE, 0.0f );
  }
}

/************ POOPING BIRD *****************/
#define MAX_PBIRD_MISSILES 8
struct pbirdmissile
{
  int x, y, f;
  int tx;
};

struct pbirdmissile pbmsls[MAX_PBIRD_MISSILES];
int pbirdnextmsl = 0;

int poopsplashframes[] = { 19, 20, 21, 22, 23 };

void initpbirds( void *v )
{
  int i;
  struct pbird *p = (struct pbird *)v;

  for( i=0; i<MAX_PBIRD_MISSILES; i++ )
    pbmsls[i].x = -1;

  for( i=0; p[i].sx!=-1; i++ )
  {
    p[i].frame = 12*4;
    p[i].x = p[i].sx;
    if( p[i].x == p[i].rx )
      p[i].returning = TRUE;
    else
      p[i].returning = FALSE;
  }
}

void animatepbirds( void *v )
{
  int i;
  struct pbird *p = (struct pbird *)v;

  for( i=0; i<MAX_PBIRD_MISSILES; i++ )
  {
    if( pbmsls[i].x != -1 )
    {
      if( spritecolis( SPRITEX, pbmsls[i].f/3, pbmsls[i].x, pbmsls[i].y, FALSE ) )
      {
        startincidental( pbmsls[i].x, pbmsls[i].y, 0, 0, SPRITEX, poopsplashframes, 5, 2 );
        pbmsls[i].x = -1;
        continue;
      }

      if( pbmsls[i].f < (18*3) )
        pbmsls[i].f++;

      pbmsls[i].y += 4;

      if( giddydanger( SPRITEX, pbmsls[i].f/3, pbmsls[i].x, pbmsls[i].y, FALSE ) )
        giddyhit();
    }
  }

  for( i=0; p[i].sx!=-1; i++ )
  {
    if( p[i].shootcount <= 0 )
      p[i].shootcount = 120;
    else
      p[i].shootcount--;
    
    switch( p[i].shootcount )
    {
      case 120:
      case 110:
      case 100:
        if( ( ((p[i].x>>8)-fgx) >= -16 ) &&
            ( ((p[i].x>>8)-fgx) <  336 ) &&
            ( ((p[i].y>>8)-fgy) >= -16 ) &&
            ( ((p[i].y>>8)-fgy) <  216 ) )
        {
          pbmsls[pbirdnextmsl].x = p[i].x>>8;
          pbmsls[pbirdnextmsl].y = p[i].y>>8;
          pbmsls[pbirdnextmsl].f = 16*3;
          pbirdnextmsl = (pbirdnextmsl+1)%MAX_PBIRD_MISSILES;
          enemysound( SND_POOP, sfxvol );
        }
        break;
    }

    if( p[i].returning )
    {
      p[i].x-=0x180;
      if( p[i].x <= p[i].lx )
      {
        p[i].x = p[i].lx;
        p[i].returning = FALSE;
      }
    } else {
      p[i].x+=0x180;
      if( p[i].x >= p[i].rx )
      {
        p[i].x = p[i].rx;
        p[i].returning = TRUE;
      }
    }

    p[i].frame++;
    if( p[i].frame > 15*4 ) p[i].frame = 12*4;

    if( giddydanger( SPRITEX, p[i].frame/4, p[i].x>>8, p[i].y>>8, p[i].returning ) )
      giddyhit();
  }
}

void renderpbirds( void *v )
{
  int i;
  struct pbird *p = (struct pbird *)v;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; i<MAX_PBIRD_MISSILES; i++ )
  {
    if( pbmsls[i].x != -1 )
      render_sprite( &sprs[pbmsls[i].f/3], pbmsls[i].x-fgx, pbmsls[i].y-fgy, FALSE, 0.0f );
  }

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; p[i].sx!=-1; i++ )
  {
    if( ( ((p[i].x>>8)-fgx) < -32 ) ||
        ( ((p[i].x>>8)-fgx) > 352 ) ||
        ( ((p[i].y>>8)-fgy) < -32 ) ||
        ( ((p[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[p[i].frame/4], (p[i].x>>8)-fgx, (p[i].y>>8)-fgy, p[i].returning, 0.0f );
  }
}

/**************** MR. SNAPPY ***************/

// Uses fish structure, and setup code
void animatemrsnappys( void *v )
{
  int i, anysnappies;
  struct fish *f = (struct fish *)v;

  anysnappies = FALSE;
  for( i=0; f[i].sx!=-1; i++ )
  {
    if( f[i].returning )
    {
      f[i].x-=0x100;
      if( f[i].x <= f[i].sx )
      {
        f[i].x = f[i].sx;
        f[i].returning = FALSE;
      }
    } else {
      f[i].x+=0x100;
      if( f[i].x >= f[i].ex )
      {
        f[i].x = f[i].ex;
        f[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 31, f[i].x>>8, f[i].y>>8, f[i].returning ) )
      giddyhit();

    if( ( ((f[i].x>>8)-fgx) > -32 ) &&
        ( ((f[i].x>>8)-fgx) < 352 ) &&
        ( ((f[i].y>>8)-fgy) > -32 ) &&
        ( ((f[i].y>>8)-fgy) < 256 ) )
      anysnappies = 1;
  }

  if( ( anysnappies ) &&
      (( frame&7)==0) )
    enemysound( SND_MRSNAPPY, sfxvol );
}

void rendermrsnappys( void *v )
{
  struct fish *f = (struct fish *)v;
  int i, fm;

  fm = ((frame>>2)&1)+31;
  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; f[i].sx!=-1; i++ )
  {
    if( ( ((f[i].x>>8)-fgx) < -32 ) ||
        ( ((f[i].x>>8)-fgx) > 352 ) ||
        ( ((f[i].y>>8)-fgy) < -32 ) ||
        ( ((f[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[fm], (f[i].x>>8)-fgx, (f[i].y>>8)-fgy, f[i].returning, 0.0f );
  }
}

/************* SPACE FROG ******************/
#define MAX_SFROG_MISSILES 8
struct sfrogmissile
{
  int x, y, f;
  int dx;
};

struct sfrogmissile sfmsls[MAX_SFROG_MISSILES];
int sfrognextmsl = 0;

void initspacefrogs( void *v )
{
  int i;
  struct spacefrog *s = (struct spacefrog *)v;

  for( i=0; i<MAX_SFROG_MISSILES; i++ )
    sfmsls[i].x = -1;

  for( i=0; s[i].x!=-1; i++ )
  {
    s[i].y = s[i].ty;
    s[i].returning = FALSE;
    s[i].shootwait = 0;
  }
}

void animatespacefrogs( void *v )
{
  int i, shn;
  struct spacefrog *s = (struct spacefrog *)v;

  for( i=0; i<MAX_SFROG_MISSILES; i++ )
  {
    if( sfmsls[i].x == -1 ) continue;

    if( giddydanger( SPRITEX, sfmsls[i].f+9, sfmsls[i].x, sfmsls[i].y, sfmsls[i].dx > 0 ) )
    {
      if( startincidental( sfmsls[i].x, sfmsls[i].y, 0, 0, SPRITEX, shfbframes, 3, 3 ) )
        incidentalsound( SND_LASERHITWALL, sfxvol );
      giddyhit();
      sfmsls[i].x = -1;
    }

    if( spritecolis( SPRITEX, sfmsls[i].f+9, sfmsls[i].x, sfmsls[i].y, sfmsls[i].dx > 0 ) )
    {
      if( startincidental( sfmsls[i].x, sfmsls[i].y, 0, 0, SPRITEX, shfbframes, 3, 3 ) )
        incidentalsound( SND_LASERHITWALL, sfxvol );
      sfmsls[i].x = -1;
      continue;
    }

    sfmsls[i].x += sfmsls[i].dx;
  }

  shn = 0;
  for( i=0; s[i].x!=-1; i++ )
  {
    if( gid.px > s[i].x )
      s[i].flipped = 0;
    else
      s[i].flipped = 1;

    if( s[i].shootwait > 0 )
    {
      s[i].shootwait--;
    } else {
      s[i].shootwait = 80;

      if( ( (s[i].x-fgx) > -32 ) &&
          ( (s[i].x-fgx) < 352 ) &&
          ( (s[i].y-fgy) > -32 ) &&
          ( (s[i].y-fgy) < 256 ) )
      {
        sfmsls[sfrognextmsl].x = s[i].flipped ? s[i].x-6 : s[i].x+6;
        sfmsls[sfrognextmsl].y = s[i].y-4;
        sfmsls[sfrognextmsl].f = 0;
        sfmsls[sfrognextmsl].dx = s[i].flipped ? -6 : 6;
        sfrognextmsl = (sfrognextmsl+1)%MAX_SFROG_MISSILES;
        shn = 1;
      }
    }

    if( s[i].returning )
    {
      s[i].y--;
      if( s[i].y <= s[i].ty )
      {
        s[i].y = s[i].ty;
        s[i].returning = FALSE;
      }
    } else {
      s[i].y++;
      if( s[i].y >= s[i].by )
      {
        s[i].y = s[i].by;
        s[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 7, s[i].x, s[i].y, s[i].flipped ) )
      giddyhit();
  }

  if( shn )
    enemysound( SND_SPACEFROGSHOOT, sfxvol );
}

void renderspacefrogs( void *v )
{
  struct spacefrog *s = (struct spacefrog *)v;
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );

  for( i=0; i<MAX_SFROG_MISSILES; i++ )
  {
    if( sfmsls[i].x == -1 ) continue;

    if( ( (sfmsls[i].x-fgx) < -32 ) ||
        ( (sfmsls[i].x-fgx) > 352 ) ||
        ( (sfmsls[i].y-fgy) < -32 ) ||
        ( (sfmsls[i].y-fgy) > 256 ) )
      continue;

    render_sprite( &sprs[sfmsls[i].f+9], sfmsls[i].x-fgx, sfmsls[i].y-fgy, sfmsls[i].dx > 0, 0.0f );
  }

  for( i=0; s[i].x!=-1; i++ )
  {
    if( ( (s[i].x-fgx) < -32 ) ||
        ( (s[i].x-fgx) > 352 ) ||
        ( (s[i].y-fgy) < -32 ) ||
        ( (s[i].y-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[((frame>>2)&1)+7], s[i].x-fgx, s[i].y-fgy, s[i].flipped, 0.0f );
  }
}

/************** BLOB ROBOT *****************/
void animateblobrobots( void *v )
{
  int i;
  struct fish *f = (struct fish *)v;

  for( i=0; f[i].sx!=-1; i++ )
  {
    if( f[i].returning )
    {
      f[i].x-=0x180;
      if( f[i].x <= f[i].sx )
      {
        f[i].x = f[i].sx;
        f[i].returning = FALSE;
      }
    } else {
      f[i].x+=0x180;
      if( f[i].x >= f[i].ex )
      {
        f[i].x = f[i].ex;
        f[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 17, f[i].x>>8, f[i].y>>8, f[i].returning ) )
      giddyhit();
  }
}

void renderblobrobots( void *v )
{
  struct fish *f = (struct fish *)v;
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; f[i].sx!=-1; i++ )
  {
    if( ( ((f[i].x>>8)-fgx) < -32 ) ||
        ( ((f[i].x>>8)-fgx) > 352 ) ||
        ( ((f[i].y>>8)-fgy) < -32 ) ||
        ( ((f[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[((frame>>2)&3)+17], (f[i].x>>8)-fgx, (f[i].y>>8)-fgy, f[i].returning, 0.0f );
  }
}

/************** ZAPPER ********************/

#define MAX_ZAPPER_MISSILES 8
struct zappermissile
{
  int x, y, dx, dy;
  int bf;
};

struct zappermissile zprmsls[MAX_ZAPPER_MISSILES];
int zappernextmsl = 0;

int laserblastframes[] = { 31, 32, 33, 34, 35, 36 };

void initzappers( void *v )
{
  int i;
  struct zapper *z = (struct zapper *)v;

  for( i=0; i<MAX_ZAPPER_MISSILES; i++ )
    zprmsls[i].x = -1;

  for( i=0; z[i].x!=-1; i++ )
  {
    z[i].state = 0;
    z[i].timer = 0;
    z[i].flipped = 0;
  }
}

void animatezappers( void *v )
{
  int i, zapnoise;
  struct zapper *z = (struct zapper *)v;

  zapnoise = 0;
  for( i=0; i<MAX_ZAPPER_MISSILES; i++ )
  {
    if( zprmsls[i].x != -1 )
    {
      if( spritecolis( SPRITEX, zprmsls[i].bf, zprmsls[i].x, zprmsls[i].y, FALSE ) )
      {
        if( startincidental( zprmsls[i].x, zprmsls[i].y, 0, 0, SPRITEX, laserblastframes, 6, 2 ) )
          incidentalsound( SND_LASERHITWALL, sfxvol/2 );
        zprmsls[i].x = -1;
        continue;
      }

      zprmsls[i].x += zprmsls[i].dx;
      zprmsls[i].y += zprmsls[i].dy;

      if( giddydanger( SPRITEX, zprmsls[i].bf, zprmsls[i].x, zprmsls[i].y, FALSE ) )
        giddyhit();
    }
  }

  for( i=0; z[i].x!=-1; i++ )
  {
    if( gid.px > z[i].x )
      z[i].flipped = TRUE;
    else
      z[i].flipped = FALSE;

    if( z[i].timer > 0 )
    {
      z[i].timer--;
      continue;
    }

    z[i].state++;
    if( z[i].state > 3 ) z[i].state = 0;
    z[i].timer = 8;

    if( z[i].state == 3 )
    {
      if( ( (z[i].x-fgx) < -32 ) ||
          ( (z[i].x-fgx) > 352 ) ||
          ( (z[i].y-fgy) < -32 ) ||
          ( (z[i].y-fgy) > 256 ) )
        continue;
      zprmsls[zappernextmsl].x  = z[i].x;
      zprmsls[zappernextmsl].y  = z[i].upzapper ? (z[i].y-24) : (z[i].y+24);
      zprmsls[zappernextmsl].dx = z[i].flipped  ? 4 : -4;
      zprmsls[zappernextmsl].dy = z[i].upzapper ? -4 : 4;
      zprmsls[zappernextmsl].bf = z[i].upzapper ? (z[i].flipped?39:37) : (z[i].flipped?37:39);
      zappernextmsl = (zappernextmsl+1)%MAX_ZAPPER_MISSILES;
      zapnoise = 1;
    }

//    if( giddydanger( SPRITEX, p[i].frame/4, p[i].x>>8, p[i].y>>8, p[i].returning ) )
//      giddyhit();
  }

  if( zapnoise )
    enemysound( SND_LASERSHOOT, sfxvol/2 );
}

void renderzappers( void *v )
{
  int i, cx;
  struct zapper *z = (struct zapper *)v;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; i<MAX_ZAPPER_MISSILES; i++ )
  {
    if( zprmsls[i].x != -1 )
      render_sprite( &sprs[zprmsls[i].bf+(frame&1)], zprmsls[i].x-fgx, zprmsls[i].y-fgy, FALSE, 0.0f );
  }

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; z[i].x!=-1; i++ )
  {
    if( ( (z[i].x-fgx) < -32 ) ||
        ( (z[i].x-fgx) > 352 ) ||
        ( (z[i].y-fgy) < -32 ) ||
        ( (z[i].y-fgy) > 256 ) )
      continue;

    cx = z[i].x-fgx;
    if( z[i].flipped ) cx += 3;

    if( z[i].upzapper )
    {
      render_sprite( &sprs[21+z[i].state], cx, (z[i].y-23)-fgy, z[i].flipped, 0.0f );
      render_sprite( &sprs[25], z[i].x-fgx, (z[i].y-7)-fgy, FALSE, 0.0f );
    } else {
      render_sprite( &sprs[26+z[i].state], cx, (z[i].y+23)-fgy, z[i].flipped, 0.0f );
      render_sprite( &sprs[30], z[i].x-fgx, (z[i].y+7)-fgy, FALSE, 0.0f );
    }
  }
}


/************** FIXED LASER ********************/

int flyo[] = { 5, 4, 5, 6, 7, 8 };
int flhs[] = { 7, 9, 7, 5, 3, 1 };

void initfixedlasers( void *v )
{
  int i;
  struct fixedlaser *l = (struct fixedlaser *)v;

  for( i=0; l[i].x!=-1; i++ )
    l[i].time = l[i].wait+l[i].idelay;
}

void animatefixedlasers( void *v )
{
  int i;
  struct fixedlaser *l = (struct fixedlaser *)v;

  for( i=0; l[i].x!=-1; i++ )
  {
    if( l[i].time > 0 )
      l[i].time--;
    else
      l[i].time = l[i].wait;

    if( l[i].time == 20 )
    {
      if( ( (l[i].x-fgx) > -l[i].w ) &&
          ( (l[i].x-fgx) < 320) &&
          ( (l[i].y-fgy) > -18 ) &&
          ( (l[i].y-fgy) < 258 ) )
        enemysound( SND_FIXEDLASERFIRE, sfxvol );
    }


    l[i].f1 = 5-(l[i].time/4);
    l[i].f2 = 43;
    if( ( l[i].f1 >= 0 ) && ( l[i].f1 < 3 ) )
      l[i].f2 = 44;
    
    if( l[i].f1 >= 0 )
    {
      if( ( gid.px >= l[i].x ) &&
          ( gid.px < (l[i].x+l[i].w) ) &&
          ( gid.py >= (l[i].y+(flyo[l[i].f1]-10)) ) &&
          ( gid.py < (l[i].y+(flyo[l[i].f1]+flhs[l[i].f1]+10)) ) )
        giddyhit();
    }
  }
}

void renderfixedlasers( void *v )
{
  int i;
  struct fixedlaser *l = (struct fixedlaser *)v;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; l[i].x!=-1; i++ )
  {
    if( ( (l[i].x-fgx) < -l[i].w ) ||
        ( (l[i].x-fgx) > 320) ||
        ( (l[i].y-fgy) < -18 ) ||
        ( (l[i].y-fgy) > 258 ) )
      continue;
    
    if( l[i].f1 >= 0 )
      render_sprite_tl_stretch( &sprs[45+l[i].f1], l[i].x-fgx, (l[i].y+flyo[l[i].f1])-fgy, l[i].w, sprs[45+l[i].f1].fh, FALSE );

    render_sprite_tl( &sprs[l[i].f2], l[i].x-fgx, l[i].y-fgy, FALSE );
  }
}

/************** UPPY DOWNY *********************/

void inituppydownies( void *v )
{
  int i;
  struct uppydowny *u = (struct uppydowny *)v;

  for( i=0; u[i].x!=-1; i++ )
  {
    u[i].y = u[i].sy;
    u[i].dy = 1;
  }
}

void animateuppydownies( void *v )
{
  int i;
  struct uppydowny *u = (struct uppydowny *)v;

  for( i=0; u[i].x!=-1; i++ )
  {
    u[i].y += u[i].dy;
    if( u[i].y >= u[i].by )
    {
      u[i].y = u[i].by;
      u[i].dy = -1;
    }
    if( u[i].y <= u[i].ty )
    {
      u[i].y = u[i].ty;
      u[i].dy = 1;
    }

    switch( u[i].type )
    {
      case 0:
        if( giddydanger( SPRITEX, 54, u[i].x, u[i].y, FALSE ) )
          giddyhit();
        break;
    }
  }
}

void renderuppydownies( void *v )
{
  int i;
  struct uppydowny *u = (struct uppydowny *)v;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; u[i].x!=-1; i++ )
  {
    if( ( (u[i].x-fgx) < -32 ) ||
        ( (u[i].x-fgx) > 352 ) ||
        ( (u[i].y-fgy) < -40 ) ||
        ( (u[i].y-fgy) > 240 ) )
      continue;
    render_sprite( &sprs[((frame>>2)%5)+54], u[i].x-fgx, u[i].y-fgy, FALSE, 0.0f );
  }
}

/**************** BIG DRIP *********************/

void initbigdrips( void *v )
{
  int i;
  struct bigdrip *b = (struct bigdrip *)v;

  for( i=0; b[i].x!=-1; i++ )
  {
    b[i].y = b[i].sy;
  }
}

void animatebigdrips( void *v )
{
  int i, ddh;
  struct bigdrip *b = (struct bigdrip *)v;

  ddh = 0;
  for( i=0; b[i].x!=-1; i++ )
  {
    b[i].y += 4;
    if( b[i].y > b[i].ey )
    {
      if( spritecolis( SPRITEX, 59, b[i].x, b[i].y, FALSE ) )
      {
        ddh |= startincidental( b[i].x, b[i].y, 0, 0, GSPRITEX, bangframes, 5, 2 );
        b[i].y = b[i].sy;
      }
    }
    if( giddydanger( SPRITEX, 59, b[i].x, b[i].y, FALSE ) )
      giddyhit();
  }

  if( ddh )
    enemysound( SND_DRIPHIT, sfxvol );
}

void renderbigdrips( void *v )
{
  int i;
  struct bigdrip *b = (struct bigdrip *)v;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; b[i].x!=-1; i++ )
  {
    if( ( (b[i].x-fgx) < -32 ) ||
        ( (b[i].x-fgx) > 352 ) ||
        ( (b[i].y-fgy) < -40 ) ||
        ( (b[i].y-fgy) > 240 ) )
      continue;
    render_sprite( &sprs[((frame>>2)&1)+59], b[i].x-fgx, b[i].y-fgy, FALSE, 0.0f );
  }
}

/**************** SPACE SLUG ***************/

// Uses fish structure, and setup code
void animatespaceslugs( void *v )
{
  int i;
  struct fish *f = (struct fish *)v;

  for( i=0; f[i].sx!=-1; i++ )
  {
    if( f[i].returning )
    {
      f[i].x-=0xc0;
      if( f[i].x <= f[i].sx )
      {
        f[i].x = f[i].sx;
        f[i].returning = FALSE;
      }
    } else {
      f[i].x+=0xc0;
      if( f[i].x >= f[i].ex )
      {
        f[i].x = f[i].ex;
        f[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, 61, f[i].x>>8, f[i].y>>8, !f[i].returning ) )
      giddyhit();
  }
}

void renderspaceslugs( void *v )
{
  struct fish *f = (struct fish *)v;
  int i, fm;

  fm = ((frame>>2)&3)+61;
  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; f[i].sx!=-1; i++ )
  {
    if( ( ((f[i].x>>8)-fgx) < -32 ) ||
        ( ((f[i].x>>8)-fgx) > 352 ) ||
        ( ((f[i].y>>8)-fgy) < -32 ) ||
        ( ((f[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[fm], (f[i].x>>8)-fgx, (f[i].y>>8)-fgy, !f[i].returning, 0.0f );
  }
}

/****************** FIREBALL ***************/

void initfireballs( void *v )
{
  int i;
  struct fireball *f = (struct fireball *)v;
  
  for( i=0; f[i].x!=-1; i++ )
  {
    f[i].y = f[i].iy;
    f[i].dy = f[i].idy;
    f[i].delay = f[i].idelay;
  }
}

void animatefireballs( void *v )
{
  int i;
  struct fireball *f = (struct fireball *)v;
  
  for( i=0; f[i].x!=-1; i++ )
  {
    if( f[i].delay > 0 )
    {
      f[i].delay--;
      if( f[i].delay == 0 )
      {
        if( ( (f[i].x-fgx) > -32 ) &&
            ( (f[i].x-fgx) < 352 ) &&
            ( ((f[i].y>>8)-fgy) > -32 ) &&
            ( ((f[i].y>>8)-fgy) < 256 ) )
          enemysound( SND_FIREBALL, sfxvol );
      }
      continue;
    }

    f[i].y += f[i].dy;
    if( f[i].dy < 0x600 )
      f[i].dy += 0x30;

    if( ( f[i].dy > 0 ) &&
        ( f[i].y >= f[i].iy ) )
    {
      f[i].y = f[i].iy;
      f[i].dy = f[i].idy;
      f[i].delay = f[i].bdelay;
    }

    if( giddydanger( PSPRITEX, 33, f[i].x, f[i].y>>8, FALSE ) )
      giddyhit();
  }
}

void renderfireballs( void *v )
{
  int i;
  struct fireball *f = (struct fireball *)v;
  
  glBindTexture( GL_TEXTURE_2D, tex[PSPRITEX] );
  for( i=0; f[i].x!=-1; i++ )
  {
    if( ( (f[i].x-fgx) < -32 ) ||
        ( (f[i].x-fgx) > 352 ) ||
        ( ((f[i].y>>8)-fgy) < -32 ) ||
        ( ((f[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &psprs[33+((frame>>1)&1)], f[i].x-fgx, (f[i].y>>8)-fgy, f[i].dy <= 0, f[i].dy > 0 ? 0 : 180 );
  }
}

/****************** BIRDIE *****************/

int birdieframe;
int nexttweet=0;

// Uses fish structure, and setup code
void animatebirdies( void *v )
{
  int i, anytweet;
  struct fish *b = (struct fish *)v;

  birdieframe = 33+((frame>>2)&3);

  anytweet = 0;
  for( i=0; b[i].sx!=-1; i++ )
  {
    if( b[i].returning )
    {
      b[i].x-=0x180;
      if( b[i].x <= b[i].sx )
      {
        b[i].x = b[i].sx;
        b[i].returning = FALSE;
      }
    } else {
      b[i].x+=0x180;
      if( b[i].x >= b[i].ex )
      {
        b[i].x = b[i].ex;
        b[i].returning = TRUE;
      }
    }

    if( giddydanger( SPRITEX, birdieframe, b[i].x>>8, b[i].y>>8, !b[i].returning ) )
      giddyhit();

    if( ( ((b[i].x>>8)-fgx) > -32 ) &&
        ( ((b[i].x>>8)-fgx) < 352 ) &&
        ( ((b[i].y>>8)-fgy) > -32 ) &&
        ( ((b[i].y>>8)-fgy) < 256 ) )
      anytweet = 1;
  }

  if( nexttweet > 0 )
  {
    nexttweet--;
  } else {
    if( anytweet )
    {
      enemysound( SND_BIRDIE, sfxvol );
      nexttweet = 10 + (rand()%4)*20;
    }
  }
}

void renderbirdies( void *v )
{
  struct fish *b = (struct fish *)v;
  int i;

  glBindTexture( GL_TEXTURE_2D, tex[SPRITEX] );
  for( i=0; b[i].sx!=-1; i++ )
  {
    if( ( ((b[i].x>>8)-fgx) < -32 ) ||
        ( ((b[i].x>>8)-fgx) > 352 ) ||
        ( ((b[i].y>>8)-fgy) < -32 ) ||
        ( ((b[i].y>>8)-fgy) > 256 ) )
      continue;
    render_sprite( &sprs[birdieframe], (b[i].x>>8)-fgx, (b[i].y>>8)-fgy, !b[i].returning, 0.0f );
  }
}

/***************************** LEVEL 1 ENEMIES *************************/

struct wasp wasps1[] = { {   22<<8,  322<<8, 352<<8, },
                         {  366<<8,  666<<8, 320<<8, },
                         {  718<<8, 1018<<8, 288<<8, },
                         { 1466<<8, 1766<<8, 296<<8, },
                         { 1852<<8, 2152<<8, 296<<8, },
                         { 2266<<8, 2566<<8, 306<<8, },
                         { 3400<<8, 3700<<8, 360<<8, },
                         { 4692<<8, 4992<<8, 322<<8, },
                         { 5470<<8, 5770<<8, 322<<8, },
                         { 5946<<8, 6246<<8, 322<<8, },
                         { -1, } };

struct spider spiders1[] = { { 1440<<8, 294<<8, 364<<8 },
                             { 4212<<8, 312<<8, 380<<8 },
                             { 4304<<8, 312<<8, 380<<8 },
                             { 4432<<8, 312<<8, 380<<8 },
                             { 4496<<8, 312<<8, 380<<8 },
                             { -1, } };

struct shroom shrooms1[] = { { 1778<<8, 1934<<8, 342<<8 },
                             { 4032<<8, 4188<<8, 390<<8 },
                             { 4560<<8, 4716<<8, 390<<8 },
                             { 6064<<8, 6220<<8, 406<<8 },
                             { -1, } };

struct shootyf shootyf1[] = { { 2042<<8, 354<<8, },
                              { 3250<<8, 458<<8, },
                              { 3744<<8, 458<<8, },
                              { 5854<<8, 404<<8, },
                              { -1, } };

struct mine mines1[] = { { 3210, 1024, },
                         { 2856,  992, },
                         { 2600,  976, },
                         { 2248,  928, },
                         { 3576,  992, },
                         { 4040,  850, },
                         { -1, } };

struct fish fish1[] = { { 3008<<8, 3130<<8, 918<<8 },
                        { 2500<<8, 2622<<8, 774<<8 },
                        { 1922<<8, 2044<<8, 826<<8 },
                        { 3672<<8, 3794<<8, 922<<8 },
                        { 4164<<8, 4286<<8, 792<<8 },
                        { 4020<<8, 4142<<8, 936<<8 },
                        { 4436<<8, 4556<<8, 872<<8 },
                        { 5176<<8, 5316<<8, 888<<8 },
                        { -1, } };

struct enemy enemies1b[] = { { initspiders, animatespiders, renderspiders, (void *)spiders1 },
                             { initshootyf, animateshootyf, rendershootyf, (void *)shootyf1 },
                             { initmines,   animatemines,   rendermines,   (void *)mines1   },
                             { NULL, } };

struct enemy enemies1[]  = { { initwasps,   animatewasps,   renderwasps,   (void *)wasps1   },
                             { initshrooms, animateshrooms, rendershrooms, (void *)shrooms1 },
                             { initfish,    animatefish,    renderfish,    (void *)fish1    },
                             { NULL, } };


/***************************** LEVEL 2 ENEMIES *************************/

struct steam steam2[] = { {  244, 1840, -0x480, 0, },
                          {  434, 1950, -0x480, 0, },
                          { 2308, 1394, -0x480, 0, },
                          { 2602, 1442,  0x480, 0, },
                          { 3606,  948,  0x480, 0, },
                          { 1464, 1426,  0x480, 0, },
                          { 1284, 1392, -0x480, 0, },
                          {  424,  944,  0x480, 0, },
                          {  228,  944, -0x480, 0, },
                          { 2388,  496, -0x480, 0, },
                          { 2520,  546,  0x480, 0, },
                          { 2776,   50,  0x480, 0, },
                          { 1160,   48,  0x480, 0, },
                          { 1094,  142, -0x480, 0, },
                          { 3240,  962,  0x480, 0, },
                          { 3140,  962, -0x480, 0, },
                          {  404,  526, -0x480, 0, },
                          { -1, } };

struct barrel barrels2[] = { { 920, 1730, 2046,   0, },
                             { -1, } };

struct grabber grabbers2[] = { { 1106, 1230, 1859, 1877, 1948, },
                               {  914, 1072, 1409, 1427, 1498, },
                               { 1506, 1645,   66,   84,  152, },
                               { 1522, 1661,  960,  978, 1068, },
                               { -1, } };

struct minitank minitanks2[] = { { 1696<<8, 1696<<8, 1760<<8, 1925<<8 },
                                 { 1900<<8, 1900<<8, 2596<<8,  181<<8 },
                                 { 2596<<8, 1900<<8, 2596<<8,  181<<8 },
                                 { 2276<<8, 2192<<8, 2276<<8,  997<<8 },
                                 { 2930<<8, 2782<<8, 2930<<8, 1972<<8 },
                                 {  526<<8,  526<<8,  768<<8,  630<<8 },
                                 { -1, } };

struct crusher crushers2[] = { { 2434<<8, 1380<<8, 1490<<8, 100 },
                               { 2690<<8, 1380<<8, 1490<<8, 100 },
                               { 1664<<8, 1380<<8, 1474<<8,  80 },
                               {  688<<8, 1380<<8, 1490<<8, 100 },
                               {  560<<8, 1380<<8, 1490<<8, 100 },
                               {  432<<8, 1380<<8, 1490<<8, 100 },
                               {  928<<8,  932<<8, 1010<<8,  80 },
                               {  736<<8,  932<<8, 1010<<8,  80 },
                               { 2640<<8,  932<<8, 1046<<8,  80 },
                               { 2368<<8,  932<<8, 1046<<8,  80 },
                               { -1, } };

struct fish dudes2[] = { { 2475<<8, 2644<<8, 1516<<8 },
                         { 3516<<8, 3686<<8, 1068<<8 },
                         {  938<<8, 1064<<8, 1516<<8 },
                         {  170<<8,  678<<8,  170<<8 },
                         { -1, } };

struct enemy enemies2b[]  = { { NULL, } };

struct enemy enemies2[] = { { initsteam,     animatesteam,     rendersteam,     (void *)steam2 },
                            { initbarrels,   animatebarrels,   renderbarrels,   (void *)barrels2 },
                            { initgrabbers,  animategrabbers,  rendergrabbers,  (void *)grabbers2 },
                            { initminitanks, animateminitanks, renderminitanks, (void *)minitanks2 },
                            { initcrushers,  animatecrushers,  rendercrushers,  (void *)crushers2 },
                            { initfish,      animatedudes,     renderdudes,     (void *)dudes2 },
                            { NULL, } };


/***************************** LEVEL 3 ENEMIES *************************/

struct spike spikes3[] = { { 1024, 556, },
                           { 1168, 652, },
                           { 1312, 748, },
                           { 2520, 732, },
                           { 1328, 108, },
                           { 1232, 108, },
                           { 3076, 652, },
                           { 3440, 588, },
                           { -1, } };

struct pbird pbirds3[] = { { 1540<<8, 1540<<8, 1754<<8,  752<<8 },
                           { 2636<<8, 2636<<8, 2816<<8,  722<<8 },
                           { 1976<<8, 1976<<8, 2124<<8,  314<<8 },
                           { 2928<<8, 2928<<8, 3104<<8,  448<<8 },
                           { 4306<<8, 4306<<8, 4492<<8,  440<<8 },
                           { 5368<<8, 5368<<8, 5526<<8,  416<<8 },
                           { -1, } };

struct shroom shrooms3[] = { { 1870<<8, 2048<<8, 866<<8 },
                             { 2416<<8, 2576<<8, 866<<8 },
                             { 2000<<8, 2160<<8, 658<<8 },
                             { 2176<<8, 2352<<8, 594<<8 },
                             { 3056<<8, 3200<<8, 770<<8 },
                             { 5760<<8, 5972<<8, 642<<8 },
                             { -1, } };

struct fish mrsnappys3[] = { { 4864<<8, 5584<<8,  628<<8 },
                             { 4768<<8, 5150<<8,  356<<8 },
                             { 6106<<8, 6272<<8,  900<<8 },
                             { 6368<<8, 6642<<8,  772<<8 },
                             { 5884<<8, 6048<<8, 1076<<8 },
                             { 4524<<8, 4832<<8,  980<<8 },
                             { 4352<<8, 4736<<8, 1092<<8 },
                             { 4032<<8, 4210<<8,  964<<8 },
                             { 3744<<8, 3936<<8,  852<<8 },
                             { -1, } };

struct fireball fireballs3[] = { { 5184, 1080<<8, -0x700,  0, 30, },
                                 { 5104, 1080<<8, -0x700, 50, 30, },
                                 { 3712, 1096<<8, -0x700,  0, 30, },
                                 { 3632, 1096<<8, -0x700, 50, 30, },
                                 { -1, } };

struct enemy enemies3b[] = { { initspikes,  animatespikes,  renderspikes,  (void *)spikes3 },
                             { initpbirds,  animatepbirds,  renderpbirds,  (void *)pbirds3 },
                             { NULL, } };

struct enemy enemies3[]  = { { initshrooms,   animateshrooms,   rendershrooms,   (void *)shrooms3 },
                             { initfish,      animatemrsnappys, rendermrsnappys, (void *)mrsnappys3 },
                             { initfireballs, animatefireballs, renderfireballs, (void *)fireballs3 },
                             { NULL, } };

/***************************** LEVEL 4 ENEMIES *************************/

struct fish dogs4[] = { {  978<<8, 1132<<8, 417<<8 },
                        { 1646<<8, 1803<<8, 417<<8 },
                        { 2370<<8, 2528<<8, 417<<8 },
                        { 6246<<8, 6406<<8, 417<<8 },
                        { -1, } };

struct poop poops4[] = { { 2385,  426, },
                         { 2736,  426, },
                         { 3600,  426, },
                         { 3808,  426, },
                         { 4401,  426, },
                         { 6512,  426, },
                         { -1, } };

struct grabbermagnet grabmags4[] = { { 528, 722, 278, 421 },
                                     {  68, 184, 278, 421 },
                                     { -1, } };

struct fish birdies4[] = { { 5322<<8, 5448<<8, 212<<8, },
                           { 5520<<8, 5632<<8,  64<<8, },
                           { 5558<<8, 5674<<8,  48<<8, },
                           { 5590<<8, 5712<<8,  32<<8, },
                           { 5820<<8, 5930<<8, 112<<8, },
                           { 5852<<8, 5954<<8,  90<<8, },
                           { 5884<<8, 5998<<8,  58<<8, },
                           { 5960<<8, 6074<<8,  90<<8, },
                           { 6146<<8, 6288<<8, 106<<8, },
                           { 6126<<8, 6262<<8,  74<<8, },
                           { 6616<<8, 6816<<8,  76<<8, },
                           { 6646<<8, 6856<<8,  60<<8, },
                           { 6626<<8, 6842<<8,  44<<8, },
                           { 6666<<8, 6886<<8,  28<<8, },
                           { 6972<<8, 7102<<8, 106<<8, },
                           { 6988<<8, 7140<<8,  90<<8, },
                           { -1, } };

struct enemy enemies4b[] = { { NULL, } };

struct enemy enemies4[]  = { { initfish,     animatedogs,     renderdogs,     (void *)dogs4 },
                             { initpoops,    animatepoops,    renderpoops,    (void *)poops4 },
                             { initgrabmags, animategrabmags, rendergrabmags, (void *)grabmags4 },
                             { initfish,     animatebirdies,  renderbirdies,  (void *)birdies4 },
                             { NULL, } };


/***************************** LEVEL 5 ENEMIES *************************/

struct spacefrog spacefrogs5[] = { {  432,  396,  496, TRUE, },
                                   {  624, 1374, 1472, TRUE, },
                                   { 1296, 1630, 1758, TRUE, },
                                   { 1678, 1630, 1758, TRUE, },
                                   {  112, 1932, 2032, TRUE, },
                                   { 1438,  924, 1030, TRUE, },
                                   {  624, 1738, 1824, TRUE, },
                                   { 1986,  500,  608, TRUE, },
                                   { 2100,  486,  580, TRUE, },
                                   { -1, } };

struct fish blobrobots5[] = { {  768<<8,  896<<8,  577<<8, },
                              {  992<<8, 1120<<8,  577<<8, },
                              {  426<<8,  594<<8, 1184<<8, },
                              { -1, } };

struct zapper zappers5[] = { { 1104, 1777, TRUE, },
                             { 1840, 1777, TRUE, },
                             { 1024, 1903, FALSE, },
                             { 1488, 1903, FALSE, },
                             { 2176, 2065, TRUE, },
                             { 2304, 2065, TRUE, },
                             { 2432, 2065, TRUE, },
                             { 1136,  910, FALSE, },
                             { 1728, 1169, TRUE, },
                             { 1840, 1169, TRUE, },
                             {  160, 1327, FALSE, },
                             { -1, } };

struct fixedlaser flasers5[] = { {  591, 2023, 189, 80,  0, },
                                 { 1295,  648, 125, 80,  0, },
                                 { 1263,  744, 125, 80, 20, },
                                 { 1295,  840, 125, 80, 40, },
                                 { 2447,  936, 221, 80,  0, },
                                 {   79, 1528,  93, 80,  0, },
                                 {  399, 1672,  93, 80,  0, },
                                 { -1, } };

struct uppydowny uppydownies5[] = { {  454, 2440, 2512, 2440, 0, },
                                    {  664, 2440, 2512, 2480, 0, },
                                    { 1552, 2356, 2444, 2356, 0, },
                                    { 1728, 2372, 2460, 2460, 0, },
                                    { -1, } };

struct bigdrip bigdrips5[] = { {  568, 2316, 2445, },
                               {  854, 2316, 2445, },
                               { 1368, 2316, 2445, },
                               { 1880, 2316, 2445, },
                               { 2026, 2316, 2445, },
                               { 2104, 2316, 2445, },
                               { 2184, 2316, 2445, },
                               { 2264, 2316, 2445, },
                               { -1, } };

struct fish spaceslugs5[] = { {  742<<8,  824<<8, 2486<<8 },
                              { 1512<<8, 1592<<8, 2486<<8 },
                              { 1880<<8, 1988<<8, 2486<<8 },
                              { 2316<<8, 2400<<8, 2486<<8 },
                              { -1, } };

struct enemy enemies5b[] = { { initbigdrips,    animatebigdrips,    renderbigdrips,    (void *)bigdrips5 },
                             { NULL, } };

struct enemy enemies5[]  = { { initspacefrogs,  animatespacefrogs,  renderspacefrogs,  (void *)spacefrogs5 },
                             { initfish,        animateblobrobots,  renderblobrobots,  (void *)blobrobots5 },
                             { initzappers,     animatezappers,     renderzappers,     (void *)zappers5 },
                             { initfixedlasers, animatefixedlasers, renderfixedlasers, (void *)flasers5 },
                             { inituppydownies, animateuppydownies, renderuppydownies, (void *)uppydownies5 },
                             { initfish,        animatespaceslugs,  renderspaceslugs,  (void *)spaceslugs5 },
                             { NULL, } };



struct enemy *enemies[]  = { enemies1,  enemies2,  enemies3,  enemies4,  enemies5  };
struct enemy *enemiesb[] = { enemies1b, enemies2b, enemies3b, enemies4b, enemies5b };
