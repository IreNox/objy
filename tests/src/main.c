#include "objy/objy.h"

#include <assert.h>

#define TIKI_ARRAY_COUNT( arr ) (sizeof( arr ) / sizeof( *(arr) ))

int main()
{
	ObjyParameters parameters = { 0 };

	ObjySystem* system = objyCreate( &parameters );

	const ObjyType* intType = objyTypeFind( system, "SInt32" );
	const ObjyType* floatType = objyTypeFind( system, "Float" );
	const ObjyType* stringType = objyTypeFind( system, "String" );

	ObjyTypeField test1Fields[] =
	{
		{ "intTest", intType },
		{ "floatTest", floatType },
		{ "stringTest", stringType }
	};

	const ObjyType* folderType = objyTypeFind( system, "Folder" );

	ObjyContext* rootContext = objyGetRootContext( system );

	{
		ObjyId contextId;
		objyIdCreate( &contextId );

		ObjyObject* contextObject = objyObjectCreateDetached( rootContext, contextId, "Test Context", folderType );

		const ObjyType* testType;
		{
			ObjyChangeSet* changeSet = objyChangeSetBegin( rootContext );

			testType = objyChangeSetTypeCreateStruct( changeSet, "Test1", test1Fields, TIKI_ARRAY_COUNT( test1Fields ), NULL);

			objyChangeSetSubmit( changeSet );
		}

		ObjyContext* context = objyContextCreate( system, contextObject );

		ObjyId testId;
		{
			ObjyChangeSet* changeSet = objyChangeSetBegin( context );

			{
				objyIdCreate( &testId );

				ObjyObject* testObject = objyChangeSetObjectCreate( changeSet, testId, "Test 1", testType, contextId );

				objyChangeSetWriteSInt( changeSet, testObject, "intTest", 42 );
				objyChangeSetWriteString( changeSet, testObject, "stringTest", "everything" );

			}

			const bool result = objyChangeSetSubmit( changeSet );
			assert( result );
		}

		{
			const ObjyObject* testObject = objyObjectFind( context, testId );
			assert( testObject );

			const ObjyValue* rootValue = objyObjectGetValue( testObject );

			assert( objyValueGetKind( rootValue ) == ObjyTypeKind_Struct );

		}

		objyContextDestroy( context );

		objyObjectDestroy( rootContext, contextObject );
	}

	objyDestroy( system );
	return 0;
}
