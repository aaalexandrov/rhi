set(dir_SOURCES
	algo.h 
	algo.cpp

	box_tree.h
	box_tree.cpp

	dbg.h
	dbg.cpp

	enumutl.h
	enumutl.cpp

	file.h
	file.cpp

	geom.h
	geom.cpp

	geom_primitive.h
	geom_primitive.cpp

	mathutl.h
	mathutl.cpp

	mem.h
	mem.cpp

	msg.h
	msg.cpp

	polytope.h
	polytope.cpp

	type_info.h
	type_info.cpp

	utl.h
	utl.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)