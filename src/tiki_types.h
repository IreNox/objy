#pragma once

#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if (defined( _DEBUG ) || defined( DEBUG )) && !defined( NDEBUG )
#	define TIKI_DEBUG
#endif

#ifdef TIKI_DEBUG
#	include <assert.h>
#endif

#ifdef _MSC_VER
#	include <intrin.h>
#endif

typedef uint8_t		uint8;
typedef uint16_t	uint16;
typedef uint32_t	uint32;
typedef uint64_t	uint64;

typedef int8_t		sint8;
typedef int16_t		sint16;
typedef int32_t		sint32;
typedef int64_t		sint64;

typedef size_t		uintsize;
typedef ptrdiff_t 	sintsize;

typedef uint8_t		byte;

typedef void* (*TikiAllocatorMallocFunc)( uintsize size, void* userData );
typedef void* (*TikiAllocatorReallocFunc)( void* memory, uintsize oldSize, uintsize newSize, void* userData );
typedef void (*TikiAllocatorFreeFunc)( void* memory, void* userData );

typedef struct TikiAllocator
{
	TikiAllocatorMallocFunc		mallocFunc;
	TikiAllocatorReallocFunc	reallocFunc;
	TikiAllocatorFreeFunc		freeFunc;
	void*						userData;
	void*						internalData;
} TikiAllocator;

typedef struct TikiStringView
{
	const char*		data;
	uintsize		length;
} TikiStringView;

#define TIKI_STRING_VIEW_DATA( str ) str.length, str.data

TikiStringView	tikiStringViewCreate( const char* string, uintsize length );
TikiStringView	tikiStringViewCreateBeginEnd( const char* begin, const char* end );
TikiStringView	tikiStringViewCreateEmpty();
TikiStringView	tikiStringViewCreateFromPointer( const char* string );

TikiStringView	tikiStringViewAllocateCopy( TikiAllocator* allocator, TikiStringView string );
TikiStringView	tikiStringViewAllocateCopyPointer( TikiAllocator* allocator, const char* string );
TikiStringView	tikiStringViewAllocateCopyPointerLength( TikiAllocator* allocator, const char* string, uintsize length );

bool			tikiStringViewIsEquals( TikiStringView lhs, TikiStringView rhs );
bool			tikiStringViewIsEqualsStr( TikiStringView lhs, const char* rhs );

#ifdef TIKI_DEBUG
#	define TIKI_ASSERT( exp ) assert( exp )
#	define TIKI_VERIFY( exp ) assert( exp )
#else
#	define TIKI_ASSERT( exp )
#	define TIKI_VERIFY( exp ) exp
#endif

#define TIKI_STATIC_MIN( a, b ) ((a) < (b) ? (a) : (b))
#define TIKI_STATIC_MAX( a, b ) ((a) > (b) ? (a) : (b))

#define TIKI_ARRAY_COUNT( arr ) (sizeof( arr ) / sizeof( *(arr) ))

#if defined( __GNUC__ ) || defined( __clang__ )
#	define TIKI_OFFSETOF( type, member )		__builtin_offsetof( type, member )
#	define TIKI_COUNT_LEADING_ZEROS32( val )	__builtin_clz( val )
#	define TIKI_COUNT_LEADING_ZEROS64( val )	__builtin_clzl( val )
#else
#	define TIKI_OFFSETOF( type, member )		((uintsize)(&((type*)0)->member))
#	define TIKI_COUNT_LEADING_ZEROS32( val )	__lzcnt( val )
#	define TIKI_COUNT_LEADING_ZEROS64( val )	__lzcnt64( val )
#endif

#if defined(_M_X64) || defined(__amd64__)
#	define TIKI_NEXT_POWER_OF_TWO( val )	(1ll << (64 - TIKI_COUNT_LEADING_ZEROS64( val -1 )))
#else
#	define TIKI_NEXT_POWER_OF_TWO( val )	(1 << (32 - TIKI_COUNT_LEADING_ZEROS32( val -1 )))
#endif
