#include "Application.hpp"
#include "Paths.hpp"

#include <algorithm>
#include <array>
#include <fmt/printf.h>
#include <fstream>
#include <functional>
#include <limits>
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
        {{ 0.0f, -0.5f},    {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f},    {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f},    {0.0f, 0.0f, 1.0f}}};
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

std::array<vk::VertexInputAttributeDescription, 2>
Vertex::getAttributeDescriptions()
{
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;

    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = vk::Format::eR32G32Sfloat;
    attributeDescriptions[0].offset   = offsetof(Vertex, pos);

    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset   = offsetof(Vertex, pos);

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
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
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
    mWindow = glfwCreateWindow(globals::windowWidth, globals::windowHeight,
                               "Vulkan", nullptr, nullptr);

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
                vk::enumerateInstanceLayerProperties(), compareLayers))
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
            extensions, vk::enumerateInstanceExtensionProperties(),
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
           isSwapChainAdequate;
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
        globals::deviceExtensions, device.enumerateDeviceExtensionProperties(),
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
        vk::ComponentMapping components;
        components.r = vk::ComponentSwizzle::eIdentity;
        components.g = vk::ComponentSwizzle::eIdentity;
        components.b = vk::ComponentSwizzle::eIdentity;
        components.a = vk::ComponentSwizzle::eIdentity;

        vk::ImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
        subresourceRange.baseMipLevel   = 0;
        subresourceRange.levelCount     = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount     = 1;

        vk::ImageViewCreateInfo createInfo;
        createInfo.image            = mSwapchainImages[i];
        createInfo.viewType         = vk::ImageViewType::e2D;
        createInfo.format           = mSwapchainImageFormat;
        createInfo.components       = components;
        createInfo.subresourceRange = subresourceRange;

        mSwapchainImageViews[i] = mDevice->createImageViewUnique(createInfo);
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
    rasterizer.frontFace               = vk::FrontFace::eClockwise;
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

    vk::AttachmentReference colourAttachmentRef;
    colourAttachmentRef.attachment = 0;
    colourAttachmentRef.layout     = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subPass;
    subPass.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;
    subPass.inputAttachmentCount = 0;
    subPass.pInputAttachments    = nullptr;
    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments    = &colourAttachmentRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass   = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass   = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                               vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo createInfo;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments    = &colourAttachment;
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
        std::array<vk::ImageView, 1> attachments{*mSwapchainImageViews[i]};

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

        // Next comes the equivalent of glClearColor. We specify the colour
        // value, along with the information about the dimensions of the
        // framebuffers, and the renderpass.
        vk::ClearValue clearValue;
        clearValue.color =
            vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass      = *mRenderPass;
        renderPassInfo.framebuffer     = *mSwapchainFramebuffers[i];
        renderPassInfo.renderArea      = {{0, 0}, mSwapchainExtent};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues    = &clearValue;

        mCommandBuffers[i]->beginRenderPass(&renderPassInfo,
                                            vk::SubpassContents::eInline);

        mCommandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics,
                                         *mGraphicsPipeline);
        std::array<vk::Buffer, 1> vertexBuffers{*mVertexBuffer};
        std::array<vk::DeviceSize, 1> offsets{0};

        mCommandBuffers[i]->bindVertexBuffers(0, vertexBuffers, offsets);
        mCommandBuffers[i]->draw(
            static_cast<std::uint32_t>(globals::vertices.size()), 1, 0, 0);
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
    mDevice->waitForFences({*mInFlightFences[mCurrentFrame]}, VK_TRUE,
                           std::numeric_limits<std::uint64_t>::max());

    auto result = mDevice->acquireNextImageKHR(
        *mSwapchain, std::numeric_limits<std::uint32_t>::max(),
        *mImageAvailableSemaphores[mCurrentFrame], {});

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
    createFramebuffers();
    createCommandBuffers();
}

void Application::cleanupSwapChain()
{
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
}

void Application::createVertexBuffer()
{
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size  = sizeof(globals::vertices[0]) * globals::vertices.size();
    bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    mVertexBuffer = mDevice->createBufferUnique(bufferInfo);

    auto memRequirements = mDevice->getBufferMemoryRequirements(*mVertexBuffer);

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits,
                       vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent);

    mVertexBufferMemory = mDevice->allocateMemoryUnique(allocInfo);
    mDevice->bindBufferMemory(*mVertexBuffer, *mVertexBufferMemory, 0);

    void* data;
    mDevice->mapMemory(*mVertexBufferMemory, 0, bufferInfo.size, {}, &data);
    memcpy(data, globals::vertices.data(),
           static_cast<std::size_t>(bufferInfo.size));
    mDevice->unmapMemory(*mVertexBufferMemory);
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
