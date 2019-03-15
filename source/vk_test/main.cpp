#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <fmt/printf.h>

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <vulkan/vulkan.hpp>

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window{glfwCreateWindow(800, 600, "Vulkan Window", nullptr, nullptr)};

    std::uint32_t extensionCount{0};
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    fmt::print("{} extensions supported\n", extensionCount);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
