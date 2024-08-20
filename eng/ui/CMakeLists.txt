set(dir_SOURCES
	imgui_ctx.h
	imgui_ctx.cpp

	properties.h
	properties.cpp

	ui.h
	ui.cpp

	window.h
	window.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)