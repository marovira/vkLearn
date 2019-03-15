#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fmt/printf.h>
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
