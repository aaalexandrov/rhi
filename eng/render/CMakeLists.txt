set(dir_SOURCES
	rendering.h
	rendering.cpp

	scene.h
	scene.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)