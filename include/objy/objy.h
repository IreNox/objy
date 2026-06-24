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

// Build in Types
//struct Folder
//{
//};
//
//enum TypeKind
//{
//	Id,
//	Bool,
//	Integer,
//	Float,
//	String,
//	Blob,
//	Struct,
//	Array,
//	Reference,
//	DateTime
//};
//
//struct Type
//{
//	TypeKind			kind;
//	Reference< Type >	base;
//	uint8				valueBitCount;
//	bool				intSigned;
//	uint32				version;
//};
//
//struct TypeField
//{
//	Reference< Type >	type;
//};

typedef struct ObjyId
{
	uint32_t			data1;
	uint16_t			data2;
	uint16_t			data3;
	uint8_t				data4[ 8 ];
} ObjyId;

static const ObjyId ObjyIdInvalid		= { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
static const ObjyId ObjyIdTypesFolder	= { 0x0eb7621b, 0x2fb8, 0x3ad5, { 0x91, 0x04, 0xcd, 0x36, 0x0b, 0x1e, 0x4f, 0x5f } }; // UUID v3: OID/ObjyTypes
static const ObjyId ObjyIdType			= { 0xfcaa256d, 0x6ec6, 0x3c2d, { 0xaa, 0x75, 0x39, 0xb7, 0x7e, 0xa6, 0xee, 0x4b } }; // UUID v3: ObjyIdTypesFolder/ObjyType
static const ObjyId ObjyIdTypeKind		= { 0x860950f1, 0xdb49, 0x3bca, { 0xa6, 0xcb, 0x70, 0x25, 0x61, 0x18, 0x3b, 0x94 } }; // UUID v3: ObjyIdTypesFolder/ObjyTypeKind
static const ObjyId ObjyIdTypeField		= { 0x4c7276f7, 0xaba2, 0x30da, { 0x88, 0xac, 0x7c, 0x8f, 0xe5, 0x43, 0x8e, 0xf9 } }; // UUID v3: ObjyIdTypesFolder/ObjyTypeField
static const ObjyId ObjyIdFolder		= { 0x9fc6a50b, 0x2d8b, 0x3811, { 0xa1, 0x01, 0x2e, 0xa1, 0xd9, 0x74, 0x47, 0x5a } }; // UUID v3: ObjyIdTypesFolder/ObjyFolder

#define OBJY_ID_FORMAT_STRING "%08x-%04x-%04x-%02x%02-x%02x%02x%02x%02x%02x%02x"
#define OBJY_ID_FORMAT_DATA( var ) var.data1, var.data2, var.data3, var.data4[ 0 ], var.data4[ 1 ], var.data4[ 2 ], var.data4[ 3 ], var.data4[ 4 ], var.data4[ 5 ], var.data4[ 6 ], var.data4[ 7 ]

bool objyIdIsValid( ObjyId id );
bool objyIdIsEqual( ObjyId lhs, ObjyId rhs );
void objyIdCreate( ObjyId* id );

typedef struct ObjyBlob
{
	const void*			data;
	size_t				size;
} ObjyBlob;

typedef struct ObjyDateTime
{
	int16_t				year;
	uint8_t				month;
	uint8_t				day;
	uint8_t				hour;
	uint8_t				minute;
	uint8_t				second;
	int8_t				timeZone;
	uint16_t			millisecond;
} ObjyDateTime;

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

typedef void(*ObjyTypeNewObjectFunc)( const ObjyType* type, const ObjyObject* object, void* userData );
typedef void(*ObjyTypeObjectDeleteFunc)( const ObjyType* type, const ObjyObject* object, void* userData );
typedef void(*ObjyTypeObjectModifyFunc)( const ObjyType* type, const ObjyObject* object, const char* path, const ObjyValue* oldValue, const ObjyValue* newValue, void* userData );

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
	ObjyTypeKind_Enum,
	ObjyTypeKind_String,
	ObjyTypeKind_Blob,
	ObjyTypeKind_Struct,
	ObjyTypeKind_Array,
	ObjyTypeKind_Reference,
	ObjyTypeKind_DateTime
} ObjyTypeKind;

typedef struct ObjyTypeField
{
	const char*			name;
	const ObjyType*		type;
} ObjyTypeField;

ObjyType*				objyTypeFind( ObjySystem* system, const char* name );

void					objyTypeSetCallbacks( ObjySystem* system, ObjyType* type, ObjyTypeNewObjectFunc* newObjectCallback, ObjyTypeObjectDeleteFunc* objectDeleteCallback, ObjyTypeObjectModifyFunc* objectModifyCallback, void* userData );
void					objyTypeSetUserData( ObjySystem* system, ObjyType* type, void* userData );

ObjyId					objyTypeGetId( const ObjyType* type );
const char*				objyTypeGetName( const ObjyType* type );
ObjyTypeKind			objyTypeGetKind( const ObjyType* type );
const ObjyType*			objyTypeGetBaseType( const ObjyType* type );
size_t					objyTypeGetValueBitCount( const ObjyType* type );
bool					objyTypeGetSignedInt( const ObjyType* type );
void*					objyTypeGetUserData( const ObjyType* type );
const ObjyTypeField*	objyTypeGetStructLocalFields( const ObjyType* type );
size_t					objyTypeGetStructLocalFieldCount( const ObjyType* type );
const ObjyTypeField*	objyTypeGetStructGlobalFields( const ObjyType* type );
size_t					objyTypeGetStructGlobalFieldCount( const ObjyType* type );

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

ObjyValue*				objyValueCreate( ObjyContext* context, const ObjyType* type );
ObjyValue*				objyValueCreateId( ObjyContext* context, ObjyId newValue );
ObjyValue*				objyValueCreateBool( ObjyContext* context, bool newValue );
ObjyValue*				objyValueCreateUInt( ObjyContext* context, uint64_t newValue );
ObjyValue*				objyValueCreateSInt( ObjyContext* context, int64_t newValue );
ObjyValue*				objyValueCreateFloat( ObjyContext* context, double newValue );
ObjyValue*				objyValueCreateString( ObjyContext* context, const char* newValue );
ObjyValue*				objyValueCreateStringLength( ObjyContext* context, const char* newValue, size_t stringLength );
ObjyValue*				objyValueCreateStruct( ObjyContext* context );
ObjyValue*				objyValueCreateArray( ObjyContext* context );
ObjyValue*				objyValueCreateReference( ObjyContext* context, const ObjyType* refType, ObjyValue* refValue );
ObjyValue*				objyValueCreateBlob( ObjyContext* context, const void* data, size_t dataSize );
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
//const ObjyType*			objyValueReadReferenceType( const ObjyValue* value );
const void*				objyValueReadBlob( const ObjyValue* value );
size_t					objyValueReadBlobSize( const ObjyValue* value );

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
bool					objyValueWriteReference( ObjyContext* context, ObjyValue* value, ObjyId typeId, ObjyValue* newValue );
bool					objyValueWriteBlob( ObjyContext* context, ObjyValue* value, const void* data, size_t dataSize );

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

ObjyObject*				objyChangeSetObjectCreate( ObjyChangeSet* changeSet, ObjyId id, const char* name, const ObjyType* structType, ObjyId parentId );
ObjyObject*				objyChangeSetObjectCreateValue( ObjyChangeSet* changeSet, ObjyId id, const char* name, size_t nameLength, const ObjyType* structType, ObjyId parentId, ObjyValue* initValue );
void					objyChangeSetObjectDelete( ObjyChangeSet* changeSet, ObjyObject* object );
void					objyChangeSetObjectDeleteId( ObjyChangeSet* changeSet, ObjyId objectId );
void					objyChangeSetObjectMove( ObjyChangeSet* changeSet, ObjyObject* object, ObjyObject* newParent );

void					objyChangeSetWriteId( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, ObjyId value );
void					objyChangeSetWriteBool( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, bool value );
void					objyChangeSetWriteUInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, uint64_t value );
void					objyChangeSetWriteSInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, int64_t value );
void					objyChangeSetWriteFloat( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, double value );
void					objyChangeSetWriteString( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const char* value );
void					objyChangeSetWriteStringLength( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const char* value, size_t stringLength );
void					objyChangeSetWriteArray( ObjyChangeSet* changeSet, ObjyObject* object, const char* path );
void					objyChangeSetWriteReference( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const ObjyValue* value, const ObjyType* type );
void					objyChangeSetWriteBlob( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const void* data, size_t dataSize );

const ObjyType*			objyChangeSetTypeCreateValue( ObjyChangeSet* changeSet, const char* name, ObjyTypeKind kind, size_t bitCount, bool signedInt, void* userData );	// bitCount: only for ObjyTypeKind_Integer and ObjyTypeKind_Float, signedInt: only for ObjyTypeKind_Integer
const ObjyType*			objyChangeSetTypeCreateStruct( ObjyChangeSet* changeSet, const char* name, const ObjyTypeField* fields, size_t fieldCount, void* userData );
const ObjyType*			objyChangeSetTypeCreateArray( ObjyChangeSet* changeSet, const ObjyType* baseType, void* userData );
const ObjyType*			objyChangeSetTypeCreateReference( ObjyChangeSet* changeSet, const ObjyType* baseType, void* userData );
const ObjyType*			objyChangeSetTypeCreateBlob( ObjyChangeSet* changeSet, void* userData );
void					objyChangeSetTypeRemove( ObjyChangeSet* changeSet, const ObjyType* type );

#ifdef __cplusplus
}
#endif
