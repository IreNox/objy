-- objy

module:add_include_dir( "include" )

module:add_define( "_CRT_SECURE_NO_WARNINGS" );

module:add_files( "include/objy/*.h" )
module:add_files( "src/*.h" )
module:add_files( "src/*.c" )
