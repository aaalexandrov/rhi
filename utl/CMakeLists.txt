set(dir_SOURCES
	algo.h 
	algo.cpp

	dbg.h
	dbg.cpp

	enumutl.h
	enumutl.cpp

	geom.h
	geom.cpp

	mathutl.h
	mathutl.cpp

	mem.h
	mem.cpp

	msg.h
	msg.cpp

	type_info.h
	type_info.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)