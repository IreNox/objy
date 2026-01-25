#pragma once

#include "tiki_types.h"

#ifdef TIKI_DEBUG
#	define TIKI_DEBUG

#	define TIKI_DEBUG_INFO( fmt, ... )		tikiDebugTrace( "Info: " fmt "\n", __VA_ARGS__ )
#	define TIKI_DEBUG_WARNING( fmt, ... )	tikiDebugTrace( "Warning: " fmt "\n", __VA_ARGS__ )
#	define TIKI_DEBUG_ERROR( fmt, ... )		tikiDebugTrace( "Error: " fmt "\n", __VA_ARGS__ )

void	tikiDebugTrace( const char* format, ... );
#else
#	define TIKI_DEBUG_INFO( fmt, ... )
#	define TIKI_DEBUG_WARNING( fmt, ... )
#	define TIKI_DEBUG_ERROR( fmt, ... )
#endif