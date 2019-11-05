get_filename_component(EXEC_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EXAMPLE_ROOT "${CMAKE_CURRENT_LIST_DIR}")

set(SOURCE_LIST
    "${EXAMPLE_ROOT}/main.cpp"
    "${EXAMPLE_ROOT}/Application.cpp"
    "${EXAMPLE_ROOT}/stb_image.cpp"
    )
set(INCLUDE_LIST
    "${EXAMPLE_ROOT}/Application.hpp"
    "${EXAMPLE_ROOT}/stb_image.h"
    )

set(KERNELS 
    "${EXAMPLE_ROOT}/shaders/triangle.vert"
    "${EXAMPLE_ROOT}/shaders/triangle.frag"
    )
set(COMPILED_KERNELS 
    "${EXAMPLE_ROOT}/shaders/triangle.vert.spv"
    "${EXAMPLE_ROOT}/shaders/triangle.frag.spv"
    )

set(PATH_INCLUDE "${EXAMPLE_ROOT}/Paths.hpp")
configure_file("${EXAMPLE_ROOT}/Paths.hpp.in" ${PATH_INCLUDE})
list(APPEND INCLUDE_LIST ${PATH_INCLUDE})

source_group("source" FILES ${SOURCE_LIST})
source_group("include" FILES ${INCLUDE_LIST})
source_group("shaders" FILES ${KERNELS})
source_group("spir-v" FILES ${COMPILED_KERNELS})

add_executable(${EXEC_NAME} ${SOURCE_LIST} ${INCLUDE_LIST} ${KERNELS}
    ${COMPILED_KERNELS})
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

foreach(KERNEL ${KERNELS})
    add_custom_command(OUTPUT ${KERNEL}.spv
        COMMAND glslc ${KERNEL} -g -o ${KERNEL}.spv
        DEPENDS ${KERNEL}
        COMMENT "Rebuilding ${KERNEL}.spv"
        )
endforeach()
