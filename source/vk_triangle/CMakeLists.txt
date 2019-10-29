get_filename_component(EXEC_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EXAMPLE_ROOT "${CMAKE_CURRENT_LIST_DIR}")

set(SOURCE_LIST
    "${EXAMPLE_ROOT}/main.cpp"
    "${EXAMPLE_ROOT}/Application.cpp"
    )
set(INCLUDE_LIST
    "${EXAMPLE_ROOT}/Application.hpp"
    )

set(PATH_INCLUDE "${EXAMPLE_ROOT}/Paths.hpp")
configure_file("${EXAMPLE_ROOT}/Paths.hpp.in" ${PATH_INCLUDE})
list(APPEND INCLUDE_LIST ${PATH_INCLUDE})

source_group("source" FILES ${SOURCE_LIST})
source_group("include" FILES ${INCLUDE_LIST})

add_executable(${EXEC_NAME} ${SOURCE_LIST} ${INCLUDE_LIST})
target_compile_features(${EXEC_NAME} PUBLIC cxx_std_17)
target_include_directories(${EXEC_NAME} PUBLIC ${EXAMPLE_ROOT})
target_link_libraries(${EXEC_NAME} PRIVATE 
    fmt::fmt-header-only 
    glm 
    glfw 
    Vulkan::Vulkan)
target_compile_options(${EXEC_NAME} PUBLIC "${COMMON_COMPILER_FLAGS}")
target_compile_options(${EXEC_NAME} PUBLIC "${COMMON_DEBUG_FLAGS}")
target_compile_options(${EXEC_NAME} PUBLIC "${COMMON_RELEASE_FLAGS}")
target_compile_options(${EXEC_NAME} PRIVATE -wd4005)
