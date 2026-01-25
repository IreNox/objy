#include "objy/objy.h"

#define TIKI_ARRAY_COUNT( arr ) (sizeof( arr ) / sizeof( *(arr) ))

int main()
{
	ObjyParameters parameters = { 0 };

	ObjySystem* system = objyCreate( &parameters );

	const ObjyType* intType = objyTypeCreateValue( system, "SInt32", ObjyTypeKind_Integer, 32, true, NULL );
	const ObjyType* floatType = objyTypeCreateValue( system, "Float32", ObjyTypeKind_Float, 32, true, NULL );
	const ObjyType* stringType = objyTypeCreateValue( system, "String", ObjyTypeKind_String, 0, false, NULL );

	ObjyTypeField test1Fields[] =
	{
		{ "intTest", intType },
		{ "floatTest", floatType },
		{ "stringTest", stringType }
	};

	const ObjyType* folderType = objyTypeCreateStruct( system, "Folder", NULL, 0, NULL );
	const ObjyType* testType = objyTypeCreateStruct( system, "Test1", test1Fields, TIKI_ARRAY_COUNT( test1Fields ), NULL);

	ObjyContext* rootContext = objyGetRootContext( system );

	{
		ObjyId contextId;
		objyIdCreate( &contextId );

		ObjyObject* contextObject = objyObjectCreateDetached( rootContext, contextId, "Test Context", folderType );

		ObjyContext* context = objyContextCreate( system, contextObject );

		{
			ObjyChangeSet* changeSet = objyChangeSetBegin( context );

			{
				ObjyId testId;
				objyIdCreate( &testId );

				ObjyObject* testObject = objyChangeSetObjectCreate( changeSet, testId, "Test 1", testType, contextId );

				objyChangeSetWriteSInt( changeSet, testObject, "intTest", 42 );
				objyChangeSetWriteString( changeSet, testObject, "stringTest", "everything" );

			}
			objyChangeSetSubmit( changeSet );
		}

		objyContextDestroy( context );

		objyObjectDestroy( rootContext, contextObject );
	}

	objyDestroy( system );
	return 0;
}
