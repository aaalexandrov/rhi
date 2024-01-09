set(dir_SOURCES
	imgui_ctx.h
	imgui_ctx.cpp

	ui.h
	ui.cpp

	use_imgui.h
	use_imgui.cpp

	window.h
	window.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)