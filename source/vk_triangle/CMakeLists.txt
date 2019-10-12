get_filename_component(EXEC_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(SOURCE_LIST
    "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/HelloTriangle.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/HelloTriangle.hpp"
    )
set(INCLUDE_LIST
    )

source_group("source" FILES ${SOURCE_LIST})

add_executable(${EXEC_NAME} ${SOURCE_LIST} ${INCLUDE_LIST})
target_compile_features(${EXEC_NAME} PUBLIC cxx_std_17)
target_include_directories(${EXEC_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${EXEC_NAME} PRIVATE fmt::fmt-header-only glm glfw Vulkan::Vulkan)
target_compile_options(${EXEC_NAME} PUBLIC "${COMMON_COMPILER_FLAGS}")
target_compile_options(${EXEC_NAME} PUBLIC "${COMMON_DEBUG_FLAGS}")
target_compile_options(${EXEC_NAME} PUBLIC "${COMMON_RELEASE_FLAGS}")
target_compile_options(${EXEC_NAME} PRIVATE -wd4005)
