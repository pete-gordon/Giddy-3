
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char outname[1024];
char *inname = NULL;

unsigned char *buf;

int main( int argc, char *argv[] )
{
  int i, x, y, w, h, rw, w4;
	unsigned char lt[4];
	unsigned int col;
	int tile16;
	FILE *f;

    x = 0;
    strcpy( outname, "image.bin" );
	tile16 = 0;

  for( i=1; i<argc; i++ )
	{
		if( argv[i][0] == '-' )
		{
			switch( argv[i][1] )
			{
				case 'o':
					strncpy( outname, &argv[i][2], 1024 );
				  break;
				
				case 't':
					tile16 = 1;
				  break;
				
				default:
          printf( "Unknown option %c\n", argv[i][1] );
				  return EXIT_FAILURE;
			}

			continue;
		}

		inname = argv[i];
	}

	if( !inname )
	{
		printf( "Usage: bmp_de_bmp -ooutname.c input.bmp\n" );
		return EXIT_FAILURE;
	}

	f = fopen( inname, "rb" );
	if( !f )
	{
		printf( "Unable to open '%s'\n", inname );
		return EXIT_FAILURE;
	}

  fseek( f, 18, SEEK_SET );
	fread( &lt[0], 4, 1, f );
	w = (lt[3]<<24)|(lt[2]<<16)|(lt[1]<<8)|lt[0];
	fread( &lt[0], 4, 1, f );
	h = (lt[3]<<24)|(lt[2]<<16)|(lt[1]<<8)|lt[0];

  // Alloc RGBA buffer
	buf = malloc( w*h*4 );
	if( !buf )
	{
		printf( "Out of memory\n" );
		fclose( f );
		return 0;
	}

	fseek( f, 54, SEEK_SET );

  rw = ((x+31)/32)*32;
	w4 = w*4;
	for( y=(h-1); y>=0; y-- )
	{
		for( x=0; x<w4; x+=4 )
		{
			fread( &lt[0], 3, 1, f );
			col = (lt[2]<<16)|(lt[1]<<8)|(lt[0]);
			buf[(y*w4)+x  ] = lt[2];  // R
			buf[(y*w4)+x+1] = lt[1];  // G
			buf[(y*w4)+x+2] = lt[0];  // B
			buf[(y*w4)+x+3] = (col==0xff00ff)?0x00:0xff; // Alpha
		}
		if( rw > w ) fseek( f, (rw-w)*3, SEEK_CUR );
	}
	fclose( f );

  f = fopen( outname, "wb" );
	if( !f )
	{
		printf( "Unable top open '%s'\n", outname );
		free( buf );
		return 0;
  }

  if( tile16 )
	{
		int ty;
		for( y=0; y<h; y+=16 )
			for( x=0; x<w4; x+=16*4 )
				for( ty=0; ty<(16*w4); ty+=w4 )
					fwrite( &buf[y*w4+ty+x], 16*4, 1, f );
	} else {
		fwrite( buf, w4*h, 1, f );
	}

	fclose( f );

  free( buf );

  return 0;  
}
