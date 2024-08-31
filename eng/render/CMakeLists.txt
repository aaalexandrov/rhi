set(dir_SOURCES
	passes.h
	passes.cpp

	rendering.h
	rendering.cpp

	scene.h
	scene.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)