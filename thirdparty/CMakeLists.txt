set(dir_SOURCES
	imgui/imgui.h
	imgui/imgui.cpp

	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_tables.cpp
	
	imgui/backends/imgui_impl_vulkan.cpp
	imgui/backends/imgui_impl_sdl2.cpp

	stb/stb_impl.cpp
	
	tinygltf/tiny_gltf_impl.cpp
)

target_include_directories(${BINARY} PUBLIC imgui stb tinygltf)
target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)
