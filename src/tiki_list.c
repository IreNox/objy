#include "tiki_list.h"

static TikiListNode*	tikiListGetNode( TikiList* list, void* item );

void tikiListConstruct( TikiList* list, uintsize nodeOffset )
{
	list->nodeOffset	= nodeOffset;
	list->firstItem		= NULL;
	list->lastItem		= NULL;
	list->count			= 0;
}

void tikiListDestruct( TikiList* list )
{
	while( list->firstItem )
	{
		tikiListPopFront( list );
	}

	list->nodeOffset = 0;
}

void* tikiListGetFront( TikiList* list )
{
	return list->firstItem;
}

void* tikiListGetBack( TikiList* list )
{
	return list->lastItem;
}

void tikiListPushFront( TikiList* list, void* item )
{
	TikiListNode* node = tikiListGetNode( list, item );
	if( !list->lastItem )
	{
		list->lastItem = item;
	}
	else
	{
		TikiListNode* firstNode = tikiListGetNode( list, list->firstItem );
		firstNode->prevItem = item;
	}

	node->nextItem = list->firstItem;
	list->firstItem = item;
}

void tikiListPushBack( TikiList* list, void* item )
{
	TikiListNode* node = tikiListGetNode( list, item );
	if( !list->firstItem )
	{
		list->firstItem = item;
	}
	else
	{
		TikiListNode* lastNode = tikiListGetNode( list, list->lastItem );
		lastNode->nextItem = item;
	}

	node->prevItem = list->lastItem;
	list->lastItem = item;
}

void* tikiListPopFront( TikiList* list )
{
	void* front = list->firstItem;
	tikiListRemove( list, front );
	return front;
}

void* tikiListPopBack( TikiList* list )
{
	void* back = list->lastItem;
	tikiListRemove( list, back );
	return back;
}

void tikiListRemove( TikiList* list, void* item )
{
	if( !item )
	{
		return;
	}

	TikiListNode* node = tikiListGetNode( list, item );
	if( item == list->firstItem )
	{
		list->firstItem = node->nextItem;
	}

	if( item == list->lastItem )
	{
		list->lastItem = node->prevItem;
	}

	if( node->nextItem )
	{
		node->nextItem->prevItem = node->prevItem;
	}

	if( node->prevItem )
	{
		node->prevItem->nextItem = node->nextItem;
	}
}

static TikiListNode* tikiListGetNode( TikiList* list, void* item )
{
	byte* itemBytes = (byte*)item;
	return (TikiListNode*)itemBytes + list->nodeOffset;
}