#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ObjySystem ObjySystem;
typedef struct ObjyMutex ObjyMutex;
typedef struct ObjyContext ObjyContext;
typedef struct ObjyType ObjyType;
typedef struct ObjyObject ObjyObject;
typedef struct ObjyValue ObjyValue;
typedef struct ObjyChangeSet ObjyChangeSet;

typedef struct ObjyId
{
	uint32_t			data1;
	uint16_t			data2;
	uint16_t			data3;
	uint8_t				data4[ 8 ];
} ObjyId;

static const ObjyId ObjyIdInvalid = { 0, 0, 0, { 0, 0, 0, 0 } };

#define OBJY_ID_FORMAT_STRING "%08x-%04x-%04x-%02x%02-x%02x%02x%02x%02x%02x%02x"
#define OBJY_ID_FORMAT_DATA( var ) var.data1, var.data2, var.data3, var.data4[ 0 ], var.data4[ 1 ], var.data4[ 2 ], var.data4[ 3 ], var.data4[ 4 ], var.data4[ 5 ], var.data4[ 6 ], var.data4[ 7 ]

bool objyIdIsValid( ObjyId id );

typedef struct ObjyBlob
{
	const void*			data;
	size_t				size;
} ObjyBlob;

typedef void* (*ObjyAllocatorMallocFunc)( size_t size, void* userData );
typedef void* (*ObjyAllocatorReallocFunc)( void* memory, size_t oldSize, size_t newSize, void* userData );
typedef void (*ObjyAllocatorFreeFunc)( void* memory, void* userData );

typedef struct ObjyAllocator
{
	ObjyAllocatorMallocFunc		mallocFunc;		// set to NULL to use default malloc/free
	ObjyAllocatorReallocFunc	reallocFunc;	// can be NULL
	ObjyAllocatorFreeFunc		freeFunc;
	void*						userData;
} ObjyAllocator;

//typedef ObjyMutex* (*ObjyMutexCreateFunc)();
//typedef void (*ObjyMutexDestryFunc)( ObjyMutex* mutex );
//typedef void (*ObjyMutexLock)( ObjyMutex* mutex );
//typedef void (*ObjyMutexUnlock)( ObjyMutex* mutex );
//
//typedef struct ObjyMutexDefinition
//{
//	ObjyMutexCreateFunc	createFunc;
//	ObjyMutexDestryFunc	destroyFunc;
//	ObjyMutexLock		lockFunc;
//	ObjyMutexUnlock		unlockFunc;
//} ObjyMutexDefinition;

typedef ObjyBlob (*ObjyObjectSerializeFunc)( const ObjyObject* object, void* userData );
typedef bool (*ObjyObjectDeserializeFunc)( ObjyObject* object, ObjyValue* objectRootValue, ObjyBlob data, void* userData );
typedef ObjyId (*ObjyObjectBlobGetIdFunc)( ObjyBlob data, void* userData );
typedef void (*ObjyObjectBlobFree)( ObjyBlob blob, void* userData );

typedef struct ObjyObjectFormatter
{
	ObjyObjectSerializeFunc		serializeFunc;
	ObjyObjectDeserializeFunc	deserializeFunc;
	ObjyObjectBlobGetIdFunc		blobGetId;
	ObjyObjectBlobFree			freeFunc;
	void*						userData;
} ObjyObjectFormatter;

typedef ObjyBlob (*ObjyChangeSetSerializeFunc)( const ObjyChangeSet* changeSet, void* userData );
typedef bool (*ObjyChangeSetDeserializeFunc)( ObjyChangeSet* changeSet, ObjyBlob data, void* userData );
typedef void (*ObjyChangeSetBlobFree)( ObjyBlob blob, void* userData );

typedef struct ObjyChangeSetFormatter
{
	ObjyChangeSetSerializeFunc		serializeFunc;
	ObjyChangeSetDeserializeFunc	deserializeFunc;
	ObjyChangeSetBlobFree			freeFunc;
	void*							userData;
} ObjyChangeSetFormatter;

typedef struct ObjyParameters					// Fill with zero for default parameters
{
	ObjyAllocator		allocator;				// Override memory Allocator. Default use malloc/free
	//ObjyMutexDefinition	mutex;
} ObjyParameters;

// Base

ObjySystem*				objyCreate( const ObjyParameters* parameters );
void					objyDestroy( ObjySystem* system );

ObjyContext*			objyGetRootContext( ObjySystem* system );

ObjyContext*			objyContextCreate( ObjySystem* system, ObjyObject* parentObject );
void					objyContextDestroy( ObjyContext* context );

// Types

typedef enum ObjyTypeKind
{
	ObjyTypeKind_Id,
	ObjyTypeKind_Bool,
	ObjyTypeKind_Integer,
	ObjyTypeKind_Float,
	ObjyTypeKind_String,
	ObjyTypeKind_Struct,
	ObjyTypeKind_Array,
	ObjyTypeKind_Reference
} ObjyTypeKind;

typedef struct ObjyTypeField
{
	const char*			name;
	const ObjyType*		type;
} ObjyTypeField;

const ObjyType*			objyTypeCreateValue( ObjySystem* system, const char* name, ObjyTypeKind kind, size_t bitCount, bool signedInt, void* userData );	// bitCount: only for ObjyTypeKind_Integer and ObjyTypeKind_Float, signedInt: only for ObjyTypeKind_Integer
const ObjyType*			objyTypeCreateStruct( ObjySystem* system, const char* name, const ObjyTypeField* fields, size_t fieldCount, void* userData );
const ObjyType*			objyTypeCreateStructInherited( ObjySystem* system, const char* name, const ObjyType* baseType, const ObjyTypeField* fields, size_t fieldCount, void* userData );
const ObjyType*			objyTypeCreateArray( ObjySystem* system, const ObjyType* baseType, void* userData );
const ObjyType*			objyTypeCreateReference( ObjySystem* system, const ObjyType* baseType, void* userData );
void					objyTypeDestroy( ObjySystem* system, const ObjyType* type );

const ObjyType*			objyTypeFind( ObjySystem* system, const char* name );

const char*				objyTypeGetName( const ObjyType* type );
ObjyTypeKind			objyTypeGetKind( const ObjyType* type );
const ObjyType*			objyTypeGetBaseType( const ObjyType* type );
void*					objyTypeGetUserData( const ObjyType* type );
const ObjyTypeField*	objyTypeGetStructLocalFields( const ObjyType* type );
size_t					objyTypeGetStructLocalFieldCount( const ObjyType* type );
const ObjyTypeField*	objyTypeGetStructGlobalFields( const ObjyType* type );
size_t					objyTypeGetStructGlobalFieldCount( const ObjyType* type );
size_t					objyTypeGetValueBitCount( const ObjyType* type );

// Object

ObjyObject*				objyObjectCreateDetached( ObjyContext* context, ObjyId id, const char* name, const ObjyType* structType );
void					objyObjectDestroy( ObjyContext* context, ObjyObject* object );

ObjyBlob				objyObjectSerialize( ObjyContext* context, const ObjyObject* object, ObjyObjectFormatter* formatter );
bool					objyObjectDeserialize( ObjyContext* context, ObjyBlob data, ObjyObjectFormatter* formatter );
ObjyObject*				objyObjectDeserializeDetached( ObjyContext* context, ObjyBlob data, ObjyObjectFormatter* formatter );

const ObjyObject*		objyObjectFind( ObjyContext* context, ObjyId id );

ObjyId					objyObjectGetId( const ObjyObject* object );
const char*				objyObjectGetName( const ObjyObject* object );
const ObjyType*			objyObjectGetType( const ObjyObject* object );
ObjyContext*			objyObjectGetContext( const ObjyObject* object );
const ObjyObject*		objyObjectGetParent( const ObjyObject* object );
const ObjyObject*		objyObjectGetFirstChild( const ObjyObject* object );
const ObjyObject*		objyObjectGetPrevSibling( const ObjyObject* object );
const ObjyObject*		objyObjectGetNextSibling( const ObjyObject* object );
size_t					objyObjectGetChildCount( const ObjyObject* object );
const ObjyObject*		objyObjectFindChildByName( const ObjyObject* object, const char* childName );

ObjyValue*				objyObjectGetValueWritable( ObjyObject* object );
const ObjyValue*		objyObjectGetValue( const ObjyObject* object );
const ObjyValue*		objyObjectFindValue( const ObjyObject* object, const char* path );
const ObjyValue*		objyObjectFindValueLength( const ObjyObject* object, const char* path, size_t pathLength );

// Value

ObjyValue*				objyValueCreateId( ObjyContext* context, ObjyId newValue );
ObjyValue*				objyValueCreateBool( ObjyContext* context, bool newValue );
ObjyValue*				objyValueCreateUInt( ObjyContext* context, uint64_t newValue );
ObjyValue*				objyValueCreateSInt( ObjyContext* context, int64_t newValue );
ObjyValue*				objyValueCreateFloat( ObjyContext* context, double newValue );
ObjyValue*				objyValueCreateString( ObjyContext* context, const char* newValue );
ObjyValue*				objyValueCreateStruct( ObjyContext* context, const ObjyType* structType );
ObjyValue*				objyValueCreateArray( ObjyContext* context, const ObjyType* elementType );
ObjyValue*				objyValueCreateReference( ObjyContext* context, const ObjyType* referenceType );
ObjyValue*				objyValueCreateCopy( ObjyContext* context, const ObjyValue* valueToCopy );

void					objyValueDestroy( ObjyContext* context, ObjyValue* value );

ObjyTypeKind			objyValueGetKind( const ObjyValue* value );

ObjyId					objyValueReadId( const ObjyValue* value );
bool					objyValueReadBool( const ObjyValue* value );
uint64_t				objyValueReadUInt( const ObjyValue* value );
int64_t					objyValueReadSInt( const ObjyValue* value );
double					objyValueReadFloat( const ObjyValue* value );
const char*				objyValueReadString( const ObjyValue* value );
const ObjyValue*		objyValueReadStructField( const ObjyValue* value, const char* fieldName );
const ObjyValue*		objyValueReadStructFieldLength( const ObjyValue* value, const char* fieldName, size_t fieldNameLength );
const ObjyValue*		objyValueReadStructFieldIndex( const ObjyValue* value, size_t index );
const char*				objyValueReadStructFieldIndexName( const ObjyValue* value, size_t index );
size_t					objyValueReadStructFieldCount( const ObjyValue* value );
const ObjyValue*		objyValueReadArray( const ObjyValue* value, size_t index );
size_t					objyValueReadArrayCount( const ObjyValue* value );
const ObjyValue*		objyValueReadReference( const ObjyValue* value );
const ObjyType*			objyValueReadReferenceType( const ObjyValue* value );

bool					objyValueWriteId( ObjyContext* context, ObjyValue* value, ObjyId newValue );
bool					objyValueWriteBool( ObjyContext* context, ObjyValue* value, bool newValue );
bool					objyValueWriteUInt( ObjyContext* context, ObjyValue* value, uint64_t newValue );
bool					objyValueWriteSInt( ObjyContext* context, ObjyValue* value, int64_t newValue );
bool					objyValueWriteFloat( ObjyContext* context, ObjyValue* value, double newValue );
bool					objyValueWriteString( ObjyContext* context, ObjyValue* value, const char* string );
bool					objyValueWriteStringLength( ObjyContext* context, ObjyValue* value, const char* string, size_t stringLength );
bool					objyValueWriteStructField( ObjyContext* context, ObjyValue* value, const char* fieldName, ObjyValue* newValue );
bool					objyValueWriteStructFieldLength( ObjyContext* context, ObjyValue* value, const char* fieldName, size_t fieldNameLength, ObjyValue* newValue );
bool					objyValueWriteArray( ObjyContext* context, ObjyValue* value, size_t index, ObjyValue* newValue );
bool					objyValueWriteReference( ObjyContext* context, ObjyValue* value, ObjyValue* newValue );

// Change

ObjyChangeSet*			objyChangeSetBegin( ObjyContext* context );
ObjyChangeSet*			objyChangeSetBeginDetached( ObjyContext* context );
bool					objyChangeSetMerge( ObjyChangeSet* changeSet, const ObjyChangeSet* sourceChangeSet );
void					objyChangeSetDiscard( ObjyChangeSet* changeSet );
bool					objyChangeSetSubmit( ObjyChangeSet* changeSet );

ObjyBlob				objyChangeSetSerialize( ObjyContext* context, const ObjyChangeSet* changeSet, ObjyChangeSetFormatter* formatter );
ObjyChangeSet*			objyChangeSetDeserialize( ObjyContext* context, ObjyBlob data, ObjyChangeSetFormatter* formatter );
bool					objyChangeSetDeserializeMerge( ObjyChangeSet* changeSet, ObjyBlob data, ObjyChangeSetFormatter* formatter );

bool					objyChangeSetIsValid( const ObjyChangeSet* changeSet );

ObjyObject*				objyChangeSetObjectCreate( ObjyChangeSet* changeSet, ObjyId id, const ObjyType* structType, ObjyId parentId );
ObjyObject*				objyChangeSetObjectCreateValue( ObjyChangeSet* changeSet, ObjyId id, const ObjyType* structType, ObjyId parentId, ObjyValue* initValue );
void					objyChangeSetObjectDelete( ObjyChangeSet* changeSet, ObjyObject* object );
void					objyChangeSetObjectDeleteId( ObjyChangeSet* changeSet, ObjyId objectId );
void					objyChangeSetObjectMove( ObjyChangeSet* changeSet, ObjyObject* object, ObjyObject* newParent );

void					objyChangeSetWriteId( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, ObjyId value );
void					objyChangeSetWriteBool( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, bool value );
void					objyChangeSetWriteUInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, uint64_t value );
void					objyChangeSetWriteSInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, int64_t value );
void					objyChangeSetWriteFloat( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, double value );
void					objyChangeSetWriteString( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const char* value );
void					objyChangeSetWriteArray( ObjyChangeSet* changeSet, ObjyObject* object, const char* path );
void					objyChangeSetWriteOptional( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const ObjyValue* value );
void					objyChangeSetWriteVariant( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const ObjyType* type );

#ifdef __cplusplus
}
#endif
