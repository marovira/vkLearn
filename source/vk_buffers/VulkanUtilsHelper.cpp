#include "VulkanUtilsHelper.hpp"
#include "Paths.hpp"

#include <fmt/printf.h>
#include <fstream>

namespace globals
{
    PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;
} // namespace globals

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

namespace vkutils
{
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

    std::vector<const char*> getRequiredExtensions()
    {
        std::uint32_t glfwExtensionCount{0};
        const char** glfwExtensionNames =
            glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(
            glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);

        if constexpr (globals::isDebugBuild)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // Make sure that all the required extensions are present.
        if (!validateProperties<vk::ExtensionProperties>(
                extensions, vk::enumerateInstanceExtensionProperties(),
                compareExtensions))
        {
            throw std::runtime_error{
                "error: there are missing required extensions."};
        }

        return extensions;
    }

    void loadDebugFunctions(vk::Instance const& instance)
    {
        globals::pfnVkCreateDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
        if (!globals::pfnVkCreateDebugUtilsMessengerEXT)
        {
            throw std::runtime_error{
                "error: unable to find function to create debug messenger."};
        }

        globals::pfnVkDestroyDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
        if (!globals::pfnVkDestroyDebugUtilsMessengerEXT)
        {
            throw std::runtime_error{
                "error: unable to find function to destroy debug messenger."};
        }
    }

    vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo()
    {
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags{
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError};
        vk::DebugUtilsMessageTypeFlagsEXT typeFlags{
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation};

        vk::DebugUtilsMessengerCreateInfoEXT createInfo;
        createInfo.messageSeverity = severityFlags;
        createInfo.messageType     = typeFlags;
        createInfo.pfnUserCallback = &debugCallback;
        createInfo.pUserData       = nullptr;
        return createInfo;
    }

    bool isDeviceSuitable(vk::PhysicalDevice const& device,
                          vk::SurfaceKHR const& surface)
    {
        auto deviceProperties = device.getProperties();
        auto deviceFeatures   = device.getFeatures();
        auto indices          = findQueueFamilies(device, surface);

        bool isDeviceValid = deviceProperties.deviceType ==
                                 vk::PhysicalDeviceType::eDiscreteGpu ||
                             deviceProperties.deviceType ==
                                 vk::PhysicalDeviceType::eIntegratedGpu;
        bool areExtensionsSupported = checkDeviceExtensionSupport(device);

        bool isSwapchainAdequate{false};
        if (areExtensionsSupported)
        {
            auto swapchainDetails = querySwapchainSupport(device, surface);
            isSwapchainAdequate   = !swapchainDetails.formats.empty() &&
                                  !swapchainDetails.presentModes.empty();
        }

        return isDeviceValid && indices.isComplete() &&
               areExtensionsSupported && isSwapchainAdequate;
    }

    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice const& device,
                                         vk::SurfaceKHR const& surface)
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

            vk::Bool32 presetSupport = device.getSurfaceSupportKHR(i, surface);
            if (queueFamily.queueCount > 0 && presetSupport)
            {
                indices.presentFamily = i;
            }
            ++i;
        }

        return indices;
    }

    bool checkDeviceExtensionSupport(vk::PhysicalDevice const& device)
    {
        std::vector<vk::ExtensionProperties> availableExtensions =
            device.enumerateDeviceExtensionProperties();
        return validateProperties<vk::ExtensionProperties>(
            globals::deviceExtensions, availableExtensions, compareExtensions);
    }

    SwapchainSupportDetails
    querySwapchainSupport(vk::PhysicalDevice const& device,
                          vk::SurfaceKHR const& surface)
    {
        SwapchainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats      = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);

        return details;
    }

    vk::UniqueSwapchainKHR
    createSwapchainKHR(vk::PhysicalDevice const& physicalDevice,
                       vk::SurfaceKHR const& surface, vk::Device const& device,
                       GLFWwindow* window)
    {
        SwapchainSupportDetails swapchainDetails =
            querySwapchainSupport(physicalDevice, surface);

        auto surfaceFormat = chooseSwapSurfaceFormat(swapchainDetails.formats);
        auto presentMode = chooseSwapPresentMode(swapchainDetails.presentModes);
        auto extent = chooseSwapExtent(swapchainDetails.capabilities, window);

        std::uint32_t imageCount =
            swapchainDetails.capabilities.minImageCount + 1;
        if (swapchainDetails.capabilities.maxImageCount > 0 &&
            imageCount > swapchainDetails.capabilities.maxImageCount)
        {
            imageCount = swapchainDetails.capabilities.maxImageCount;
        }

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
        std::array<std::uint32_t, 2> queueFamilies{*indices.graphicsFamily,
                                                   *indices.presentFamily};

        vk::SharingMode sharingMode;
        std::uint32_t queueFamilyIndexCount{0};
        std::uint32_t const* queueFamilyIndices{nullptr};
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
        createInfo.surface          = surface;
        createInfo.minImageCount    = imageCount;
        createInfo.imageFormat      = surfaceFormat.format;
        createInfo.imageColorSpace  = surfaceFormat.colorSpace;
        createInfo.imageExtent      = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
        createInfo.imageSharingMode = sharingMode;
        createInfo.queueFamilyIndexCount = queueFamilyIndexCount;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        createInfo.preTransform   = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode    = presentMode;
        createInfo.clipped        = true;

        return device.createSwapchainKHRUnique(createInfo);
    }

    vk::SurfaceFormatKHR
    chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& formats)
    {
        for (auto const& format : formats)
        {
            if (format.format == vk::Format::eB8G8R8A8Unorm &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return format;
            }
        }

        return formats[0];
    }

    vk::PresentModeKHR
    chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& presentModes)
    {
        for (auto const& presentMode : presentModes)
        {
            if (presentMode == vk::PresentModeKHR::eMailbox)
            {
                return presentMode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D
    chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities,
                     GLFWwindow* window)
    {
        if (capabilities.currentExtent.width !=
            std::numeric_limits<std::uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            vk::Extent2D actualExtent = {static_cast<std::uint32_t>(width),
                                         static_cast<std::uint32_t>(height)};

            actualExtent.width =
                std::max(capabilities.minImageExtent.width,
                         std::min(capabilities.maxImageExtent.width,
                                  actualExtent.width));
            actualExtent.height =
                std::max(capabilities.minImageExtent.height,
                         std::min(capabilities.maxImageExtent.height,
                                  actualExtent.height));
            return actualExtent;
        }
    }

    std::vector<vk::UniqueImageView>
    createImageViews(vk::Device const& device,
                     std::vector<vk::Image> const& images,
                     vk::Format const& format)
    {
        std::vector<vk::UniqueImageView> imageViews(images.size());

        for (std::size_t i{0}; i < images.size(); ++i)
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
            createInfo.image            = images[i];
            createInfo.viewType         = vk::ImageViewType::e2D;
            createInfo.format           = format;
            createInfo.components       = components;
            createInfo.subresourceRange = subresourceRange;

            imageViews[i] = device.createImageViewUnique(createInfo);
        }

        return imageViews;
    }

    vk::UniqueRenderPass createRenderPass(vk::Device const& device,
                                          vk::Format const& format)
    {
        vk::AttachmentDescription colourAttachment;
        colourAttachment.format         = format;
        colourAttachment.samples        = vk::SampleCountFlagBits::e1;
        colourAttachment.loadOp         = vk::AttachmentLoadOp::eClear;
        colourAttachment.storeOp        = vk::AttachmentStoreOp::eStore;
        colourAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        colourAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colourAttachment.initialLayout  = vk::ImageLayout::eUndefined;
        colourAttachment.finalLayout    = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference colourAttachmentRef;
        colourAttachmentRef.attachment = 0;
        colourAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subPass;
        subPass.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;
        subPass.inputAttachmentCount = 0;
        subPass.pInputAttachments    = nullptr;
        subPass.colorAttachmentCount = 1;
        subPass.pColorAttachments    = &colourAttachmentRef;

        vk::SubpassDependency dependency;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask =
            vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstStageMask =
            vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                                   vk::AccessFlagBits::eColorAttachmentWrite;

        vk::RenderPassCreateInfo createInfo;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments    = &colourAttachment;
        createInfo.subpassCount    = 1;
        createInfo.pSubpasses      = &subPass;
        createInfo.dependencyCount = 1;
        createInfo.pDependencies   = &dependency;

        return device.createRenderPassUnique(createInfo);
    }

    std::vector<char> readFile(std::string const& filename)
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

    Pipeline createPipeline(vk::Device const& device,
                            vk::Extent2D const& extent,
                            vk::RenderPass const& renderPass)
    {
        std::string root{ShaderPath};
        auto vertexShaderCode   = readFile(root + "triangle.vert.spv");
        auto fragmentShaderCode = readFile(root + "triangle.frag.spv");

        vk::UniqueShaderModule vertModule =
            createShaderModule(device, vertexShaderCode);
        vk::UniqueShaderModule fragModule =
            createShaderModule(device, fragmentShaderCode);

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

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = false;

        vk::Viewport viewport;
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(extent.width);
        viewport.height   = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor;
        scissor.offset = {0, 0};
        scissor.extent = extent;

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

        std::array<vk::DynamicState, 2> dynamicStates{
            vk::DynamicState::eViewport, vk::DynamicState::eLineWidth};
        vk::PipelineDynamicStateCreateInfo dynamicState;
        dynamicState.dynamicStateCount =
            static_cast<std::uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        vk::PipelineLayout pipelineLayout =
            device.createPipelineLayout(pipelineLayoutCreateInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo;
        pipelineInfo.stageCount =
            static_cast<std::uint32_t>(shaderStages.size());
        pipelineInfo.pStages             = shaderStages.data();
        pipelineInfo.pVertexInputState   = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pColorBlendState    = &colourBlending;
        pipelineInfo.layout              = pipelineLayout;
        pipelineInfo.renderPass          = renderPass;
        pipelineInfo.subpass             = 0;
        pipelineInfo.basePipelineIndex   = -1;

        vk::Pipeline graphicsPipeline =
            device.createGraphicsPipeline({}, pipelineInfo);

        Pipeline p;
        p.layout = pipelineLayout;

        return {pipelineLayout, graphicsPipeline};
    }

    vk::UniqueShaderModule createShaderModule(vk::Device const& device,
                                              std::vector<char> const& code)
    {
        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<std::uint32_t const*>(code.data());
        return device.createShaderModuleUnique(createInfo);
    }

    std::vector<vk::UniqueFramebuffer>
    createFramebuffers(vk::Device const& device,
                       std::vector<vk::UniqueImageView> const& imageViews,
                       vk::RenderPass const& renderPass,
                       vk::Extent2D const& extent)
    {
        std::vector<vk::UniqueFramebuffer> framebuffers(imageViews.size());
        for (std::size_t i{0}; i < imageViews.size(); ++i)
        {
            std::array<vk::ImageView, 1> attachments{*imageViews[i]};

            vk::FramebufferCreateInfo createInfo;
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount =
                static_cast<std::uint32_t>(attachments.size());
            createInfo.pAttachments = attachments.data();
            createInfo.width        = extent.width;
            createInfo.height       = extent.height;
            createInfo.layers       = 1;

            framebuffers[i] = device.createFramebufferUnique(createInfo);
        }

        return framebuffers;
    }

    vk::UniqueCommandPool
    createCommandPool(vk::Device const& device,
                      vk::PhysicalDevice const& physicalDevice,
                      vk::SurfaceKHR const& surface)
    {
        auto indices = findQueueFamilies(physicalDevice, surface);
        vk::CommandPoolCreateInfo createInfo;
        createInfo.queueFamilyIndex = *indices.graphicsFamily;
        return device.createCommandPoolUnique(createInfo);
    }

    std::vector<vk::UniqueCommandBuffer> createCommandBuffers(
        vk::Device const& device,
        std::vector<vk::UniqueFramebuffer> const& framebuffers,
        vk::CommandPool const& commandPool, vk::RenderPass const& renderPass,
        vk::Extent2D const& extent, vk::Pipeline const& pipeline)
    {
        std::vector<vk::UniqueCommandBuffer> commandBuffers(
            framebuffers.size());

        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = commandPool;
        allocInfo.level       = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount =
            static_cast<std::uint32_t>(commandBuffers.size());

        commandBuffers = device.allocateCommandBuffersUnique(allocInfo);

        for (std::size_t i{0}; i < commandBuffers.size(); ++i)
        {
            vk::CommandBufferBeginInfo beginInfo;
            commandBuffers[i]->begin(beginInfo);

            vk::ClearValue clearValue;
            clearValue.color = {std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
            vk::RenderPassBeginInfo renderPassInfo;
            renderPassInfo.renderPass      = renderPass;
            renderPassInfo.framebuffer     = *framebuffers[i];
            renderPassInfo.renderArea      = {{0, 0}, extent};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues    = &clearValue;

            // Begin the render pass.
            commandBuffers[i]->beginRenderPass(&renderPassInfo,
                                               vk::SubpassContents::eInline);
            commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics,
                                            pipeline);
            commandBuffers[i]->draw(3, 1, 0, 0);
            commandBuffers[i]->endRenderPass();
            commandBuffers[i]->end();
        }

        return commandBuffers;
    }
} // namespace vkutils
