
#include <stdlib.h>
#include <stdio.h>

unsigned char buf[32*32*3];

int main( void )
{
  FILE *f;
  int x, y;

  f = fopen( "tva.raw", "rb" );
	if( f )
	{
		fread( &buf[0], 32*32*3, 1, f );
		fclose( f );
	}

	f = fopen( "tva.c", "w" );
	if( f )
	{
		fprintf( f, "Uint8 tvborders[] = { " );
		for( y=0; y<32; y++ )
		{
			for( x=0; x<32; x++ )
				fprintf( f, "0,0,0,0x%02X%s", buf[(y*32+x)*3]&0xff, ((x==31)&(y==31))?"":"," );
			if( y != 31 ) fprintf( f, "\n                      " ); else fprintf( f, "};\n\n" );
		}
  	fclose( f );
	}
	return 0;
}
