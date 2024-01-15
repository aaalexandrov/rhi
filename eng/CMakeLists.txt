set(dir_SOURCES
	component.h
	component.cpp

	object.h
	object.cpp

	sys.h
	sys.cpp

	world.h
	world.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)

add_subdirectory(render)
add_subdirectory(ui)
