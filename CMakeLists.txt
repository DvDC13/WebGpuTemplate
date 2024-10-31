cmake_minimum_required(VERSION 3.15)
project(webOceanGpu VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(COMPILE_WARNING_AS_ERROR ON)

if (NOT EMSCRIPTEN)
    add_subdirectory(glfw)
else()
    add_subdirectory(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(webgpu)

add_subdirectory(glfw3webgpu)

add_executable(webOceanGpu main.cpp
    webgpu-utils.cpp webgpu-utils.h
    application.cpp application.h
)

if (MSVC)
    target_compile_options(webOceanGpu PRIVATE /W4)
else()
    target_compile_options(webOceanGpu PRIVATE -Wall -Wextra -pedantic)
endif()

if (XCODE)
    set_target_properties(webOceanGpu PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()

target_link_libraries(webOceanGpu PRIVATE webgpu glfw glfw3webgpu)

target_copy_webgpu_binaries(webOceanGpu)

if (EMSCRIPTEN)
    target_link_options(webOceanGpu PRIVATE "-sASYNCIFY")
    set_target_properties(webOceanGpu PROPERTIES SUFFIX ".html")
endif()