cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_policy(SET CMP0091 NEW) #to enable the MSVC_RUNTIME_LIBRARY property
project(rhi)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(LINUX TRUE)
endif()


set(VK_SDK_PATH $ENV{VULKAN_SDK})

if (WIN32)
	set(VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

	#in order for this to work, in the vulkan sdk the cmake/sdl2/* files need to be copied to cmake/*
	set(SDL2_DIR ${VK_SDK_PATH}/cmake)
endif()



find_package(SDL2 CONFIG REQUIRED)

set(SOURCES
	stdafx.h
	stdafx.cpp

	engine.cpp
)

set(BINARY engine)


add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)

add_executable(${BINARY} ${SOURCES})

add_subdirectory(thirdparty)

add_subdirectory(utl)
add_subdirectory(rhi)
add_subdirectory(eng)

set_property(TARGET ${BINARY} PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

if(LINUX)
	set(LIBS SDL2::SDL2 X11)
else()
	set(LIBS SDL2::SDL2-static)
endif()
target_link_libraries(${BINARY} PRIVATE ${LIBS})

target_include_directories(${BINARY} PRIVATE . ${VK_SDK_PATH}/include ${VK_SDK_PATH}/include/vma)

target_link_directories(${BINARY} PRIVATE ${VK_SDK_PATH}/lib)

target_precompile_headers(${BINARY} PRIVATE stdafx.h)



