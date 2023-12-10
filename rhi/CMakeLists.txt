set(dir_SOURCES
	base.h
	base.cpp

	pass.h
	pass.cpp

	resource.h
	resource.cpp

	rhi.h
	rhi.cpp

	submit.h
	submit.cpp
)

target_sources(${BINARY} PRIVATE ${dir_SOURCES} CMakeLists.txt)

add_subdirectory(vk)