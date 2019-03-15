cmake_minimum_required(VERSION 3.10)
project(vkLearn LANGUAGES CXX C)

if (APPLE)
    message(ERROR "Vulkan does not work on Apple")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCE_ROOT "${PROJECT_SOURCE_DIR}/source")

find_package(fmt REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

if (MSVC)
    set(COMMON_COMPILER_FLAGS /W4 /WX)
    set(COMMON_DEBUG_FLAGS "$<$<CONFIG:DEBUG>:/ZI>")
endif()

add_subdirectory("${SOURCE_ROOT}")
