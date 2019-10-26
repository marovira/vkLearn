#include "VulkanUtils.hpp"
#include "Paths.hpp"
#include "VulkanUtilsHelper.hpp"

#include <array>
#include <fmt/printf.h>
#include <fstream>
#include <set>

namespace vkutils
{
    void listAvailableInstanceExtensions()
    {
        std::vector<vk::ExtensionProperties> extensions =
            vk::enumerateInstanceExtensionProperties();

        for (auto const& extension : extensions)
        {
            fmt::print("{}\n", extension.extensionName);
        }
    }

    vk::Instance createVkInstance()
    {
        // First create the app info. Specifically, we only care about the API
        // version of Vulkan.
        vk::ApplicationInfo appInfo;
        appInfo.apiVersion = VK_API_VERSION_1_1;

        // If we are building debug, ensure that the validation layers exist.
        if constexpr (globals::isDebugBuild)
        {
            if (!validateProperties<vk::LayerProperties>(
                    globals::validationLayers,
                    vk::enumerateInstanceLayerProperties(), compareLayers))
            {
                throw std::runtime_error{
                    "error: there are missing required validation layers"};
            }
        }

        // Now grab the required extensions.
        auto extensionNames = getRequiredExtensions();

        // Set the data related to the validation layers.
        std::uint32_t layerCount =
            (globals::isDebugBuild)
                ? static_cast<std::uint32_t>(globals::validationLayers.size())
                : 0;
        char const* const* layerNames = (globals::isDebugBuild)
                                            ? globals::validationLayers.data()
                                            : nullptr;

        // Finally, create the instance.
        vk::InstanceCreateInfo createInfo;
        createInfo.pApplicationInfo    = &appInfo;
        createInfo.enabledLayerCount   = layerCount;
        createInfo.ppEnabledLayerNames = layerNames;
        createInfo.enabledExtensionCount =
            static_cast<std::uint32_t>(extensionNames.size());
        createInfo.ppEnabledExtensionNames = extensionNames.data();

        auto debugCreateInfo = getDebugMessengerCreateInfo();
        if constexpr (globals::isDebugBuild)
        {
            createInfo.setPNext(&debugCreateInfo);
        }

        return vk::createInstance(createInfo);
    }

    vk::DebugUtilsMessengerEXT
    createDebugMessenger(vk::Instance const& instance)
    {
        if constexpr (globals::isDebugBuild)
        {
            loadDebugFunctions(instance);
            return instance.createDebugUtilsMessengerEXT(
                getDebugMessengerCreateInfo());
        }

        return {};
    }

    vk::SurfaceKHR createSurfaceKHR(vk::Instance const& instance,
                                    GLFWwindow* window)
    {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface))
        {
            throw std::runtime_error{"error: could not create window surface."};
        }

        return surface;
    }

    vk::PhysicalDevice pickPhysicalDevice(vk::Instance const& instance,
                                          vk::SurfaceKHR const& surface)
    {
        std::vector<vk::PhysicalDevice> devices =
            instance.enumeratePhysicalDevices();
        if (devices.empty())
        {
            throw std::runtime_error{
                "error: there are no devices that support Vulkan."};
        }

        for (auto const& device : devices)
        {
            if (isDeviceSuitable(device, surface))
            {
                return device;
            }
        }

        return {};
    }

    LogicalDevice createLogicalDevice(vk::PhysicalDevice const& physicalDevice,
                                      vk::SurfaceKHR const& surface)
    {
        auto indices = findQueueFamilies(physicalDevice, surface);

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
            (globals::isDebugBuild)
                ? static_cast<std::uint32_t>(globals::validationLayers.size())
                : 0;
        char const* const* layerNames = (globals::isDebugBuild)
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

        LogicalDevice device;
        device.device = physicalDevice.createDevice(createInfo);
        device.graphicsQueue =
            device.device.getQueue(*indices.graphicsFamily, 0);
        device.presentQueue = device.device.getQueue(*indices.presentFamily, 0);
        return device;
    }

    Swapchain createSwapchain(vk::PhysicalDevice const& physicalDevice,
                              vk::SurfaceKHR const& surface,
                              vk::Device const& device, GLFWwindow* window)
    {
        auto swapchainDetails = querySwapchainSupport(physicalDevice, surface);

        Swapchain s;
        s.chain  = createSwapchainKHR(physicalDevice, surface, device, window);
        s.images = device.getSwapchainImagesKHR(*s.chain);
        s.format = chooseSwapSurfaceFormat(swapchainDetails.formats).format;
        s.extent = chooseSwapExtent(swapchainDetails.capabilities, window);
        s.imageViews = createImageViews(device, s.images, s.format);
        s.renderPass = createRenderPass(device, s.format);

        Pipeline p         = createPipeline(device, s.extent, *s.renderPass);
        s.pipelineLayout   = vk::UniquePipelineLayout(p.layout, device);
        s.graphicsPipeline = vk::UniquePipeline(p.pipeline, device);
        s.framebuffers =
            createFramebuffers(device, s.imageViews, *s.renderPass, s.extent);
        s.commandPool = createCommandPool(device, physicalDevice, surface);
        s.commandBuffers =
            createCommandBuffers(device, s.framebuffers, *s.commandPool,
                                 *s.renderPass, s.extent, *s.graphicsPipeline);

        return s;
    }

    SyncObjects createSyncObjects(std::size_t maxFrames,
                                  vk::Device const& device)
    {
        SyncObjects objects;
        objects.imageAvailable.resize(maxFrames);
        objects.renderFinished.resize(maxFrames);
        objects.inFlight.resize(maxFrames);

        vk::SemaphoreCreateInfo semaphore;
        vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};

        for (std::size_t i{0}; i < maxFrames; ++i)
        {
            objects.imageAvailable[i] = device.createSemaphoreUnique(semaphore);
            objects.renderFinished[i] = device.createSemaphoreUnique(semaphore);
            objects.inFlight[i]       = device.createFenceUnique(fenceInfo);
        }

        return objects;
    }

    void recreateSwapchain(Swapchain& s, GLFWwindow* window,
                           vk::Device const& device,
                           vk::PhysicalDevice const& physicalDevice,
                           vk::SurfaceKHR const& surface)
    {
        int width{0}, height{0};
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        device.waitIdle();

        cleanupSwapchain(s, device);

        auto swapchainDetails = querySwapchainSupport(physicalDevice, surface);

        s.chain  = createSwapchainKHR(physicalDevice, surface, device, window);
        s.images = device.getSwapchainImagesKHR(*s.chain);
        s.format = chooseSwapSurfaceFormat(swapchainDetails.formats).format;
        s.extent = chooseSwapExtent(swapchainDetails.capabilities, window);
        s.imageViews = createImageViews(device, s.images, s.format);
        s.renderPass = createRenderPass(device, s.format);

        Pipeline p         = createPipeline(device, s.extent, *s.renderPass);
        s.pipelineLayout   = vk::UniquePipelineLayout(p.layout, device);
        s.graphicsPipeline = vk::UniquePipeline(p.pipeline, device);
        s.framebuffers =
            createFramebuffers(device, s.imageViews, *s.renderPass, s.extent);
        s.commandBuffers =
            createCommandBuffers(device, s.framebuffers, *s.commandPool,
                                 *s.renderPass, s.extent, *s.graphicsPipeline);
    }

    void cleanupSwapchain(Swapchain& s, vk::Device const& device)
    {
        // Start with the framebuffers
        for (std::size_t i{0}; i < s.framebuffers.size(); ++i)
        {
            device.destroyFramebuffer(*s.framebuffers[i]);
            s.framebuffers[i].release();
        }

        s.framebuffers.clear();

        // Now come the command buffers.
        std::vector<vk::CommandBuffer> tempBuffers;
        for (std::size_t i{0}; i < s.commandBuffers.size(); ++i)
        {
            tempBuffers.push_back(*s.commandBuffers[i]);
            s.commandBuffers[i].release();
        }
        s.commandBuffers.clear();

        device.freeCommandBuffers(*s.commandPool, tempBuffers);

        device.destroyPipeline(*s.graphicsPipeline);
        s.graphicsPipeline.release();
        device.destroyPipelineLayout(*s.pipelineLayout);
        s.pipelineLayout.release();
        device.destroyRenderPass(*s.renderPass);
        s.renderPass.release();

        for (std::size_t i{0}; i < s.imageViews.size(); ++i)
        {
            device.destroyImageView(*s.imageViews[i]);
            s.imageViews[i].release();
        }
        s.imageViews.clear();

        device.destroySwapchainKHR(*s.chain);
        s.chain.release();
    }

} // namespace vkutils
