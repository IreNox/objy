#pragma once

#include "tiki_types.h"

typedef struct TikiListNode
{
	struct TikiListNode*	prevItem;
	struct TikiListNode*	nextItem;
} TikiListNode;

typedef struct TikiList
{
	uintsize		nodeOffset;

	TikiListNode*	firstItem;
	TikiListNode*	lastItem;

	uintsize		count;
} TikiList;

void						tikiListConstruct( TikiList* list, uintsize nodeOffset );
void						tikiListDestruct( TikiList* list );

void*						tikiListGetFront( TikiList* list );
void*						tikiListGetBack( TikiList* list );

void						tikiListPushFront( TikiList* list, void* item );
void						tikiListPushBack( TikiList* list, void* item );
void*						tikiListPopFront( TikiList* list );
void*						tikiListPopBack( TikiList* list );

void						tikiListRemove( TikiList* list, void* item );
