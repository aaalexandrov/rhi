set(dir_SOURCES
	base_vk.h
	base_vk.cpp

	buffer_vk.h
	buffer_vk.cpp

	compute_pass_vk.h
	compute_pass_vk.cpp

	copy_pass_vk.h
	copy_pass_vk.cpp

	graphics_pass_vk.h
	graphics_pass_vk.cpp

	pipeline_vk.h
	pipeline_vk.cpp

	present_pass_vk.h
	present_pass_vk.cpp

	rhi_vk.h
	rhi_vk.cpp

	sampler_vk.h
	sampler_vk.cpp

	submit_vk.h
	submit_vk.cpp

	swapchain_vk.h
	swapchain_vk.cpp

	texture_vk.h
	texture_vk.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)

if(WIN32)
	set(VK_LIB vulkan-1)
else()
	set(VK_LIB vulkan)
endif()

target_link_libraries(${BINARY} PRIVATE 
	${VK_LIB}
	shaderc_shared
	spirv-cross-core
	spirv-cross-reflect
)