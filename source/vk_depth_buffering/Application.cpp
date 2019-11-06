#include "Application.hpp"
#include "Paths.hpp"
#include "stb_image.h"

#include <algorithm>
#include <array>
#include <fmt/printf.h>
#include <fstream>
#include <functional>
#include <limits>
#include <numeric>
#include <set>

namespace globals
{
    static constexpr auto windowWidth{800};
    static constexpr auto windowHeight{600};
    static constexpr auto maxFramesInFlight{2};
    static const std::vector<const char*> validationLayers{
        "VK_LAYER_KHRONOS_validation"};
    static const std::vector<const char*> deviceExtensions{
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

    // clang-format off
    const std::vector<Vertex> vertices{
        {{-0.5f, -0.5f, 0.0f},    {1.0f, 0.0f, 0.0f},     {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f},    {0.0f, 1.0f, 0.0f},     {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f},    {0.0f, 0.0f, 1.0f},     {0.0f, 1.0f}},
        {{-0.5f,  0.5f, 0.0f},    {1.0f, 1.0f, 1.0f},     {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f},    {1.0f, 0.0f, 0.0f},     {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f},    {0.0f, 1.0f, 0.0f},     {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f},    {0.0f, 0.0f, 1.0f},     {0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f},    {1.0f, 1.0f, 1.0f},     {1.0f, 1.0f}}
    };

    const std::vector<std::uint16_t> indices{
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    // clang-format on
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

#if defined(_WIN32) || defined(_WIN64)
    __debugbreak();
#endif

    return false;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger)
{
    return globals::pfnVkCreateDebugUtilsMessengerEXT(
        instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                VkDebugUtilsMessengerEXT messenger,
                                const VkAllocationCallbacks* pAllocator)
{
    return globals::pfnVkDestroyDebugUtilsMessengerEXT(
        instance, messenger, pAllocator);
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

static void onFramebufferResize(GLFWwindow* window,
                                [[maybe_unused]] int width,
                                [[maybe_unused]] int height)
{
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->framebuffeResized();
}

static void onWindowRedraw(GLFWwindow* window)
{
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->redrawWindow();
}

bool compareLayers(std::string_view const& layerName,
                   vk::LayerProperties const& layer)
{
    std::string_view name{layer.layerName};
    return name == layerName;
}

bool compareExtensions(std::string_view const& extensionName,
                       vk::ExtensionProperties const& extension)
{
    std::string_view name{extension.extensionName};
    return name == extensionName;
}

vk::VertexInputBindingDescription Vertex::getBindingDescription()
{
    vk::VertexInputBindingDescription bindingDescription;
    bindingDescription.binding   = 0;
    bindingDescription.stride    = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;

    return bindingDescription;
}

std::array<vk::VertexInputAttributeDescription, 3>
Vertex::getAttributeDescriptions()
{
    std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;

    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[0].offset   = offsetof(Vertex, pos);

    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset   = offsetof(Vertex, colour);

    attributeDescriptions[2].binding  = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format   = vk::Format::eR32G32Sfloat;
    attributeDescriptions[2].offset   = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

void Application::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Application::framebuffeResized()
{
    mFramebufferResized = true;
}

void Application::redrawWindow()
{
    if (mFramebufferResized)
    {
        drawFrame();
        drawFrame();
    }
}

void Application::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createDepthResources();
    createFramebuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void Application::mainLoop()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        drawFrame();
        glfwPollEvents();
    }

    mDevice->waitIdle();
}

void Application::cleanup()
{
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void Application::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mWindow = glfwCreateWindow(globals::windowWidth,
                               globals::windowHeight,
                               "Vulkan",
                               nullptr,
                               nullptr);

    glfwSetWindowUserPointer(mWindow, this);
    glfwSetFramebufferSizeCallback(mWindow, onFramebufferResize);
    glfwSetWindowRefreshCallback(mWindow, onWindowRedraw);
}

void Application::createInstance()
{
    vk::ApplicationInfo appInfo;
    appInfo.apiVersion = VK_API_VERSION_1_1;

    if constexpr (globals::enableValidationLayers)
    {
        if (!validateProperties<vk::LayerProperties>(
                globals::validationLayers,
                vk::enumerateInstanceLayerProperties(),
                compareLayers))
        {
            throw std::runtime_error{
                "error: there are missing required validation layers."};
        }
    }

    auto extensionNames = getRequiredExtensions();
    std::uint32_t layerCount =
        (globals::enableValidationLayers)
            ? static_cast<std::uint32_t>(globals::validationLayers.size())
            : 0;
    char const* const* layerNames = (globals::enableValidationLayers)
                                        ? globals::validationLayers.data()
                                        : nullptr;

    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo    = &appInfo;
    createInfo.enabledLayerCount   = layerCount;
    createInfo.ppEnabledLayerNames = layerNames;
    createInfo.enabledExtensionCount =
        static_cast<std::uint32_t>(extensionNames.size());
    createInfo.ppEnabledExtensionNames = extensionNames.data();

    auto debugCreateInfo = getDebugMessengerCreateInfo();
    if constexpr (globals::enableValidationLayers)
    {
        createInfo.setPNext(&debugCreateInfo);
    }

    mInstance = vk::createInstanceUnique(createInfo);
}

void Application::listExtensions()
{
    std::vector<vk::ExtensionProperties> extensions =
        vk::enumerateInstanceExtensionProperties();

    for (auto const& extension : extensions)
    {
        fmt::print("{}\n", extension.extensionName);
    }
}

std::vector<const char*> Application::getRequiredExtensions()
{
    std::uint32_t glfwExtensionCount{0};
    const char** glfwExtensionNames;
    glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(
        glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);

    if constexpr (globals::enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if (!validateProperties<vk::ExtensionProperties>(
            extensions,
            vk::enumerateInstanceExtensionProperties(),
            compareExtensions))
    {
        throw std::runtime_error{
            "error: there are missing required extensions."};
    }

    return extensions;
}

void Application::setupDebugMessenger()
{
    if (!globals::enableValidationLayers)
    {
        return;
    }

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

vk::DebugUtilsMessengerCreateInfoEXT Application::getDebugMessengerCreateInfo()
{
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags{
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError};
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags{
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation};

    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.messageSeverity = severityFlags;
    createInfo.messageType     = messageTypeFlags;
    createInfo.pfnUserCallback = &debugCallback;
    createInfo.pUserData       = nullptr;

    return createInfo;
}

void Application::pickPhysicalDevice()
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

bool Application::isDeviceSuitable(vk::PhysicalDevice const& device)
{
    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures     = device.getFeatures();
    auto indices                                  = findQueueFamilies(device);

    bool isDeviceValid =
        deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ||
        deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu;
    bool areExtensionsSupported = checkDeviceExtensionSupport(device);

    bool isSwapChainAdequate{false};
    if (areExtensionsSupported)
    {
        auto swapChainDetails = querySwapChainSupport(device);
        isSwapChainAdequate   = !swapChainDetails.formats.empty() &&
                              !swapChainDetails.presentModes.empty();
    }

    return isDeviceValid && indices.isComplete() && areExtensionsSupported &&
           isSwapChainAdequate && deviceFeatures.samplerAnisotropy;
}

QueueFamilyIndices
Application::findQueueFamilies(vk::PhysicalDevice const& device)
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

        vk::Bool32 presetSupport = device.getSurfaceSupportKHR(i, *mSurface);
        if (queueFamily.queueCount > 0 && presetSupport)
        {
            indices.presentFamily = i;
        }
        ++i;
    }

    return indices;
}

void Application::createLogicalDevice()
{
    auto indices = findQueueFamilies(mPhysicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<std::uint32_t> uniqueQueueFamilies{*indices.graphicsFamily,
                                                *indices.presentFamily};

    float queuePriority{1.0f};
    for (auto queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.samplerAnisotropy = true;
    std::uint32_t layerCount =
        (globals::enableValidationLayers)
            ? static_cast<std::uint32_t>(globals::validationLayers.size())
            : 0;
    char const* const* layerNames = (globals::enableValidationLayers)
                                        ? globals::validationLayers.data()
                                        : nullptr;

    vk::DeviceCreateInfo createInfo;
    createInfo.queueCreateInfoCount =
        static_cast<std::uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos   = queueCreateInfos.data();
    createInfo.enabledLayerCount   = layerCount;
    createInfo.ppEnabledLayerNames = layerNames;
    createInfo.enabledExtensionCount =
        static_cast<std::uint32_t>(globals::deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = globals::deviceExtensions.data();
    createInfo.pEnabledFeatures        = &deviceFeatures;

    mDevice        = mPhysicalDevice.createDeviceUnique(createInfo);
    mGraphicsQueue = mDevice->getQueue(*indices.graphicsFamily, 0);
    mPresentQueue  = mDevice->getQueue(*indices.presentFamily, 0);
}

void Application::createSurface()
{
    VkSurfaceKHR surface = static_cast<VkSurfaceKHR>(*mSurface);
    if (glfwCreateWindowSurface(*mInstance, mWindow, nullptr, &surface))
    {
        throw std::runtime_error{"error: could not create window surface."};
    }

    mSurface = vk::UniqueSurfaceKHR{surface, *mInstance};
}

bool Application::checkDeviceExtensionSupport(vk::PhysicalDevice const& device)
{
    return validateProperties<vk::ExtensionProperties>(
        globals::deviceExtensions,
        device.enumerateDeviceExtensionProperties(),
        compareExtensions);
}

SwapChainSupportDetails
Application::querySwapChainSupport(vk::PhysicalDevice const& device)
{
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(*mSurface);
    details.formats      = device.getSurfaceFormatsKHR(*mSurface);
    details.presentModes = device.getSurfacePresentModesKHR(*mSurface);

    return details;
}

vk::SurfaceFormatKHR Application::chooseSwapSurfaceFormat(
    std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
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

vk::PresentModeKHR Application::chooseSwapPresentMode(
    std::vector<vk::PresentModeKHR> const& availablePresentModes)
{
    for (auto const& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D
Application::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
{
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

void Application::createSwapChain()
{
    SwapChainSupportDetails swapChainDetails =
        querySwapChainSupport(mPhysicalDevice);

    vk::SurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(swapChainDetails.formats);
    vk::PresentModeKHR presentMode =
        chooseSwapPresentMode(swapChainDetails.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainDetails.capabilities);

    std::uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;
    if (swapChainDetails.capabilities.maxImageCount > 0 &&
        imageCount > swapChainDetails.capabilities.maxImageCount)
    {
        imageCount = swapChainDetails.capabilities.maxImageCount;
    }

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

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.surface               = *mSurface;
    createInfo.minImageCount         = imageCount;
    createInfo.imageFormat           = surfaceFormat.format;
    createInfo.imageColorSpace       = surfaceFormat.colorSpace;
    createInfo.imageExtent           = extent;
    createInfo.imageArrayLayers      = 1;
    createInfo.imageUsage            = vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.imageSharingMode      = sharingMode;
    createInfo.queueFamilyIndexCount = queueFamilyIndexCount;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    createInfo.preTransform   = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = true;

    mSwapchain            = mDevice->createSwapchainKHRUnique(createInfo);
    mSwapchainImages      = mDevice->getSwapchainImagesKHR(*mSwapchain);
    mSwapchainImageFormat = surfaceFormat.format;
    mSwapchainExtent      = extent;
}

void Application::createImageViews()
{
    mSwapchainImageViews.resize(mSwapchainImages.size());

    for (std::size_t i{0}; i < mSwapchainImages.size(); ++i)
    {
        auto view               = createImageView(mSwapchainImages[i],
                                    mSwapchainImageFormat,
                                    vk::ImageAspectFlagBits::eColor);
        mSwapchainImageViews[i] = vk::UniqueImageView(view, *mDevice);
    }
}

void Application::createGraphicsPipeline()
{
    std::string root{ShaderPath};
    auto vertexShaderCode   = readFile(root + "triangle.vert.spv");
    auto fragmentShaderCode = readFile(root + "triangle.frag.spv");

    vk::UniqueShaderModule vertModule = createShaderModule(vertexShaderCode);
    vk::UniqueShaderModule fragModule = createShaderModule(fragmentShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderInfo;
    vertShaderInfo.stage  = vk::ShaderStageFlagBits::eVertex;
    vertShaderInfo.module = *vertModule;
    vertShaderInfo.pName  = "main";

    vk::PipelineShaderStageCreateInfo fragShaderInfo;
    fragShaderInfo.stage  = vk::ShaderStageFlagBits::eFragment;
    fragShaderInfo.module = *fragModule;
    fragShaderInfo.pName  = "main";

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{
        vertShaderInfo, fragShaderInfo};

    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions    = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<std::uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology               = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = false;

    vk::Viewport viewport;
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(mSwapchainExtent.width);
    viewport.height   = static_cast<float>(mSwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = mSwapchainExtent;

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthTestEnable       = true;
    depthStencil.depthWriteEnable      = true;
    depthStencil.depthCompareOp        = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = false;
    depthStencil.stencilTestEnable     = false;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable        = false;
    rasterizer.rasterizerDiscardEnable = false;
    rasterizer.polygonMode             = vk::PolygonMode::eFill;
    rasterizer.cullMode                = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace               = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable         = false;
    rasterizer.lineWidth               = 1.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.sampleShadingEnable  = false;
    multisampling.minSampleShading     = 1.0f;

    vk::PipelineColorBlendAttachmentState colourBlendAttachment;
    colourBlendAttachment.blendEnable         = false;
    colourBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
    colourBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
    colourBlendAttachment.colorBlendOp        = vk::BlendOp::eAdd;
    colourBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colourBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colourBlendAttachment.alphaBlendOp        = vk::BlendOp::eAdd;
    colourBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colourBlending;
    colourBlending.logicOpEnable   = false;
    colourBlending.logicOp         = vk::LogicOp::eCopy;
    colourBlending.attachmentCount = 1;
    colourBlending.pAttachments    = &colourBlendAttachment;

    std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport,
                                                  vk::DynamicState::eLineWidth};
    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.dynamicStateCount =
        static_cast<std::uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts    = &(*mDescriptorSetLayout);
    mPipelineLayout =
        mDevice->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = static_cast<std::uint32_t>(shaderStages.size());
    pipelineInfo.pStages    = shaderStages.data();
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colourBlending;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.layout              = *mPipelineLayout;
    pipelineInfo.renderPass          = *mRenderPass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineIndex   = -1;

    mGraphicsPipeline = mDevice->createGraphicsPipelineUnique({}, pipelineInfo);
}

vk::UniqueShaderModule
Application::createShaderModule(std::vector<char> const& code)
{
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<std::uint32_t const*>(code.data());
    return mDevice->createShaderModuleUnique(createInfo);
}

void Application::createRenderPass()
{
    vk::AttachmentDescription colourAttachment;
    colourAttachment.format         = mSwapchainImageFormat;
    colourAttachment.samples        = vk::SampleCountFlagBits::e1;
    colourAttachment.loadOp         = vk::AttachmentLoadOp::eClear;
    colourAttachment.storeOp        = vk::AttachmentStoreOp::eStore;
    colourAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    colourAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colourAttachment.initialLayout  = vk::ImageLayout::eUndefined;
    colourAttachment.finalLayout    = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentDescription depthAttachment;
    depthAttachment.format         = findDepthFormat();
    depthAttachment.samples        = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp         = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp        = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout  = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout =
        vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference colourAttachmentRef;
    colourAttachmentRef.attachment = 0;
    colourAttachmentRef.layout     = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subPass;
    subPass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subPass.inputAttachmentCount    = 0;
    subPass.pInputAttachments       = nullptr;
    subPass.colorAttachmentCount    = 1;
    subPass.pColorAttachments       = &colourAttachmentRef;
    subPass.pDepthStencilAttachment = &depthAttachmentRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass   = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass   = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                               vk::AccessFlagBits::eColorAttachmentWrite;

    std::array<vk::AttachmentDescription, 2> attachments = {colourAttachment,
                                                            depthAttachment};
    vk::RenderPassCreateInfo createInfo;
    createInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    createInfo.pAttachments    = attachments.data();
    createInfo.subpassCount    = 1;
    createInfo.pSubpasses      = &subPass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies   = &dependency;

    mRenderPass = mDevice->createRenderPassUnique(createInfo);
}

void Application::createFramebuffers()
{
    mSwapchainFramebuffers.resize(mSwapchainImageViews.size());

    for (std::size_t i{0}; i < mSwapchainImageViews.size(); ++i)
    {
        std::array<vk::ImageView, 2> attachments{*mSwapchainImageViews[i],
                                                 *mDepthImageView};

        vk::FramebufferCreateInfo createInfo;
        createInfo.renderPass = *mRenderPass;
        createInfo.attachmentCount =
            static_cast<std::uint32_t>(attachments.size());
        createInfo.pAttachments = attachments.data();
        createInfo.width        = mSwapchainExtent.width;
        createInfo.height       = mSwapchainExtent.height;
        createInfo.layers       = 1;

        mSwapchainFramebuffers[i] =
            mDevice->createFramebufferUnique(createInfo);
    }
}

void Application::createCommandPool()
{
    auto indices = findQueueFamilies(mPhysicalDevice);
    vk::CommandPoolCreateInfo createInfo;
    createInfo.queueFamilyIndex = *indices.graphicsFamily;

    mCommandPool = mDevice->createCommandPoolUnique(createInfo);

    createInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    mBufferPool      = mDevice->createCommandPoolUnique(createInfo);
}

void Application::createCommandBuffers()
{
    mCommandBuffers.resize(mSwapchainFramebuffers.size());

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = *mCommandPool;
    allocInfo.level       = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount =
        static_cast<std::uint32_t>(mCommandBuffers.size());

    mCommandBuffers = mDevice->allocateCommandBuffersUnique(allocInfo);

    for (std::size_t i{0}; i < mCommandBuffers.size(); ++i)
    {
        vk::CommandBufferBeginInfo beginInfo;
        mCommandBuffers[i]->begin(beginInfo);

        std::array<vk::ClearValue, 2> clearValues;
        clearValues[0].color =
            vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass  = *mRenderPass;
        renderPassInfo.framebuffer = *mSwapchainFramebuffers[i];
        renderPassInfo.renderArea  = {{0, 0}, mSwapchainExtent};
        renderPassInfo.clearValueCount =
            static_cast<std::uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        mCommandBuffers[i]->beginRenderPass(&renderPassInfo,
                                            vk::SubpassContents::eInline);

        mCommandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics,
                                         *mGraphicsPipeline);
        std::array<vk::Buffer, 1> vertexBuffers{*mVertexBuffer};
        std::array<vk::DeviceSize, 1> offsets{0};

        mCommandBuffers[i]->bindVertexBuffers(0, vertexBuffers, offsets);
        mCommandBuffers[i]->bindIndexBuffer(
            *mIndexBuffer, 0, vk::IndexType::eUint16);
        mCommandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                               *mPipelineLayout,
                                               0,
                                               {*mDescriptorSets[i]},
                                               {});
        mCommandBuffers[i]->drawIndexed(
            static_cast<std::uint32_t>(globals::indices.size()), 1, 0, 0, 0);
        mCommandBuffers[i]->endRenderPass();
        mCommandBuffers[i]->end();
    }
}

void Application::createSyncObjects()
{
    mImageAvailableSemaphores.resize(globals::maxFramesInFlight);
    mRenderFinishedSemaphores.resize(globals::maxFramesInFlight);
    mInFlightFences.resize(globals::maxFramesInFlight);

    vk::SemaphoreCreateInfo semaphoreInfo{};
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

void Application::drawFrame()
{
    mDevice->waitForFences({*mInFlightFences[mCurrentFrame]},
                           VK_TRUE,
                           std::numeric_limits<std::uint64_t>::max());

    auto result =
        mDevice->acquireNextImageKHR(*mSwapchain,
                                     std::numeric_limits<std::uint32_t>::max(),
                                     *mImageAvailableSemaphores[mCurrentFrame],
                                     {});

    if (result.result == vk::Result::eErrorOutOfDateKHR ||
        result.result == vk::Result::eSuboptimalKHR || mFramebufferResized)
    {
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

    std::uint32_t imageIndex = result.value;

    updateUniformBuffer(imageIndex);

    std::array<vk::Semaphore, 1> waitSemaphores{
        *mImageAvailableSemaphores[mCurrentFrame]};
    std::array<vk::PipelineStageFlags, 1> waitStages{
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    std::array<vk::Semaphore, 1> signalSemaphores{
        *mRenderFinishedSemaphores[mCurrentFrame]};

    vk::SubmitInfo submitInfo;
    submitInfo.waitSemaphoreCount =
        static_cast<std::uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores    = waitSemaphores.data();
    submitInfo.pWaitDstStageMask  = waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &(*mCommandBuffers[imageIndex]);
    submitInfo.signalSemaphoreCount =
        static_cast<std::uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    mDevice->resetFences({*mInFlightFences[mCurrentFrame]});

    mGraphicsQueue.submit({submitInfo}, *mInFlightFences[mCurrentFrame]);

    std::array<vk::SwapchainKHR, 1> swapchains{*mSwapchain};

    // The last step is to submit the resulting render back into the swap chain
    // so that it can be shown on the screen.
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount =
        static_cast<std::uint32_t>(signalSemaphores.size());
    presentInfo.pWaitSemaphores = signalSemaphores.data();
    presentInfo.swapchainCount  = static_cast<std::uint32_t>(swapchains.size());
    presentInfo.pSwapchains     = swapchains.data();
    presentInfo.pImageIndices   = &imageIndex;

    mPresentQueue.presentKHR(presentInfo);
    mCurrentFrame = (mCurrentFrame + 1) % globals::maxFramesInFlight;
}

void Application::recreateSwapChain()
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
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}

void Application::cleanupSwapChain()
{
    mDevice->destroyImageView(*mDepthImageView);
    mDepthImageView.release();
    mDevice->destroyImage(*mDepthImage);
    mDepthImage.release();
    mDevice->freeMemory(*mDepthImageMemory);
    mDepthImageMemory.release();

    for (std::size_t i{0}; i < mSwapchainFramebuffers.size(); ++i)
    {
        mDevice->destroyFramebuffer(*mSwapchainFramebuffers[i]);
        mSwapchainFramebuffers[i].release();
    }
    mSwapchainFramebuffers.clear();

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

    for (std::size_t i{0}; i < mSwapchainImageViews.size(); ++i)
    {
        mDevice->destroyImageView(*mSwapchainImageViews[i]);
        mSwapchainImageViews[i].release();
    }
    mSwapchainImageViews.clear();

    mDevice->destroySwapchainKHR(*mSwapchain);
    mSwapchain.release();

    mUniformBuffers.clear();
    mUniformBuffersMemory.clear();

    mDescriptorSets.clear();
    mDevice->destroyDescriptorPool(*mDescriptorPool);
    mDescriptorPool.release();
}

void Application::createVertexBuffer()
{
    vk::DeviceSize bufferSize =
        sizeof(globals::vertices[0]) * globals::vertices.size();

    // Create staging buffer.
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize,
                 vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer,
                 stagingBufferMemory);

    // Map and copy the data over.
    void* data;
    mDevice->mapMemory(stagingBufferMemory, 0, bufferSize, {}, &data);
    memcpy(
        data, globals::vertices.data(), static_cast<std::size_t>(bufferSize));
    mDevice->unmapMemory(stagingBufferMemory);

    // Now create the vertex buffer.
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexMemory;
    createBuffer(bufferSize,
                 vk::BufferUsageFlagBits::eTransferDst |
                     vk::BufferUsageFlagBits::eVertexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 vertexBuffer,
                 vertexMemory);
    mVertexBuffer       = vk::UniqueBuffer(vertexBuffer, *mDevice);
    mVertexBufferMemory = vk::UniqueDeviceMemory(vertexMemory, *mDevice);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // The staging buffer data is not in unique handles, so we have to release
    // them manually.
    mDevice->destroyBuffer(stagingBuffer);
    mDevice->freeMemory(stagingBufferMemory);
}

std::uint32_t
Application::findMemoryType(std::uint32_t typeFilter,
                            vk::MemoryPropertyFlags const& properties)
{
    auto memProperties = mPhysicalDevice.getMemoryProperties();
    for (std::uint32_t i{0}; i < memProperties.memoryTypeCount; ++i)
    {
        if (typeFilter & (1 << i) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties)
        {
            return i;
        }
    }

    throw std::runtime_error{"error: failed to find suitable memory type."};
}

void Application::createBuffer(vk::DeviceSize const& size,
                               vk::BufferUsageFlags const& usage,
                               vk::MemoryPropertyFlags const& properties,
                               vk::Buffer& buffer,
                               vk::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    buffer = mDevice->createBuffer(bufferInfo);

    auto memRequirements = mDevice->getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits, properties);

    bufferMemory = mDevice->allocateMemory(allocInfo);
    mDevice->bindBufferMemory(buffer, bufferMemory, 0);
}

void Application::copyBuffer(vk::Buffer const& srcBuffer,
                             vk::Buffer const& dstBuffer,
                             vk::DeviceSize const& size)
{
    auto commandBuffer = beginSingleTimeCommands();

    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size      = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void Application::createIndexBuffer()
{
    vk::DeviceSize bufferSize =
        sizeof(globals::indices[0]) * globals::indices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize,
                 vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer,
                 stagingBufferMemory);

    void* data;
    mDevice->mapMemory(stagingBufferMemory, 0, bufferSize, {}, &data);
    memcpy(data, globals::indices.data(), static_cast<std::size_t>(bufferSize));
    mDevice->unmapMemory(stagingBufferMemory);

    vk::Buffer indexBuffer;
    vk::DeviceMemory indexMemory;
    createBuffer(bufferSize,
                 vk::BufferUsageFlagBits::eTransferDst |
                     vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 indexBuffer,
                 indexMemory);
    mIndexBuffer       = vk::UniqueBuffer(indexBuffer, *mDevice);
    mIndexBufferMemory = vk::UniqueDeviceMemory(indexMemory, *mDevice);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    mDevice->destroyBuffer(stagingBuffer);
    mDevice->freeMemory(stagingBufferMemory);
}

void Application::createDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.binding         = 0;
    uboLayoutBinding.descriptorType  = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags      = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.binding         = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType =
        vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
        uboLayoutBinding, samplerLayoutBinding};
    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
    createInfo.pBindings    = bindings.data();

    mDescriptorSetLayout = mDevice->createDescriptorSetLayoutUnique(createInfo);
}

void Application::createUniformBuffers()
{
    vk::DeviceSize bufferSize = sizeof(UniformMatrices);

    mUniformBuffers.resize(mSwapchainImages.size());
    mUniformBuffersMemory.resize(mSwapchainImages.size());

    for (std::size_t i{0}; i < mSwapchainImages.size(); ++i)
    {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible |
                         vk::MemoryPropertyFlagBits::eHostCoherent,
                     buffer,
                     memory);
        mUniformBuffers[i]       = vk::UniqueBuffer(buffer, *mDevice);
        mUniformBuffersMemory[i] = vk::UniqueDeviceMemory(memory, *mDevice);
    }
}

void Application::updateUniformBuffer(std::uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    UniformMatrices ubo;
    ubo.model = glm::rotate(glm::mat4{1.0f},
                            time * glm::radians(90.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view  = glm::lookAt(
        glm::vec3(2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.projection = glm::perspective(
        glm::radians(45.0f),
        mSwapchainExtent.width / static_cast<float>(mSwapchainExtent.height),
        0.1f,
        10.0f);
    ubo.projection[1][1] *= -1;

    void* data;
    mDevice->mapMemory(
        *mUniformBuffersMemory[currentImage], 0, sizeof(ubo), {}, &data);
    memcpy(data, &ubo, sizeof(ubo));
    mDevice->unmapMemory(*mUniformBuffersMemory[currentImage]);
}

void Application::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes;
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount =
        static_cast<std::uint32_t>(mSwapchainImages.size());
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount =
        static_cast<std::uint32_t>(mSwapchainImages.size());

    vk::DescriptorPoolCreateInfo createInfo;
    createInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    createInfo.pPoolSizes    = poolSizes.data();
    createInfo.maxSets = static_cast<std::uint32_t>(mSwapchainImages.size());

    // Because of the way we are freeing the descriptor pool, this flag needs
    // to be added to prevent the validation layers from issuing an error.
    createInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    mDescriptorPool = mDevice->createDescriptorPoolUnique(createInfo);
}

void Application::createDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(mSwapchainImages.size(),
                                                 *mDescriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool     = *mDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<std::uint32_t>(layouts.size());
    allocInfo.pSetLayouts        = layouts.data();

    mDescriptorSets = mDevice->allocateDescriptorSetsUnique(allocInfo);

    for (std::size_t i{0}; i < mSwapchainImages.size(); ++i)
    {
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = *mUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformMatrices);

        vk::DescriptorImageInfo imageInfo;
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView   = *mTextureImageView;
        imageInfo.sampler     = *mTextureSampler;

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites;

        descriptorWrites[0].dstSet          = *mDescriptorSets[i];
        descriptorWrites[0].dstBinding      = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo     = &bufferInfo;

        descriptorWrites[1].dstSet          = *mDescriptorSets[i];
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType =
            vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo      = &imageInfo;

        mDevice->updateDescriptorSets(descriptorWrites, {});
    }
}

void Application::createTextureImage()
{
    int texWidth, texHeight, texChannels;
    std::string root{ImagePath};
    std::string filename{root + "texture.jpg"};
    stbi_uc* pixels = stbi_load(
        filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error{"error: failed to load texture image."};
    }

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingMemory;

    createBuffer(imageSize,
                 vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer,
                 stagingMemory);

    void* data;
    mDevice->mapMemory(stagingMemory, 0, imageSize, {}, &data);
    memcpy(data, pixels, static_cast<std::size_t>(imageSize));
    mDevice->unmapMemory(stagingMemory);
    stbi_image_free(pixels);

    vk::Image image;
    vk::DeviceMemory imageMemory;
    createImage(texWidth,
                texHeight,
                vk::Format::eR8G8B8A8Unorm,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransferDst |
                    vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                image,
                imageMemory);

    mTextureImage       = vk::UniqueImage(image, *mDevice);
    mTextureImageMemory = vk::UniqueDeviceMemory(imageMemory, *mDevice);

    transitionImageLayout(image,
                          vk::Format::eR8G8B8A8Unorm,
                          vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(stagingBuffer, image, texWidth, texHeight);
    transitionImageLayout(image,
                          vk::Format::eR8G8B8A8Unorm,
                          vk::ImageLayout::eTransferDstOptimal,
                          vk::ImageLayout::eShaderReadOnlyOptimal);

    mDevice->destroyBuffer(stagingBuffer);
    mDevice->freeMemory(stagingMemory);
}

void Application::createImage(std::uint32_t width,
                              std::uint32_t height,
                              vk::Format const& format,
                              vk::ImageTiling const& tiling,
                              vk::ImageUsageFlags const& usage,
                              vk::MemoryPropertyFlags const& properties,
                              vk::Image& image,
                              vk::DeviceMemory& imageMemory)
{
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType     = vk::ImageType::e2D;
    imageInfo.extent        = {width, height, 1};
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage         = usage;
    imageInfo.sharingMode   = vk::SharingMode::eExclusive;
    imageInfo.samples       = vk::SampleCountFlagBits::e1;

    image = mDevice->createImage(imageInfo);

    auto memRequirements = mDevice->getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits, properties);
    imageMemory = mDevice->allocateMemory(allocInfo);
    mDevice->bindImageMemory(image, imageMemory, 0);
}

vk::CommandBuffer Application::beginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level              = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool        = *mBufferPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer =
        mDevice->allocateCommandBuffers(allocInfo).front();

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void Application::endSingleTimeCommands(vk::CommandBuffer const& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    mGraphicsQueue.submit({submitInfo}, {});
    mGraphicsQueue.waitIdle();

    mDevice->freeCommandBuffers(*mBufferPool, commandBuffer);
}

void Application::transitionImageLayout(
    vk::Image const& image,
    [[maybe_unused]] vk::Format const& format,
    vk::ImageLayout const& oldLayout,
    vk::ImageLayout const& newLayout)
{
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    auto commandBuffer = beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    if (oldLayout == vk::ImageLayout::eUndefined &&
        newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage      = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined &&
             newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask =
            vk::AccessFlagBits::eDepthStencilAttachmentRead |
            vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    }
    else
    {
        throw std::runtime_error{"error: unsupported layout transition."};
    }

    commandBuffer.pipelineBarrier(
        sourceStage, destinationStage, {}, {}, {}, {barrier});

    endSingleTimeCommands(commandBuffer);
}

void Application::copyBufferToImage(vk::Buffer const& buffer,
                                    vk::Image const& image,
                                    std::uint32_t width,
                                    std::uint32_t height)
{
    auto commandBuffer = beginSingleTimeCommands();

    vk::BufferImageCopy region;
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    commandBuffer.copyBufferToImage(
        buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});

    endSingleTimeCommands(commandBuffer);
}

void Application::createTextureImageView()
{
    auto view         = createImageView(*mTextureImage,
                                vk::Format::eR8G8B8A8Unorm,
                                vk::ImageAspectFlagBits::eColor);
    mTextureImageView = vk::UniqueImageView(view, *mDevice);
}

vk::ImageView
Application::createImageView(vk::Image const& image,
                             vk::Format const& format,
                             vk::ImageAspectFlags const& aspectFlags)
{
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image                           = image;
    viewInfo.viewType                        = vk::ImageViewType::e2D;
    viewInfo.format                          = format;
    viewInfo.subresourceRange.aspectMask     = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    return mDevice->createImageView(viewInfo);
}

void Application::createTextureSampler()
{
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter               = vk::Filter::eLinear;
    samplerInfo.minFilter               = vk::Filter::eLinear;
    samplerInfo.addressModeU            = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV            = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW            = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable        = true;
    samplerInfo.maxAnisotropy           = 16;
    samplerInfo.borderColor             = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = false;
    samplerInfo.compareEnable           = false;
    samplerInfo.compareOp               = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode              = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias              = 0.0f;
    samplerInfo.minLod                  = 0.0f;
    samplerInfo.maxLod                  = 0.0f;

    mTextureSampler = mDevice->createSamplerUnique(samplerInfo);
}

void Application::createDepthResources()
{
    auto depthFormat = findDepthFormat();

    vk::Image image;
    vk::DeviceMemory imageMemory;
    createImage(mSwapchainExtent.width,
                mSwapchainExtent.height,
                depthFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                image,
                imageMemory);
    auto imageView =
        createImageView(image, depthFormat, vk::ImageAspectFlagBits::eDepth);

    mDepthImage       = vk::UniqueImage(image, *mDevice);
    mDepthImageMemory = vk::UniqueDeviceMemory(imageMemory, *mDevice);
    mDepthImageView   = vk::UniqueImageView(imageView, *mDevice);
}

vk::Format
Application::findSupportedFormat(std::vector<vk::Format> const& candidates,
                                 vk::ImageTiling const& tiling,
                                 vk::FormatFeatureFlags const& features)
{
    for (auto format : candidates)
    {
        auto props = mPhysicalDevice.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear &&
            (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal &&
                 (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error{"error: failed to find supported format."};
}

vk::Format Application::findDepthFormat()
{
    return findSupportedFormat(
        {vk::Format::eD32Sfloat,
         vk::Format::eD32SfloatS8Uint,
         vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool Application::hasStencilComponent(vk::Format const& format)
{
    return format == vk::Format::eD32SfloatS8Uint ||
           format == vk::Format::eD24UnormS8Uint;
}
