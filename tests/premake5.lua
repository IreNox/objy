-- tests

local project = Project:new( ProjectTypes.ConsoleApplication )

project:add_files( 'src/*.c' )

project:add_external( "local://.." )

finalize_default_solution( project )
