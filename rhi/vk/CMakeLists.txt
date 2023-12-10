set(dir_SOURCES
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

target_link_libraries(${BINARY} PRIVATE vulkan-1)