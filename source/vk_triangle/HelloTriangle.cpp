#include "HelloTriangle.hpp"
#include "Paths.hpp"

#include <algorithm>
#include <array>
#include <fmt/printf.h>
#include <fstream>
#include <functional>
#include <limits>
#include <set>

// Some general notes:
// Throughout this tutorial, we are using the full explicit initialization of
// the Vulkan objects. This is merely to showcase what the parameters are as
// well as provide some form of documentation on them. In practice, this may not
// be readable and the individual parameters should be assigned separately.

namespace globals
{
    static constexpr auto windowWidth{800};
    static constexpr auto windowHeight{600};
    static constexpr auto maxFramesInFlight{2};
    static constexpr std::array<const char*, 1> validationLayers{
        "VK_LAYER_KHRONOS_validation"};
    static constexpr std::array<const char*, 1> deviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

static std::vector<char> readFile(std::string const& filename)
{
    std::ifstream file{filename, std::ios::ate | std::ios::binary};

    if (!file.is_open())
    {
        std::string message =
            fmt::format("error: unable to open file {}.", filename);
        throw std::runtime_error{message};
    }

    std::size_t fileSize = static_cast<std::size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

static void onFramebufferResize(GLFWwindow* window, [[maybe_unused]] int width,
                                [[maybe_unused]] int height)
{
    auto app = reinterpret_cast<HelloTriangleApplication*>(
        glfwGetWindowUserPointer(window));
    app->framebuffeResized();
}

void HelloTriangleApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::framebuffeResized()
{
    mFramebufferResized = true;
}

void HelloTriangleApplication::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        drawFrame();
        glfwPollEvents();
    }

    mDevice->waitIdle();
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
    mWindow = glfwCreateWindow(globals::windowWidth, globals::windowHeight,
                               "Vulkan", nullptr, nullptr);

    glfwSetWindowUserPointer(mWindow, this);
    glfwSetFramebufferSizeCallback(mWindow, onFramebufferResize);
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

void HelloTriangleApplication::pickPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> devices =
        mInstance->enumeratePhysicalDevices();
    if (devices.empty())
    {
        throw std::runtime_error{
            "error: there are no devices that support Vulkan."};
    }

    for (auto const& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            mPhysicalDevice = device;
            break;
        }

        if (mPhysicalDevice)
        {
            throw std::runtime_error{
                "error: there are no devices that support Vulkan."};
        }
    }
}

bool HelloTriangleApplication::isDeviceSuitable(
    vk::PhysicalDevice const& device)
{
    // A few notes on this function:
    // Currently it is setup to be really simple and to only accept either
    // discrete GPUs or integrated, but this can be easily extended to handle
    // anything we wish. The device properties struct contains things like the
    // type of device, API version, etc. while the device features struct
    // contains the actual capabilities of the device. An interesting point here
    // (for multi-GPU architectures) is that we can simply rank the devices
    // based on whether they support the features we want and then select the
    // highest score available.
    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures     = device.getFeatures();
    auto indices                                  = findQueueFamilies(device);

    bool isDeviceValid =
        deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ||
        deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu;
    bool areExtensionsSupported = checkDeviceExtensionSupport(device);

    // The existence of the swap chains does not actually imply that they are
    // compatible with our surface. As a result, we first need to check
    // that they are before they can be used.
    bool isSwapChainAdequate{false};
    if (areExtensionsSupported)
    {
        auto swapChainDetails = querySwapChainSupport(device);
        isSwapChainAdequate   = !swapChainDetails.formats.empty() &&
                              !swapChainDetails.presentModes.empty();
    }

    return isDeviceValid && indices.isComplete() && areExtensionsSupported &&
           isSwapChainAdequate;
}

HelloTriangleApplication::QueueFamilyIndices
HelloTriangleApplication::findQueueFamilies(vk::PhysicalDevice const& device)
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies =
        device.getQueueFamilyProperties();
    std::uint32_t i{0};
    for (auto const& queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0 &&
            queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }

        // Check to see if the queue family has support for presenting.
        vk::Bool32 presetSupport = device.getSurfaceSupportKHR(i, *mSurface);
        if (queueFamily.queueCount > 0 && presetSupport)
        {
            indices.presentFamily = i;
        }
        ++i;
    }

    return indices;
}

void HelloTriangleApplication::createLogicalDevice()
{
    auto indices = findQueueFamilies(mPhysicalDevice);

    // It is perfectly possible for the queue indices of the
    // graphics queue and presentation queues to be different. As such,
    // we need to know how many queues we will need to create on the
    // logical device.
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<std::uint32_t> uniqueQueueFamilies{*indices.graphicsFamily,
                                                *indices.presentFamily};

    float queuePriority{1.0f};
    for (auto queueFamily : uniqueQueueFamilies)
    {
        // As usual, this struct contains the information required to create
        // a device queue. The idea is that command buffers will submit their
        // work into the device queue, which will then execute the specified
        // commands. The parameters then are:
        // 1. flags: The device creation flags that can be left as is.
        // 2. queueFamilyIndex: The index of the queue family.
        // 3. queueCount: The number of queues we want to create.
        // 4. priority: The priority level in range 0.0f -> 1.0f.
        //
        // Note: there is a limited number of queues that can be created
        // from a single device. In general, we only really need one, since
        // the command buffers can be created across multiple threads.
        vk::DeviceQueueCreateInfo queueCreateInfo{
            {}, queueFamily, 1, &queuePriority};
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // This holds the features that we want our logical device to
    // have. Note that not everything needs to be enabled. You are free
    // to only enable the features that you actually need.
    vk::PhysicalDeviceFeatures deviceFeatures{};

    // Similar to the InstaceCreateInfo, this contains all the required
    // information to create the logical device. The parameters are:
    // 1. flags: The device creation flags and can be left as is.
    // 2. queueCreateInfoCount: The number of create info queue structs.
    // 3. pQueueCreateInfo: The pointer to the create info queue structs.
    // 4. enabledLayerCount: The number of enabled layers.
    // 5. ppEnabledLayerNames: The names of the layers to enable.
    // 6. enabledExtensionCount: The number of extensions to enable.
    // 7. ppEnabledExtensionNames: The names of the extensions to enable.
    // 8. pEnabledFeatures: the pointer to the physical device features.
    //
    // Note that the extensions and layers we are enabling here are NOT the same
    // we enabled before. These are per device, while the others are global.
    // Now, in modern implementations (i.e. 1.1.12+), there is no longer any
    // distinction between the validation layers of the device and those of the
    // instance, as such the values set in the device create info are ignored.
    // That being said, in order to remain backwards compatible with previous
    // versions, we can set them anyway. Note however that extensions are still
    // handled separately and they should be set accordingly.
    vk::DeviceCreateInfo createInfo{
        {},
        static_cast<std::uint32_t>(queueCreateInfos.size()),
        queueCreateInfos.data(),
        (globals::enableValidationLayers)
            ? static_cast<std::uint32_t>(globals::validationLayers.size())
            : 0,
        (globals::enableValidationLayers) ? globals::validationLayers.data()
                                          : nullptr,
        static_cast<std::uint32_t>(globals::deviceExtensions.size()),
        globals::deviceExtensions.data(),
        &deviceFeatures};
    mDevice        = mPhysicalDevice.createDeviceUnique(createInfo);
    mGraphicsQueue = mDevice->getQueue(*indices.graphicsFamily, 0);
    mPresentQueue  = mDevice->getQueue(*indices.presentFamily, 0);
}

void HelloTriangleApplication::createSurface()
{
    VkSurfaceKHR surface = static_cast<VkSurfaceKHR>(*mSurface);
    if (glfwCreateWindowSurface(*mInstance, mWindow, nullptr, &surface))
    {
        throw std::runtime_error{"error: could not create window surface."};
    }

    // The surface is technically a child of the instance, and so it must be
    // deleted prior to the deletion of the instance. If we simply do *mSurface
    // = surface; it will copy the surface over (which is what we want), but it
    // won't link the deleter of the surface with the instance. This then leads
    // to either an exception being thrown by attempting to access a nullptr, or
    // by the validation layers sending an error because we haven't deleted the
    // surface. The solution is then to actually create the unique surface with
    // the handle of the instance. Once all of this is done, everything works
    // correctly.
    mSurface = vk::UniqueSurfaceKHR{surface, *mInstance};
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(
    vk::PhysicalDevice const& device)
{
    // Similarly to how we checked whether the global instance had support for
    // the extensions we required, we also need to check if the device has
    // support for specific extensions. For rendering, we require support for
    // the swap chain. Since this is a device specific extension (as opposed to
    // a global one) we check this here.
    std::vector<vk::ExtensionProperties> availableExtensions =
        device.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(globals::deviceExtensions.begin(),
                                             globals::deviceExtensions.end());

    for (auto const& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

HelloTriangleApplication::SwapChainSupportDetails
HelloTriangleApplication::querySwapChainSupport(
    vk::PhysicalDevice const& device)
{
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(*mSurface);
    details.formats      = device.getSurfaceFormatsKHR(*mSurface);
    details.presentModes = device.getSurfacePresentModesKHR(*mSurface);

    return details;
}

vk::SurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(
    std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
    // For this particular example, we are going to be needing a swap chain that
    // supports 8-bit RGBA for the colour format and SRGB for the colour space.
    // Similar to how we can rank the devices depending on whether they support
    // what we want or not, we can also rank the formats here. For the most part
    // though, we can simply grab the first one that meets our needs.
    for (auto const& availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(
    std::vector<vk::PresentModeKHR> const& availablePresentModes)
{
    // Here we set under which conditions we swap our images and showing them to
    // the screen. Our options are:
    // * eImmediate: images submitted are transferred right away. May result in
    // tearing.
    // * eFifo: The swap chain is a queue and we take images from the front of
    // the queue to present while rendering happens on the back. This is the
    // most similar to vertical sync and double buffering.
    // * eFifoRelaced: Identical to fifo save for the case of when the queue is
    // empty and the application is late. In that case, the frame is transferred
    // right away, which may result in tearing.
    // * eMailbox: a variation on fifo. The idea is that if the queue is full,
    // we don't block the main application. Instead the images that are already
    // in the queue are replaced with the new ones. This allows us to implement
    // triple buffering for less tearing at less latency.
    for (auto const& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D HelloTriangleApplication::chooseSwapExtent(
    vk::SurfaceCapabilitiesKHR const& capabilities)
{
    // The swap extent is the resolution of the images in the swap chain and is
    // almost always the same size as the window we are rendering to. Keep in
    // mind that depending on the window manager we may have some variation on
    // the sizes, and so we use max of uint32_t to specify this.
    if (capabilities.currentExtent.width !=
        std::numeric_limits<std::uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(mWindow, &width, &height);
        vk::Extent2D actualExtent = {static_cast<std::uint32_t>(width),
                                     static_cast<std::uint32_t>(height)};

        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void HelloTriangleApplication::createSwapChain()
{
    // Vulkan does not support a framebuffer by default. Instead what we have
    // to do is create the default framebuffer, and all the setup it requires.
    // The swap chain is then the chain of images that we can swap for
    // performing things like double buffering. Now that we know that the swap
    // chain is supported, we need to create it. To do this, we first need to
    // query for the best settings (or nearest to) so that we can populate the
    // creation struct. We need to determine the following 3 settings:
    // 1. Surface format: controls the colour depth,
    // 2. Presentation mode: conditions for "swapping" images to the screen, and
    // 3. Swap extent: resolution of images in swap chain.
    //
    SwapChainSupportDetails swapChainDetails =
        querySwapChainSupport(mPhysicalDevice);

    vk::SurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(swapChainDetails.formats);
    vk::PresentModeKHR presentMode =
        chooseSwapPresentMode(swapChainDetails.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainDetails.capabilities);

    // We now need to specify the size of the swap chain. As a general rule of
    // thumb, its always better to have one more than the minimum available
    // size. That being said, certain devices allow the swap chains to have no
    // limit, which is represented by a size of 0. The check here ensures that
    // we don't exceed the maximum size (if there is one).
    std::uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;
    if (swapChainDetails.capabilities.maxImageCount > 0 &&
        imageCount > swapChainDetails.capabilities.maxImageCount)
    {
        imageCount = swapChainDetails.capabilities.maxImageCount;
    }

    // Recall that we created two queue families: one for presentation and one
    // for graphics. Given that we have two handles, these can either be
    // distinct or equal. This means that we have to tell the swap chain whether
    // the images are going to be used across several queue families or not.
    // There are two options:
    // 1. SharingMode::eExclusive: images are owned by a single queue and must
    // be explicitly transferred between queues before use. Offers the best
    // performance.
    // 2. SharingMode::eConcurrent: images can be shared across queue families
    // without explicit ownership transfers.
    //
    // For the time being we are going to be using the second option for the
    // sake of simplicity. In this case, we need to specify in advance which
    // queue families are going to be sharing ownership. If the graphics and
    // presentation are the same, then we should use exclusive mode. Here we
    // grab the indices of the queue families.
    QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);
    std::array<std::uint32_t, 2> queueFamilies{*indices.graphicsFamily,
                                               *indices.presentFamily};

    vk::SharingMode sharingMode;
    std::uint32_t queueFamilyIndexCount{0};
    const std::uint32_t* queueFamilyIndices{nullptr};
    if (*indices.graphicsFamily != *indices.presentFamily)
    {
        sharingMode = vk::SharingMode::eConcurrent;
        queueFamilyIndexCount =
            static_cast<std::uint32_t>(queueFamilies.size());
        queueFamilyIndices = queueFamilies.data();
    }
    else
    {
        sharingMode = vk::SharingMode::eExclusive;
    }

    // Same pattern as before. We create the image struct that holds the
    // creation data for the swap chain. Parameters are:
    // 1. surface: the surface we created previously,
    // 2. minImageCount: The number of images in the swap chain,
    // 3. imageFormat: the surface format we specify,
    // 4. imageColorSpace: the surface colour space we specify,
    // 5. iamgeExtent: the size of the images in the swap chain,
    // 6. imageArrayLayers: the number of layers each image consists of.
    // 7. imageUsage: Specifies how we are going to use the images.
    // 8. sharingMode: Specifies how images are shared amongst queues.
    // 9. queueFamilyIndexCount: the number of queue families.
    // 10. queueFamilyIndeces: the indices of the queue families.
    // 11. preTransform: the type of stransform we want to apply to images on
    // the swap chain (if any).
    // 12. compositeAlpha: whether the alpha channel should be used to blend
    // with other windows.
    // 13. presentMode: the way the images are presented.
    // 14. clipped: if true, then we don't care about the pixels that are
    // obscured by another window (for example). Should probably be left as
    // true.
    // 15. oldSwapChain: if our current swap chain becomes invalid and we need
    // to recreate it, we must provide the pointer to the old one here.
    //
    // Note that the number of layers per image will always be 1 unless we are
    // doing stereoscopic 3D. In the case of the imageUsage, if we are going
    // to be rendering to a separate image for post-processing and then copying
    // over to the swap chain, we would use ImageUsageFlagBits::eTransferDST
    vk::SwapchainCreateInfoKHR createInfo{
        {},
        *mSurface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        sharingMode,
        queueFamilyIndexCount,
        queueFamilyIndices,
        vk::SurfaceTransformFlagBitsKHR::eIdentity,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        true};

    mSwapChain            = mDevice->createSwapchainKHRUnique(createInfo);
    mSwapChainImages      = mDevice->getSwapchainImagesKHR(*mSwapChain);
    mSwapChainImageFormat = surfaceFormat.format;
    mSwapChainExtent      = extent;
}

void HelloTriangleApplication::createImageViews()
{
    mSwapChainImageViews.resize(mSwapChainImages.size());

    for (std::size_t i{0}; i < mSwapChainImages.size(); ++i)
    {
        // The components of the image view allow us to swizzle
        // the individual channels around as we need to. Unless
        // we are doing something really clever, this should
        // probably be left as is. Each parameter represents each
        // of the channels in our image.
        vk::ComponentMapping components{
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity};

        // THe resource range describes the purpose of the image and what parts
        // of the image should be accessed. In the case of our images, we are
        // going to be using colour targets without any mipmapping or multiple
        // layers. The parameters then are:
        // 1. aspectMask: the flags that control the type of image.
        // 2. baseMipLevel: the base mipmap level.
        // 3. levelCount: the number of mipmap levels.
        // 4. baseArrayLayer: if we have multiple layers, then this is the first
        // one.
        // 5. layerCount: the number of layers.
        vk::ImageSubresourceRange subresourceRange{
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        vk::ImageViewCreateInfo createInfo{{},
                                           mSwapChainImages[i],
                                           vk::ImageViewType::e2D,
                                           mSwapChainImageFormat,
                                           components,
                                           subresourceRange};

        mSwapChainImageViews[i] = mDevice->createImageViewUnique(createInfo);
    }
}

void HelloTriangleApplication::createGraphicsPipeline()
{
    std::string root{ShaderPath};
    auto vertexShaderCode   = readFile(root + "triangle.vert.spv");
    auto fragmentShaderCode = readFile(root + "triangle.frag.spv");

    vk::UniqueShaderModule vertModule = createShaderModule(vertexShaderCode);
    vk::UniqueShaderModule fragModule = createShaderModule(fragmentShaderCode);

    // This is very similar to OpenGL, where the shaders need to
    // be given a type. In this case, we do this with another create info
    // struct. The parameters are then:
    // 1. flags: leave as is.
    // 2. stage: The stage that the shader fits in.
    // 3. module: The module for that shader.
    // 4. name: The name of the entrypoint into the shader. This means that we
    // can bundle several entry points under a single shader and just change the
    // entry point here.
    // 5. specializationInfo: this allows us to control the values of constants
    // in order to optimize as early as possible the layout of the shader
    // itself.
    vk::PipelineShaderStageCreateInfo vertShaderInfo{
        {}, vk::ShaderStageFlagBits::eVertex, *vertModule, "main"};
    vk::PipelineShaderStageCreateInfo fragShaderInfo{
        {}, vk::ShaderStageFlagBits::eFragment, *fragModule, "main"};

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{
        vertShaderInfo, fragShaderInfo};

    // We need a way of describing the format of the vertex data that is being
    // sent to the vertex shader (similar to the attribute bindings in the vao
    // from OpenGL). To do this we use the struct below to specify the data.
    // The input paramaters are:
    // 1. flags: leave as is.
    // 2. vertexBindingsDescriptionCount: the number of vertex bindings.
    // 3. vertexBindingDescriptions: pointer to the vertex binding structures.
    // 4. vertexAttributeDescriptionCount: the number of vertex attributes.
    // 5. vertexAttributeDescriptions: pointer to the vertex attribute
    // structures.
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        {}, 0, nullptr, 0, nullptr};

    // Now that we know how the data in the vertex shader is laid out, we need
    // to specify what kind of geometry will be rendered so the assembler
    // can take care of it. The parameters are:
    // 1. flats: leave as is.
    // 2. topology: the type of primitive we are rendering.
    // 3. primitiveRestartEnable: Allows us to break up the primitives into
    // chunks.
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE};

    // Now we need to setup our viewport. This is going to be very similar to
    // how we set up the viewport in OpenGL, with the addition that we also have
    // to specify the depth range for the depth buffer. Unless something clever
    // is being done, leave in the range 0 to 1. The parameters are:
    // 1. x: the origin's x-coordinate.
    // 2. y: the origin's y-coordinate.
    // 3. width: width of the viewport.
    // 4. height: height of the viewport.
    // 5. minDepth: the minimum depth (must be at least 0).
    // 6. maxDepth: the maximum depth (must be at most 1).
    //
    // Note that in Vulkan the origin of the viewport is set on the
    // top-left corner, as opposed to the bottom-left of OpenGL. If we wish to
    // use the same setups that we had with OpenGL, then we have to make the
    // following changes here:
    // 1. The height of the viewport must be negative,
    // 2. The y coordinate of the viewport must be the height of the extent.
    vk::Viewport viewport{0.0f,
                          0.0f,
                          static_cast<float>(mSwapChainExtent.width),
                          static_cast<float>(mSwapChainExtent.height),
                          0.0f,
                          1.0f};

    // Next we setup the scissor rectangle. Since we want to draw the entire
    // framebuffer, we just initialize it to the same size as the extent.
    vk::Rect2D scissor{{0, 0}, mSwapChainExtent};

    // We have the viewport specifications as well as the scissor rectangle, so
    // we are now ready to combine the two into the viewport state. The
    // parameters are:
    // 1. flags: leave as is.
    // 2. viewportCount: the number of viewports.
    // 3. viewports: pointer to the list of viewports.
    // 4. scissorCount: the number of scissor rects.
    // 5. scissors: pointer to the list of scissor rects.
    vk::PipelineViewportStateCreateInfo viewportState{
        {}, 1, &viewport, 1, &scissor};

    // The viewport is setup, so the next thing to create is the rasterizer
    // stage. The parameters are:
    // 1. flags: leave as is.
    // 2. depthClampEnable: discards fragments beyond the near and far planes.
    // Requires enabling a GPU feature.
    // 3. rasterizerDiscardEnable: disables any and all output to the
    // framebuffer.
    // 4. polygonMode: same as OpenGL.
    // 5. cullMode: same as OpenGL.
    // 6. frontFace: same as OpenGL.
    // 7. depthBiasEnable: enables depth shifting.
    // 8. depthBiasConstantFactor: factor for depth shifting.
    // 9. depthBiasClamp: ditto.
    // 10. depthBiasSlopeFactor: ditto.
    // 11. lineWidth: width (in pixels) of lines.
    vk::PipelineRasterizationStateCreateInfo rasterizer{
        {},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        VK_FALSE,
        0.0f,
        0.0f,
        0.0f,
        1.0f};

    // Next is the multisampling portion which is used for anti-aliasing. For
    // now we will keep this disabled.
    vk::PipelineMultisampleStateCreateInfo multisampling{
        {},      vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE,
        VK_FALSE};

    // If we require any depth or stencil buffers, they would get configured
    // here. Since we don't need them for the time being, let's leave them
    // alone.

    // Next come colour blending settings. These are similar to the blend
    // functions that OpenGL has. The important point to note here is that these
    // settings are per attached framebuffer. There's an alternative struct
    // called vk::PipelineColorBlendStateCreateInfo which contains the global
    // colour blending settings.
    vk::PipelineColorBlendAttachmentState colourBlendAttachment{
        VK_FALSE,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    // We have the per-framebuffer settings, so now we create the global
    // settings for the entire swap chain.
    vk::PipelineColorBlendStateCreateInfo colourBlending{
        {}, VK_FALSE, vk::LogicOp::eCopy, 1, &colourBlendAttachment};

    // The last piece is to designate all the dynamic state in the pipeline.
    // In particular, the viewport, line width, and blend constants. The rest
    // are static. We can do this as follows:
    std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport,
                                                  vk::DynamicState::eLineWidth};

    vk::PipelineDynamicStateCreateInfo dynamicState{
        {},
        static_cast<std::uint32_t>(dynamicStates.size()),
        dynamicStates.data()};

    // Now that we have the pipeline, we can set the layout of our uniform
    // variables.
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        {}, 0, nullptr, 0, nullptr};

    mPipelineLayout =
        mDevice->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

    // Finally, we put everything together to create the graphics pipeline
    // itself.
    vk::GraphicsPipelineCreateInfo pipelineInfo{
        {},
        static_cast<std::uint32_t>(shaderStages.size()),
        shaderStages.data(),
        &vertexInputInfo,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        nullptr,
        &colourBlending,
        nullptr,
        *mPipelineLayout,
        *mRenderPass,
        0,
        {},
        -1};

    mGraphicsPipeline = mDevice->createGraphicsPipelineUnique({}, pipelineInfo);
}

vk::UniqueShaderModule
HelloTriangleApplication::createShaderModule(std::vector<char> const& code)
{
    // The shader module is basically a wrapper around the shader code itself.
    // In this it is very similar to the shaders from OpenGL. The parameters
    // of the struct are very simple:
    // 1. flags: left as is.
    // 2. codeSize: the size of the buffer containing the SPIR-V code.
    // 3. code: the pointer to the code.
    //
    // Note that the code needs to be a buffer of std::uint32_t. If we use
    // vectors with char, then everything will line up correctly.
    vk::ShaderModuleCreateInfo createInfo{
        {}, code.size(), reinterpret_cast<std::uint32_t const*>(code.data())};

    vk::UniqueShaderModule shaderModule =
        mDevice->createShaderModuleUnique(createInfo);
    return shaderModule;
}

void HelloTriangleApplication::createRenderPass()
{
    vk::AttachmentDescription colourAttachment{{},
                                               mSwapChainImageFormat,
                                               vk::SampleCountFlagBits::e1,
                                               vk::AttachmentLoadOp::eClear,
                                               vk::AttachmentStoreOp::eStore,
                                               vk::AttachmentLoadOp::eDontCare,
                                               vk::AttachmentStoreOp::eDontCare,
                                               vk::ImageLayout::eUndefined,
                                               vk::ImageLayout::ePresentSrcKHR};

    vk::AttachmentReference colourAttachmentRef{
        0, vk::ImageLayout::eColorAttachmentOptimal};

    vk::SubpassDescription subpass{{}, vk::PipelineBindPoint::eGraphics,
                                   0,  nullptr,
                                   1,  &colourAttachmentRef};

    vk::SubpassDependency dependency{
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentRead |
            vk::AccessFlagBits::eColorAttachmentWrite};

    vk::RenderPassCreateInfo renderPassInfo{{},       1, &colourAttachment, 1,
                                            &subpass, 1, &dependency};

    mRenderPass = mDevice->createRenderPassUnique(renderPassInfo);
}

void HelloTriangleApplication::createFramebuffers()
{
    mSwapChainFramebuffers.resize(mSwapChainImageViews.size());

    // Iterate over each image view and create the framebuffers.
    for (std::size_t i{0}; i < mSwapChainImageViews.size(); ++i)
    {
        std::array<vk::ImageView, 1> attachments{*mSwapChainImageViews[i]};

        vk::FramebufferCreateInfo framebufferInfo{
            {},
            *mRenderPass,
            static_cast<std::uint32_t>(attachments.size()),
            attachments.data(),
            mSwapChainExtent.width,
            mSwapChainExtent.height,
            1};

        mSwapChainFramebuffers[i] =
            mDevice->createFramebufferUnique(framebufferInfo);
    }
}

void HelloTriangleApplication::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(mPhysicalDevice);

    vk::CommandPoolCreateInfo poolInfo{{}, *queueFamilyIndices.graphicsFamily};
    mCommandPool = mDevice->createCommandPoolUnique(poolInfo);
}

void HelloTriangleApplication::createCommandBuffers()
{
    mCommandBuffers.resize(mSwapChainFramebuffers.size());

    // The level parameter in the command buffer specifies the following
    // behaviour:
    // 1. Primary: can be submitted to a queue for execution, but cannot be
    // called from other command buffers.
    // 2. Secondary: cannot be submitted directly, but can be called from a
    // primary command buffer.
    vk::CommandBufferAllocateInfo allocInfo{
        *mCommandPool, vk::CommandBufferLevel::ePrimary,
        static_cast<std::uint32_t>(mCommandBuffers.size())};

    mCommandBuffers = mDevice->allocateCommandBuffersUnique(allocInfo);

    for (std::size_t i{0}; i < mCommandBuffers.size(); ++i)
    {
        // Before we begin, let's create the info struct for the command buffer
        // that will hold our commands. Once we have that, we can set the
        // command buffer so it starts recording things.
        vk::CommandBufferBeginInfo beginInfo{{}, nullptr};
        mCommandBuffers[i]->begin(beginInfo);

        // Next comes the equivalent of glClearColor. We specify the colour
        // value, along with the information about the dimensions of the
        // framebuffers, and the renderpass.
        vk::ClearValue clearValue{};
        clearValue.color =
            vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
        vk::RenderPassBeginInfo renderPassInfo{
            *mRenderPass, *mSwapChainFramebuffers[i],
            vk::Rect2D{{0, 0}, mSwapChainExtent}, 1, &clearValue};

        // We now begin the render pass.
        mCommandBuffers[i]->beginRenderPass(&renderPassInfo,
                                            vk::SubpassContents::eInline);

        // First thing we need to do is bind the graphics pipeline.
        mCommandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics,
                                         *mGraphicsPipeline);

        // Since the shaders already contain all the information, just tell
        // Vulkan to render everything. This is going to be very similar to
        // OpenGL drawBuffers command.
        mCommandBuffers[i]->draw(3, 1, 0, 0);

        // We are done, so end the render pass.
        mCommandBuffers[i]->endRenderPass();

        // And close off the command buffer. Note that any error checking will
        // be done at this point.
        mCommandBuffers[i]->end();
    }
}

void HelloTriangleApplication::createSyncObjects()
{
    mImageAvailableSemaphores.resize(globals::maxFramesInFlight);
    mRenderFinishedSemaphores.resize(globals::maxFramesInFlight);
    mInFlightFences.resize(globals::maxFramesInFlight);

    vk::SemaphoreCreateInfo semaphoreInfo{};

    // Fences are created in an unsignaled state. This means that any
    // function waiting on this fence will be stuck waiting forever. To
    // solve this, we must set the fence to be signaled.
    vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};

    for (std::size_t i{0}; i < globals::maxFramesInFlight; ++i)
    {
        mImageAvailableSemaphores[i] =
            mDevice->createSemaphoreUnique(semaphoreInfo);
        mRenderFinishedSemaphores[i] =
            mDevice->createSemaphoreUnique(semaphoreInfo);
        mInFlightFences[i] = mDevice->createFenceUnique(fenceInfo);
    }
}

void HelloTriangleApplication::drawFrame()
{
    // Before any action takes place, let's wait for the fences on the frames.
    mDevice->waitForFences({*mInFlightFences[mCurrentFrame]}, VK_TRUE,
                           std::numeric_limits<std::uint64_t>::max());

    // First things first, we need to acquire an image from the swap chain. We
    // can specify a timeout if we wish, but for now we'll disable it, and we'll
    // use our semaphore to synchronize access with the images.
    auto result = mDevice->acquireNextImageKHR(
        *mSwapChain, std::numeric_limits<std::uint32_t>::max(),
        *mImageAvailableSemaphores[mCurrentFrame], {});

    // The return value of ErrorOutOfDateKHR is not guaranteed to be
    // triggered by the platforms or the drivers. As a result, we need
    // to provide our own flag to check when the swapchain becomes invalid.
    if (result.result == vk::Result::eErrorOutOfDateKHR ||
        result.result == vk::Result::eSuboptimalKHR || mFramebufferResized)
    {
        // Because the return of acquireNextImageKHR is not guaranteed,
        // we need an extra check. If the window is resized but the return is
        // success, then the corresponding semaphore is signaled. Since we
        // rebuild the swapchain and then return, the semaphore is still
        // in its signaled state, which would then cause an error when we
        // draw the next frame. The solution to the problem is to reset the
        // semaphore if the window is resized and acquireNextImageKHR returned
        // success. Note that this will not happen if the return is a failure,
        // and so this code is not needed in those cases.
        if (mFramebufferResized && result.result == vk::Result::eSuccess)
        {
            mImageAvailableSemaphores[mCurrentFrame].reset();

            vk::SemaphoreCreateInfo semaphoreInfo{};
            mImageAvailableSemaphores[mCurrentFrame] =
                mDevice->createSemaphoreUnique(semaphoreInfo);
        }

        mFramebufferResized = false;
        recreateSwapChain();
        return;
    }
    else if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error{"error: unable to acquire swap chain image."};
    }

    // Now grab the index of the image that we retrieved. This will tell us
    // which command buffer to use.
    std::uint32_t imageIndex = result.value;

    std::array<vk::Semaphore, 1> waitSemaphores{
        *mImageAvailableSemaphores[mCurrentFrame]};
    std::array<vk::PipelineStageFlags, 1> waitStages{
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    std::array<vk::Semaphore, 1> signalSemaphores{
        *mRenderFinishedSemaphores[mCurrentFrame]};

    // Now we need to submit the command buffer. The wait semaphores tell
    // the command what to wait on, and the signal semaphores will be signaled
    // once the command is done.
    vk::SubmitInfo submitInfo{
        static_cast<std::uint32_t>(waitSemaphores.size()),
        waitSemaphores.data(),
        waitStages.data(),
        1,
        &(*mCommandBuffers[imageIndex]),
        static_cast<std::uint32_t>(signalSemaphores.size()),
        signalSemaphores.data()};

    // Notice that, unlike semaphores, we need to reset the state of the fence
    // so it can be used again.
    mDevice->resetFences({*mInFlightFences[mCurrentFrame]});

    // The array here is to make it more scalable when we're submitting multiple
    // things, and the fence that can be passed in is for signaling when all
    // submitted command buffers finish execution.
    mGraphicsQueue.submit({submitInfo}, *mInFlightFences[mCurrentFrame]);

    std::array<vk::SwapchainKHR, 1> swapChains{*mSwapChain};

    // The last step is to submit the resulting render back into the swap chain
    // so that it can be shown on the screen.
    vk::PresentInfoKHR presentInfo{
        1,
        signalSemaphores.data(),
        static_cast<std::uint32_t>(swapChains.size()),
        swapChains.data(),
        &imageIndex,
        nullptr};

    mPresentQueue.presentKHR(presentInfo);
    mCurrentFrame = (mCurrentFrame + 1) % globals::maxFramesInFlight;
}

void HelloTriangleApplication::recreateSwapChain()
{
    int width{0}, height{0};
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(mWindow, &width, &height);
        glfwWaitEvents();
    }

    mDevice->waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
}

void HelloTriangleApplication::cleanupSwapChain()
{
    for (std::size_t i{0}; i < mSwapChainFramebuffers.size(); ++i)
    {
        mDevice->destroyFramebuffer(*mSwapChainFramebuffers[i]);
        mSwapChainFramebuffers[i].release();
    }
    mSwapChainFramebuffers.clear();

    std::vector<vk::CommandBuffer> tempBuffers;
    for (std::size_t i{0}; i < mCommandBuffers.size(); ++i)
    {
        tempBuffers.push_back(*mCommandBuffers[i]);
        mCommandBuffers[i].release();
    }
    mCommandBuffers.clear();

    mDevice->freeCommandBuffers(*mCommandPool, tempBuffers);

    mDevice->destroyPipeline(*mGraphicsPipeline);
    mGraphicsPipeline.release();
    mDevice->destroyPipelineLayout(*mPipelineLayout);
    mPipelineLayout.release();
    mDevice->destroyRenderPass(*mRenderPass);
    mRenderPass.release();

    for (std::size_t i{0}; i < mSwapChainImageViews.size(); ++i)
    {
        mDevice->destroyImageView(*mSwapChainImageViews[i]);
        mSwapChainImageViews[i].release();
    }
    mSwapChainImageViews.clear();

    mDevice->destroySwapchainKHR(*mSwapChain);
    mSwapChain.release();
}
