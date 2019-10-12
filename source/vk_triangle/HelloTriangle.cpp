#include "HelloTriangle.hpp"

#include <algorithm>
#include <fmt/printf.h>

static constexpr auto gWindowWidth{800};
static constexpr auto gWindowHeight{600};

namespace vk
{
    constexpr std::uint32_t
    makeVersion(std::uint32_t major, std::uint32_t minor, std::uint32_t patch)
    {
        return (((major) << 0x16) | ((minor) << 0x0C) | patch);
    }
} // namespace vk

void errorCallack(int code, const char* message)
{
    fmt::print("error ({}): {}\n", code, message);
}

void HelloTriangleApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow()
{
    glfwInit();

    // Since GLFW is designed to create an OpenGL context, we need to disable it
    // for Vulkan.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfwSetErrorCallback(errorCallack);
    mWindow = glfwCreateWindow(gWindowWidth, gWindowHeight, "Vulkan", nullptr,
                               nullptr);
}

void HelloTriangleApplication::initVulkan()
{
    checkExtensions();
    createInstance();
}

void HelloTriangleApplication::createInstance()
{
    // Fill in the instance struct with some information about our application.
    // While it is technically optional, it allows the driver to optimize
    // certain things. Any global properties that are known beforehand should be
    // filled in here.
    vk::ApplicationInfo appInfo{"Hello Triangle", vk::makeVersion(1, 1, 0),
                                "No Engine", vk::makeVersion(1, 0, 0),
                                VK_API_VERSION_1_1};

    // This struct is NOT optional. It tells Vulkan which global extensions and
    // validation layers we want. Note that global means the whole app, not a
    // particular device.
    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;

    // Because Vulkan is API agnostic, we have to load the extensions here. In
    // this case, we use GLFW's function to load the extensions it needs.
    std::uint32_t glfwExtensionCount{0};
    const char**  glfwExtensions{nullptr};

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount   = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount       = 0;

    // Note: because we didn't define VULKAN_HPP_NO_EXCEPTIONS, then this
    // function (and pretty much all create functions) will throw. The
    // alternative is to define VULKAN_HPP_NO_EXCEPTIONS, in which case we get a
    // tuple back and we need to check the result manually. So this code
    // becomes:
    // auto [result, instance] = vk::createInstance(createInfo);
    // if (result != vk::Result::eSuccess) { ... }
    mInstance = vk::createInstance(createInfo);
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
    }
}

void HelloTriangleApplication::cleanup()
{
    mInstance.destroy();
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void HelloTriangleApplication::checkExtensions()
{
    // This function is entirely optional, but it shows how we can enumerate all
    // the extensions available and check to ensure that the required extensions
    // exist. This needs to be called prior to the creation of the instance, and
    // it's a good place to check if the minimum requirements are met.
    std::vector<vk::ExtensionProperties> extensions =
        vk::enumerateInstanceExtensionProperties();
    for (auto const& extension : extensions)
    {
        fmt::print("{}\n", extension.extensionName);
    }

    // Check to see if the extensions that GLFW requires are here. If any of
    // them aren't there, throw an exception.
    std::uint32_t glfwExtensionCount{0};
    const char**  glfwExtensions{nullptr};

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<std::string_view> glfwExtensionNames;
    for (std::uint32_t i{0}; i < glfwExtensionCount; ++i)
    {
        glfwExtensionNames.push_back(glfwExtensions[i]);
    }

    std::uint32_t numSupportedExtensions{0};
    for (std::size_t i{0}; i < glfwExtensionNames.size(); ++i)
    {
        auto name = glfwExtensionNames[i];
        if (std::find_if(extensions.begin(), extensions.end(),
                         [name](auto const& extension) {
                             std::string_view extName{extension.extensionName};
                             return extName == name;
                         }) != extensions.end())
        {
            ++numSupportedExtensions;
        }
    }

    if (numSupportedExtensions != glfwExtensionCount)
    {
        throw std::runtime_error{"error: system does not support the minimum "
                                 "required Vulkan extensions."};
    }
}
