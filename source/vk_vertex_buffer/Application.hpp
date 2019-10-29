#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <functional>
#include <optional>

class Application
{
public:
    void run();
    void framebuffeResized();
    void redrawWindow();

private:
    void initVulkan();
    void mainLoop();
    void cleanup();
    void initWindow();

    void createInstance();
    void listExtensions();

    template<typename T>
    using ComparatorFn = std::function<bool(std::string_view const&, T const&)>;

    using CStringVector = std::vector<char const*>;

    template<typename T>
    bool validateProperties(CStringVector const& required,
                            std::vector<T> const& available,
                            ComparatorFn<T> const& fn)
    {
        return std::all_of(
            required.begin(), required.end(), [&available, &fn](auto name) {
                std::string_view propertyName{name};
                auto result =
                    std::find_if(available.begin(), available.end(),
                                 [&propertyName, &fn](auto const& prop) {
                                     return fn(propertyName, prop);
                                 });
                return result != available.end();
            });
    }

    std::vector<const char*> getRequiredExtensions();
    vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();
    void setupDebugMessenger();

    struct QueueFamilyIndices
    {
        std::optional<std::uint32_t> graphicsFamily;
        std::optional<std::uint32_t> presentFamily;

        bool isComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    void pickPhysicalDevice();
    bool isDeviceSuitable(vk::PhysicalDevice const& device);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice const& device);

    void createLogicalDevice();

    void createSurface();

    struct SwapChainSupportDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    bool checkDeviceExtensionSupport(vk::PhysicalDevice const& device);
    SwapChainSupportDetails
    querySwapChainSupport(vk::PhysicalDevice const& device);
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        std::vector<vk::SurfaceFormatKHR> const& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(
        std::vector<vk::PresentModeKHR> const& availablePresentModes);
    vk::Extent2D
    chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities);
    void createSwapChain();

    void createImageViews();

    void createGraphicsPipeline();
    vk::UniqueShaderModule createShaderModule(std::vector<char> const& code);

    void createRenderPass();

    void createFramebuffers();

    void createCommandPool();
    void createCommandBuffers();

    void createSyncObjects();
    void drawFrame();

    void recreateSwapChain();
    void cleanupSwapChain();

    GLFWwindow* mWindow{nullptr};

    vk::UniqueInstance mInstance{};
    vk::UniqueDebugUtilsMessengerEXT mDebugMessenger{};

    vk::PhysicalDevice mPhysicalDevice{};
    vk::UniqueDevice mDevice{};

    vk::Queue mGraphicsQueue{};

    vk::UniqueSurfaceKHR mSurface{};
    vk::Queue mPresentQueue{};

    vk::UniqueSwapchainKHR mSwapchain{};
    std::vector<vk::Image> mSwapchainImages{};
    vk::Format mSwapchainImageFormat{};
    vk::Extent2D mSwapchainExtent{};
    std::vector<vk::UniqueImageView> mSwapchainImageViews{};
    vk::UniqueRenderPass mRenderPass{};
    vk::UniquePipelineLayout mPipelineLayout{};
    vk::UniquePipeline mGraphicsPipeline{};
    std::vector<vk::UniqueFramebuffer> mSwapchainFramebuffers{};

    vk::UniqueCommandPool mCommandPool{};
    std::vector<vk::UniqueCommandBuffer> mCommandBuffers{};

    std::vector<vk::UniqueSemaphore> mImageAvailableSemaphores{};
    std::vector<vk::UniqueSemaphore> mRenderFinishedSemaphores{};

    std::vector<vk::UniqueFence> mInFlightFences{};

    std::size_t mCurrentFrame{0};
    bool mFramebufferResized{false};
};
