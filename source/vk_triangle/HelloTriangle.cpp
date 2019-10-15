#include "HelloTriangle.hpp"

#include <algorithm>
#include <array>
#include <fmt/printf.h>
#include <functional>

namespace globals
{
    static constexpr auto windowWidth{800};
    static constexpr auto windowHeight{600};
    static constexpr std::array<const char*, 1> validationLayers{
        "VK_LAYER_KHRONOS_validation"};

#if defined(NDEBUG)
    static constexpr auto enableValidationLayers{false};
#else
    static constexpr auto enableValidationLayers{true};
#endif

    // We need to declare these function pointers ourselves as they are not
    // loaded automatically by Vulkan.
    PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;
} // namespace globals

static VKAPI_ATTR vk::Bool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              VkDebugUtilsMessengerCallbackDataEXT const* callbackData,
              [[maybe_unused]] void* userData)
{
    auto severity =
        static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity);
    auto type = static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageType);

    std::string message{};
    message += vk::to_string(severity) + ":";
    message += vk::to_string(type) + ": (";
    message += callbackData->pMessageIdName;
    message += "): ";
    message += callbackData->pMessage;
    fmt::print("{}\n", message);

    return false;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger)
{
    return globals::pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo,
                                                      pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* pAllocator)
{
    return globals::pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger,
                                                       pAllocator);
}

void HelloTriangleApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initVulkan()
{
    createInstance();
    setupDebugMessenger();
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
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void HelloTriangleApplication::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    mWindow = glfwCreateWindow(globals::windowWidth, globals::windowHeight,
                               "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::createInstance()
{
    // Struct holding the data from our application. This can be mostly left as
    // is (with the sole exception of perhaps updating the API version as we go
    // along). Note that this struct is optional, but its worthwhile setting it
    // up so that the correct version is used.
    vk::ApplicationInfo appInfo{"Hello Triangle", 1, "No engine", 1,
                                VK_API_VERSION_1_1};

    // Optional: List all the available extensions.
    listExtensions();

    // Optional: Check to ensure that all of the required validation layers
    // exist.
    if constexpr (globals::enableValidationLayers)
    {
        if (!checkLayers(
                std::vector<const char*>(globals::validationLayers.begin(),
                                         globals::validationLayers.end()),
                vk::enumerateInstanceLayerProperties()))
        {
            throw std::runtime_error{
                "error: there are missing required validation layers."};
        }
    }

    auto extensionNames = getRequiredExtensions();

    // This struct is NOT optional. It specifies all the information required to
    // create the Vulkan instance. The parameters that the constructor requires
    // (though keep in mind that all of them have default values assigned) are:
    // 1. flags: These are the instance creation flags. They are empty and
    // should be left as is.
    // 2. appInfo: The pointer to the vk::ApplicationInfo struct we made
    // earlier.
    // 3. enabledLayerCount: The number of enabled validation layers.
    // 4. enabledLayerNames: The names of the validation layers.
    // 5. enabledExtensionCount: The number of extensions to enable.
    // 6. enabledExtensionNames: The names of the extensions.
    vk::InstanceCreateInfo createInfo{
        vk::InstanceCreateFlags{},
        &appInfo,
        (globals::enableValidationLayers)
            ? static_cast<std::uint32_t>(globals::validationLayers.size())
            : 0,
        (globals::enableValidationLayers) ? globals::validationLayers.data()
                                          : nullptr,
        static_cast<std::uint32_t>(extensionNames.size()),
        extensionNames.data()};

    auto debugCreateInfo = getDebugMessengerCreateInfo();
    if constexpr (globals::enableValidationLayers)
    {
        createInfo.setPNext(&debugCreateInfo);
    }

    // Now that we have everything, we can create the instance.
    mInstance = vk::createInstanceUnique(createInfo);
}

void HelloTriangleApplication::listExtensions()
{
    // This retrieves the list of extensions that are available. Notice that
    // since we left the argument list empty, it will simply retrieve all
    // extensions. If we wish to search for a particular extension, we can
    // specify its name.
    std::vector<vk::ExtensionProperties> extensions =
        vk::enumerateInstanceExtensionProperties();

    // Now print out the available extensions.
    for (auto const& extension : extensions)
    {
        fmt::print("{}\n", extension.extensionName);
    }
}

bool HelloTriangleApplication::checkExtensions(
    std::vector<char const*> const& layers,
    std::vector<vk::ExtensionProperties> const& properties)
{
    return std::all_of(layers.begin(), layers.end(), [&properties](auto name) {
        std::string_view layerName{name};
        auto result =
            std::find_if(properties.begin(), properties.end(),
                         [&layerName](auto const& property) {
                             std::string_view propName{property.extensionName};
                             return layerName == propName;
                         });

        return result != properties.end();
    });
}

bool HelloTriangleApplication::checkLayers(
    std::vector<char const*> const& layers,
    std::vector<vk::LayerProperties> const& properties)
{
    return std::all_of(layers.begin(), layers.end(), [&properties](auto name) {
        std::string_view layerName{name};
        auto result =
            std::find_if(properties.begin(), properties.end(),
                         [&layerName](auto const& property) {
                             std::string_view propName{property.layerName};
                             return layerName == propName;
                         });

        return result != properties.end();
    });
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions()
{
    // Because Vulkan is a platform agnostic API, we need an extension to
    // interface with the OS window system. We can handle this easily with GLFW,
    // which tells us which extensions are required.
    std::uint32_t glfwExtensionCount{0};
    const char** glfwExtensionNames;
    glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(
        glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);

    if constexpr (globals::enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Optional: Check to ensure that all the required extensions are present.
    if (!checkExtensions(extensions,
                         vk::enumerateInstanceExtensionProperties()))
    {
        throw std::runtime_error{
            "error: there are missing required extensions."};
    }

    return extensions;
}

void HelloTriangleApplication::setupDebugMessenger()
{
    // Because the debug messenger is an extension, Vulkan does not load the
    // function pointers by default. Instead, we have to manually load them
    // ourselves. In particular, we need to load the pointers for the creation
    // and destruction of the messenger.
    globals::pfnVkCreateDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            mInstance->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if (!globals::pfnVkCreateDebugUtilsMessengerEXT)
    {
        throw std::runtime_error{"error: unable to find "
                                 "pfnVkCreateDebugUtilsMessengerEXT function."};
    }

    globals::pfnVkDestroyDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            mInstance->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
    if (!globals::pfnVkDestroyDebugUtilsMessengerEXT)
    {
        throw std::runtime_error{
            "error: unable to find "
            "pfnVkDestroyDebugUtilsMessengerEXT function."};
    }

    mDebugMessenger = mInstance->createDebugUtilsMessengerEXTUnique(
        getDebugMessengerCreateInfo());
}

vk::DebugUtilsMessengerCreateInfoEXT
HelloTriangleApplication::getDebugMessengerCreateInfo()
{
    // These two setup the types of messages that we want to receive from
    // Vulkan. To keep things simple, pretty much everything is enabled.
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags{
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError};
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags{
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation};

    // This struct sets up the data we need to create the debug messenger.
    // The parameters are (once more keeping in mind that all the parameters
    // have default values):
    // 1. flags: These are the messenger creation flags and should be left as
    // is.
    // 2. severityFlags: the flags denoting the message severity.
    // 3. messageTypeFlags: the flags denoting the mssage type.
    // 4. debugCallback: the pointer to the callback function.
    // 5. userData: pointer to any custom user data we want to pass.
    vk::DebugUtilsMessengerCreateInfoEXT createInfo{
        {}, severityFlags, messageTypeFlags, &debugCallback, nullptr};

    return createInfo;
}
