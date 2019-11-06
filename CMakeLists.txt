cmake_minimum_required(VERSION 3.10)
project(vkLearn LANGUAGES CXX C)

if (APPLE)
    message(ERROR "Vulkan does not work on Apple")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCE_ROOT "${PROJECT_SOURCE_DIR}/source")
set(DATA_ROOT "${PROJECT_SOURCE_DIR}/data")
set(IMAGES_ROOT "${PROJECT_SOURCE_DIR}/images")

find_package(fmt REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(
    atlas
    GIT_REPOSITORY https://github.com/marovira/atlas.git)
FetchContent_Populate(atlas)
add_subdirectory(${atlas_SOURCE_DIR} ${atlas_BINARY_DIR})

if (MSVC)
    set(COMMON_COMPILER_FLAGS /W4 /WX)
    set(COMMON_DEBUG_FLAGS "$<$<CONFIG:DEBUG>:/ZI>")
endif()

add_subdirectory("${SOURCE_ROOT}")
