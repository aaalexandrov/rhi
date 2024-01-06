set(dir_SOURCES
	component.h
	component.cpp

	object.h
	object.cpp

	rendering.h
	rendering.cpp

	sys.h
	sys.cpp

	use_imgui.h
	use_imgui.cpp

	world.h
	world.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)
