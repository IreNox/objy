#include "tiki_debug.h"

#include <stdarg.h>
#include <stdio.h>

#ifdef WIN32
#	include <DbgHelp.h>
#endif

#ifdef TIKI_DEBUG
void tikiDebugTrace( const char* format, ... )
{
	char buffer[ 1024u ];
	va_list args;
	va_start( args, format );
	vsnprintf( buffer, sizeof( buffer ), format, args );
	va_end( args );

#ifdef WIN32
	OutputDebugStringA( buffer );
#else
	puts( buffer );
#endif
}
#endif