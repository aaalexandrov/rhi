set(dir_SOURCES
	use_imgui.h
	use_imgui.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)
