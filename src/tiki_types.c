#include "tiki_types.h"

#include "tiki_memory.h"

TikiStringView tikiStringViewCreate( const char* string, uintsize length )
{
	const TikiStringView result = { string, length };
	return result;
}

TikiStringView tikiStringViewCreateBeginEnd( const char* begin, const char* end )
{
	const TikiStringView result = { begin, end - begin };
	return result;
}

TikiStringView tikiStringViewCreateEmpty()
{
	const TikiStringView result = { NULL, 0 };
	return result;
}

TikiStringView tikiStringViewCreateFromPointer( const char* string )
{
	const TikiStringView result = { string, string ? strlen( string ) : 0 };
	return result;
}

TikiStringView tikiStringViewAllocateCopy( TikiAllocator* allocator, TikiStringView string )
{
	return tikiStringViewAllocateCopyPointerLength( allocator, string.data, string.length );
}

TikiStringView tikiStringViewAllocateCopyPointer( TikiAllocator* allocator, const char* string )
{
	return tikiStringViewAllocateCopyPointerLength( allocator, string, strlen( string ) );
}

TikiStringView tikiStringViewAllocateCopyPointerLength( TikiAllocator* allocator, const char* string, uintsize length )
{
	char* newString = tikiMemoryAlloc( allocator, length + 1 );
	if( !newString )
	{
		return tikiStringViewCreateEmpty();
	}

	memcpy( newString, string, length );
	newString[ length ] = '\0';

	return tikiStringViewCreate( newString, length );
}

bool tikiStringViewIsEquals( TikiStringView lhs, TikiStringView rhs )
{
	if( lhs.length != rhs.length )
	{
		return false;
	}
	else if( !lhs.data || !rhs.data )
	{
		return lhs.length == 0;
	}

	return strncmp( lhs.data, rhs.data, lhs.length ) == 0;
}

bool tikiStringViewIsEqualsStr( TikiStringView lhs, const char* rhs )
{
	if( !lhs.data || !rhs )
	{
		return lhs.length == 0;
	}

	return strncmp( lhs.data, rhs, lhs.length ) == 0;
}
