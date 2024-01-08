set(dir_SOURCES
	ui.h
	ui.cpp

	use_imgui.h
	use_imgui.cpp

	window.h
	window.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)