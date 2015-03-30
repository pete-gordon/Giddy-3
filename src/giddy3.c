
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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
#else
#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef __HAIKU__
#include <libgen.h>
#include <unistd.h>
#endif

#ifdef __PSP__
#include "psp/psp.h"
#endif

#include "giddy3.h"
#include "render.h"
#include "samples.h"
#include "titles.h"

#ifdef __amigaos4__
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/picasso96api.h>
extern struct Library *MiniGLBase;
//struct Library *P96Base;
//struct P96IFace *IP96;
struct Window *win;
//BOOL screenbodge = 0;
#endif

#if defined(__amigaos4__) || defined(__MORPHOS__)
TEXT USED ver[] = "\0$VER: Giddy3 1.5 (30.03.2015)";
#endif

extern time_t gamestarttime;
BOOL needquit = FALSE, audioavailable = FALSE;

BOOL pleft=FALSE, pright=FALSE, pjump=FALSE, spacehit=FALSE, enterhit=FALSE;
BOOL jleft=FALSE, jright=FALSE, jjump=FALSE;
BOOL gleft=FALSE, gright=FALSE, gjump=FALSE;
BOOL paused,fullscreen=FALSE;

extern int keyleft, keyright, keyjump, keyuse, keyhint, endingstate;
extern int keyleft2, keyright2, keyjump2, keyuse2, keyhint2;
extern int sfxvol, musicvol, sfxvolopt, musicvolopt;

SDL_Surface *ssrf;
SDL_Joystick *joy;

int mousx, mousy;

extern struct what_is_giddy_doing gid;
extern int fgx, fgy, titlestate;

extern Mix_Music *moozak;

int what_are_we_doing = WAWD_TITLES;

BOOL musicon = TRUE;

void save_options( void )
{
  FILE *f;

#ifndef __APPLE__

  f = fopen( "settings.txt", "w" );

#else

  char *home_dir = getenv( "HOME" );
  
  if ( home_dir != NULL )
  {
    char *rel_path = "/Library/Preferences/Giddy3 Preferences.txt";
    char *full_path = calloc( strlen( home_dir ) + strlen( rel_path ) + 1, 1 );
	
	strcpy( full_path, home_dir );
	strcat( full_path, rel_path );
    
    f = fopen( full_path, "w" );
	
	free( full_path );
  }

#endif
  
  if( !f ) return;

  fprintf( f, "sfxvol %d\n", sfxvolopt );
  fprintf( f, "musicvol %d\n", musicvolopt );
  fprintf( f, "keyleft %d\n", keyleft );
  fprintf( f, "keyright %d\n", keyright );
  fprintf( f, "keyjump %d\n", keyjump );
  fprintf( f, "keyuse %d\n", keyuse );
  fprintf( f, "keyhint %d\n", keyhint );
  fprintf( f, "fullscr %d\n", (int)fullscreen );
  fclose( f );
}

void load_options( void )
{
  FILE *f;
  int i;
  char ltmp[80];

#ifndef __APPLE__

  f = fopen( "settings.txt", "r" );

#else

  char *home_dir = getenv( "HOME" );
  
  if ( home_dir != NULL )
  {
    char *rel_path = "/Library/Preferences/Giddy3 Preferences.txt";
    char *full_path = calloc( strlen( home_dir ) + strlen( rel_path ) + 1, 1 );
	
	strcpy( full_path, home_dir );
	strcat( full_path, rel_path );
    
    f = fopen( full_path, "r" );
	
	free( full_path );
  }

#endif
  
  if( !f ) return;

  while( !feof( f ) )
  {
    if( fgets( ltmp, 80, f ) == NULL )
      break;

    if( strncmp( ltmp, "sfxvol ", 7 ) == 0 )
    {
      i = atoi( &ltmp[7] );
      if( ( i >= 0 ) && ( i <= 8 ) )
      {
        sfxvolopt = i;
        sfxvol = (sfxvolopt * ((MIX_MAX_VOLUME*5)/6))/8;
      }
      continue;
    }

    if( strncmp( ltmp, "musicvol ", 9 ) == 0 )
    {
      i = atoi( &ltmp[9] );
      if( ( i >= 0 ) && ( i <= 8 ) )
      {
        musicvolopt = i;
        musicvol = (musicvolopt * ((MIX_MAX_VOLUME*5)/6))/8;
      }
      continue;
    }

    if( strncmp( ltmp, "keyleft ", 8 ) == 0 )
    {
      i = atoi( &ltmp[8] );
      do_define_a_key( &keyleft, &keyleft2, i );
      continue;
    }

    if( strncmp( ltmp, "keyright ", 9 ) == 0 )
    {
      i = atoi( &ltmp[9] );
      do_define_a_key( &keyright, &keyright2, i );
      continue;
    }

    if( strncmp( ltmp, "keyjump ", 8 ) == 0 )
    {
      i = atoi( &ltmp[8] );
      do_define_a_key( &keyjump, &keyjump2, i );
      continue;
    }

    if( strncmp( ltmp, "keyuse ", 7 ) == 0 )
    {
      i = atoi( &ltmp[7] );
      do_define_a_key( &keyuse, &keyuse2, i );
      continue;
    }

    if( strncmp( ltmp, "keyhint ", 8 ) == 0 )
    {
      i = atoi( &ltmp[8] );
      do_define_a_key( &keyhint, &keyhint2, i );
      continue;
    }

    if( strncmp( ltmp, "fullscr ", 8 ) == 0 )
    {
      fullscreen = atoi( &ltmp[8] ) ? TRUE : FALSE;
      continue;
    }
  }    

  fclose( f );
}

#ifdef HAVE_GLES
// EGL context variables
EGLDisplay          eglDisplay    = NULL;
EGLContext          eglContext    = NULL;
EGLSurface          eglSurface    = NULL;
#endif

BOOL init( void )
{
  const SDL_VideoInfo *info = NULL;
  int depth;
  int flags;

  load_options();

  if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK ) < 0 )
  {
    fprintf( stderr, "SDL says: %s\n", SDL_GetError() );
    return FALSE;
  }

  joy = SDL_JoystickOpen( 0 );
  if( joy ) SDL_JoystickEventState( SDL_ENABLE );
  
//  printf( "%p\n", joy );

  needquit = TRUE;

  depth = 32;
  if( (info = SDL_GetVideoInfo()) )
    depth = info->vfmt->BitsPerPixel;

#ifdef __amigaos4__
  printf( "Detected MiniGL v%d.%d\n",
    MiniGLBase->lib_Version,
    MiniGLBase->lib_Revision );

  if( ( MiniGLBase->lib_Version < 2 ) ||
      ( ( MiniGLBase->lib_Version == 2 ) &&
        ( MiniGLBase->lib_Revision < 1 ) ) )
  {
    //screenbodge = TRUE;
    printf( "WARNING: The video screen won't work properly on the spaceship level\n" );
    printf( "with this version on MiniGL.\n" );
  }
#endif

#ifdef HAVE_GLES
flags = 0;
#else
flags = SDL_OPENGL;
#endif

#ifndef PANDORA
  if( fullscreen )
#endif
	flags |= SDL_FULLSCREEN;

#ifdef PANDORA
  ssrf = SDL_SetVideoMode( 800, 480, depth, flags );
#else
  ssrf = SDL_SetVideoMode( WINWIDTH, WINHEIGHT, depth, flags );
#endif
	
  if( ssrf == 0 )
  {
    fprintf( stderr, "SDL says: %s\n", SDL_GetError() );
    return FALSE;
  }

  SDL_ShowCursor( SDL_DISABLE );
#ifdef HAVE_GLES
// create EGL context
// *TODO* : real egl Checking for error
#define eglCheck()	 FALSE
	eglDisplay = eglGetDisplay((EGLNativeDisplayType) EGL_DEFAULT_DISPLAY);
	
	// Initialise EGL
	EGLint maj, min;
	if (!eglInitialize(eglDisplay, &maj, &min))
	{
		fprintf( stderr, "EGL Error: eglInitialize failed\n");
		return FALSE;
	}
	
	EGLint pi32ConfigAttribs[]  = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT , EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8, EGL_NONE };
	EGLint pi32ContextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 1 , EGL_NONE };

	int num_config;
	
	EGLConfig config;
	if (!eglChooseConfig(eglDisplay, pi32ConfigAttribs, &config, 1, &num_config) || (num_config != 1))
	{
		fprintf(stderr, "EGL Error: eglChooseConfig failed\n");
		return FALSE;
	}
	
	eglSurface = eglCreateWindowSurface(eglDisplay, config, NULL, NULL);
	
	if (eglCheck())
	return FALSE;
	
	eglBindAPI(EGL_OPENGL_ES_API);
	if (eglCheck())
	return FALSE;
	
	eglContext = eglCreateContext(eglDisplay, config, NULL, pi32ContextAttribs);
	
	if (eglCheck())
	return FALSE;

	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	
	if (eglCheck())
	return FALSE;
	
	EGLint w,h;
	eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &w);
	eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &h);
	
	// now, setup viewport to adapt to the surface
	//*TODO* adjust if surface is smaller
	int dx = (w-WINWIDTH)/2;
	int dy = (h-WINHEIGHT)/2;
	glViewport(dx, dy, WINWIDTH, WINHEIGHT);
#endif
/*
#ifdef __amigaos4__
  if( screenbodge )
  {
    struct IntuitionBase *ibase = (struct IntuitionBase *)IntuitionBase;
    win = ibase->ActiveWindow;
    if( !win )
      screenbodge = FALSE;
  }
#endif
*/
  // initialize sdl mixer, open up the audio device
  if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
  {
    fprintf( stderr, "SDL_Mixer says: %s\n", Mix_GetError() );
  } else {
    audioavailable = TRUE;
  }

  loadsounds();

  if( !render_init() ) return FALSE;

  titlestate = 0;
  what_are_we_doing = WAWD_TITLES;

  return TRUE;
}

void shut( void )
{
  freesounds();
  if( audioavailable ) Mix_CloseAudio();
  if( joy ) SDL_JoystickClose( joy );
#ifdef HAVE_GLES
	eglMakeCurrent( eglDisplay, NULL, NULL, EGL_NO_CONTEXT );
	if (eglContext)
		eglDestroyContext(eglDisplay, eglContext);        
	if (eglSurface)
		eglDestroySurface(eglDisplay, eglSurface);
	if (eglDisplay)
		eglTerminate(eglDisplay);
	eglContext = NULL;
	eglSurface = NULL;
	eglDisplay = NULL;
#endif
  if( needquit ) SDL_Quit();
}

int main( int argc, char *argv[] )
{
#ifdef __APPLE__
  char lofasz[1024];
  getwd(lofasz);
  printf("%s\n", lofasz);
  chdir(query_resource_directory());
  getwd(lofasz);
  printf("%s\n", lofasz);
#elif defined __HAIKU__
  chdir(dirname(argv[0]));
#endif

#ifdef __PSP__
    psp_init( );
#endif

  if( init() )
  {
    BOOL done, needrender, jletgoh, jletgov;

    paused = FALSE;
		jletgoh = FALSE;
		jletgov = FALSE;

    SDL_AddTimer( FPSTIME, timing, 0 );

    done = FALSE;
    needrender = FALSE;
    while( !done )
    {
      SDL_Event event;
      
      SDL_WaitEvent( &event );
      do {
        switch( event.type )
        {
					case SDL_JOYAXISMOTION:
						switch( what_are_we_doing )
  					{
						  case WAWD_GAME:
								switch( event.jaxis.axis )
							  {
								  case 0:
										if( event.jaxis.value < -3200 )
								  	{
										  jleft = TRUE;
											jright = FALSE;
										} else if( event.jaxis.value > 3200 ) {
											jleft = FALSE;
											jright = TRUE;
										} else {
											jleft = FALSE;
											jright = FALSE;
										}
										break;
								}

								gleft = pleft || jleft;
								gright = pright || jright;
								break;

              case WAWD_MENU:
								switch( event.jaxis.axis )
							  {
								  case 0:
										if( event.jaxis.value < -3200 )
									  {
										  if( !jletgoh ) menu_left();
											jletgoh = TRUE;
										} else if( event.jaxis.value > 3200 ) {
											if( !jletgoh ) menu_right();
                      jletgoh = TRUE;
										} else {
											jletgoh = FALSE;
										}
										break;
									
									case 1:
										if( event.jaxis.value < -3200 )
								  	{
										  if( !jletgov ) menu_up();
											jletgov = TRUE;
										} else if( event.jaxis.value > 3200 ) {
											if( !jletgov ) menu_down();
											jletgov = TRUE;
										} else {
											jletgov = FALSE;
										}
										break;
								}
								break;

	   				}
						break;
					
					case SDL_JOYBUTTONDOWN:
						switch( what_are_we_doing )
					  {
						  case WAWD_GAME:
								switch( event.jbutton.button )
								{
									case 0:
										jjump = TRUE;
										gjump = TRUE;
										break;									
								}
								break;
						}
						break;
					
					case SDL_JOYBUTTONUP:
						switch( what_are_we_doing )
					  {
						  case WAWD_GAME:
								switch( event.jbutton.button )
							  {
								  case 0:
										jjump = FALSE;
									  gjump = pjump;
										break;

									case 1:
										spacehit = TRUE;
										break;
									
									case 2:
										enterhit = TRUE;
										break;
								}
								break;
							
							case WAWD_TITLES:
								go_menus();
								break;

							case WAWD_MENU:
								done |= menu_do();
								break;

						  case WAWD_ENDING:
								if( endingstate < 9 )
								  endingstate = 11;
								break;
						}
						break;


					case SDL_MOUSEMOTION:
            mousx = event.motion.x/2;
            mousy = event.motion.y/2;
            break;

          case SDL_KEYDOWN:
            switch( what_are_we_doing )
            {
              case WAWD_TITLES:
                break;
              
              case WAWD_MENU:
                break;
              
              case WAWD_GAME:
                if( ( event.key.keysym.sym == keyleft ) ||
                    ( event.key.keysym.sym == keyleft2 ) )
                {
                  pleft = TRUE;
									gleft = TRUE;
                  break;
                }

                if( ( event.key.keysym.sym == keyright ) ||
                    ( event.key.keysym.sym == keyright2 ) )
                {
                  pright = TRUE;
									gright = TRUE;
                  break;
                }

                if( ( event.key.keysym.sym == keyjump ) ||
                    ( event.key.keysym.sym == keyjump2 ) )
                {
                  pjump = TRUE;
									gjump = TRUE;
                  break;
                }

                if( ( event.key.keysym.sym == keyuse ) ||
                    ( event.key.keysym.sym == keyuse2 ) )
                {
                  spacehit = TRUE;
                  break;
                }

                if( ( event.key.keysym.sym == keyhint ) ||
                    ( event.key.keysym.sym == keyhint2 ) )
                {
                  enterhit = TRUE;
                  break;
                }

                switch( event.key.keysym.sym )
                {
/*
									case 's':
                  case 'S':
                    {
                      FILE *f;
                      f = fopen( "deleteme.tmp", "wb" );
                      if( !f ) break;
                      fprintf( f, "%d %d", gid.x, gid.y );
                      fclose( f );
                    }
                    break;

                  case 'l':
                  case 'L':
                    {
                      FILE *f;
                      int x, y;
                      f = fopen( "deleteme.tmp", "rb" );
                      if( !f ) break;
                      if( fscanf( f, "%d %d", &x, &y ) == 2 )
                      {
                        gid.x = x;
                        gid.px = x>>8;
                        gid.y = y;
                        gid.py = y>>8;
                      }
                      fclose( f );
                      fgx = gid.px-160;
                      fgy = gid.py-140;
                    }
                    break;
*/

                  case SDLK_F11:
                  case SDLK_DELETE:   // Amiga keyboards have no F11, so also use DEL
                    if( musicon )
                    {
                      musicon = FALSE;
                      if( moozak ) Mix_FadeOutMusic( 500 );
                    } else {
                      musicon = TRUE;
                      if( moozak ) Mix_FadeInMusic( moozak, -1, 500 );
                    }
                    break;
                  
                  case SDLK_F12:
                  case SDLK_HELP:  // Amiga keyboards have no F12, so also use HELP
                    paused = !paused;
                    if( paused )
                      giddy_say( "Paused" );
                    break;
                  
                  default:
                    break;
                }
                break;
            }
            break;
          
          case SDL_KEYUP:
            switch( what_are_we_doing )
            {
						  case WAWD_ENDING:
								if( endingstate < 9 )
								  endingstate = 11;
								break;

							case WAWD_TITLES:
                go_menus();
                break;
              
              case WAWD_DEFINE_A_KEY:
                define_a_key( event.key.keysym.sym );
                break;

              case WAWD_MENU:
                switch( event.key.keysym.sym )
                {
                  case SDLK_UP:
                    menu_up();
                    break;
                  
                  case SDLK_DOWN:
                    menu_down();
                    break;
                  
                  case SDLK_LEFT:
                    menu_left();
                    break;
                  
                  case SDLK_RIGHT:
                    menu_right();
                    break;
                  
                  case SDLK_RETURN:
                  case SDLK_SPACE:
                    done |= menu_do();
                    break;
                  
                  case SDLK_ESCAPE:
                    done = TRUE;
                    break;
                  
                  default:
                    if( ( event.key.keysym.sym == keyjump ) || ( event.key.keysym.sym == keyjump2 ) ||
                        ( event.key.keysym.sym == keyhint ) || ( event.key.keysym.sym == keyhint2 ) ||
                        ( event.key.keysym.sym == keyuse  ) || ( event.key.keysym.sym == keyuse2  ) )
                    {
                      done |= menu_do();
                      break;
                    }
                    break;
                }
                break;
              
              case WAWD_GAME:
                if( ( event.key.keysym.sym == keyleft ) ||
                    ( event.key.keysym.sym == keyleft2 ) )
                {
                  pleft = FALSE;
									gleft = jleft;
                  break;
                }

                if( ( event.key.keysym.sym == keyright ) ||
                    ( event.key.keysym.sym == keyright2 ) )
                {
                  pright = FALSE;
									gright = jright;
                  break;
                }

                if( ( event.key.keysym.sym == keyjump ) ||
                    ( event.key.keysym.sym == keyjump2 ) )
                {
                  pjump = FALSE;
									gjump = jjump;
                  break;
                }

                switch( event.key.keysym.sym )
                {
                  case SDLK_ESCAPE:
                    titlestate = 0;
                    what_are_we_doing = WAWD_TITLES;
                    break;
                  
                  default:
                    break;
                }
                break;
            }
            break;

          case SDL_QUIT:
            done = TRUE;
            break;

          case SDL_USEREVENT:
            needrender = TRUE;
            break;
        }
      } while( SDL_PollEvent( &event ) );

      if( needrender )
      {
        needrender = FALSE;
        switch( what_are_we_doing )
        {
          case WAWD_TITLES:
          case WAWD_MENU:
          case WAWD_DEFINE_A_KEY:
            done |= render_titles();
            break;

          case WAWD_GAME:
            done |= render();
            break;
				
				  case WAWD_ENDING:
						done |= render_ending();
					  break;
        }
      }
    }
  }
  shut();
  return 0;
}


#ifdef __APPLE__
char *query_resource_directory( void )
{
	static char *buffer = NULL;
	
	if (buffer != NULL)
	{
		return buffer;
	}
	
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	
	int bufferSize;
	

	CFURLRef resourceURL = CFBundleCopyBundleURL(mainBundle);
	CFStringRef fullPathString = CFURLCopyFileSystemPath(resourceURL, kCFURLPOSIXPathStyle);
	int fullPathLength = CFStringGetLength(fullPathString);

	bufferSize = fullPathLength * 3 + 1;
	char *bundleBuffer = (char *)calloc(bufferSize, 1);
	CFStringGetCString(fullPathString, bundleBuffer, bufferSize, kCFStringEncodingUTF8);

	CFRelease(fullPathString);
	CFRelease(resourceURL);


	resourceURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	fullPathString = CFURLCopyFileSystemPath(resourceURL, kCFURLPOSIXPathStyle);
	fullPathLength = CFStringGetLength(fullPathString);

	bufferSize = fullPathLength * 3 + 1;
	char *resourcesBuffer = (char *)calloc(bufferSize, 1);
	CFStringGetCString(fullPathString, resourcesBuffer, bufferSize, kCFStringEncodingUTF8);

	CFRelease(fullPathString);
	CFRelease(resourceURL);


	buffer = calloc(strlen(bundleBuffer) + strlen(resourcesBuffer) + 2, 1);
	strcpy(buffer, bundleBuffer);
	buffer[strlen(buffer)] = '/';
	strcat(buffer, resourcesBuffer);
	
	free(bundleBuffer);
	free(resourcesBuffer);

	return buffer;
}
#endif
