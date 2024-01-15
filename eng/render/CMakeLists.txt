set(dir_SOURCES
	renderer.h
	renderer.cpp

	rendering.h
	rendering.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)