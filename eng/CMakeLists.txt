set(dir_SOURCES
	object.h
	object.cpp

	use_imgui.h
	use_imgui.cpp

	world.h
	world.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)
