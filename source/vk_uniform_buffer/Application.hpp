#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <functional>
#include <glm/glm.hpp>
#include <optional>

struct QueueFamilyIndices
{
    std::optional<std::uint32_t> graphicsFamily;
    std::optional<std::uint32_t> presentFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 colour;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 2>
    getAttributeDescriptions();
};

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

    void pickPhysicalDevice();
    bool isDeviceSuitable(vk::PhysicalDevice const& device);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice const& device);

    void createLogicalDevice();

    void createSurface();

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

    void createVertexBuffer();
    std::uint32_t findMemoryType(std::uint32_t typeFilter,
                                 vk::MemoryPropertyFlags const& properties);

    void createBuffer(vk::DeviceSize const& size,
                      vk::BufferUsageFlags const& usage,
                      vk::MemoryPropertyFlags const& properties,
                      vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
    void copyBuffer(vk::Buffer const& srcBuffer, vk::Buffer const& dstBuffer,
                    vk::DeviceSize const& size);

    void createIndexBuffer();

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
    vk::UniqueCommandPool mBufferPool{};
    std::vector<vk::UniqueCommandBuffer> mCommandBuffers{};

    std::vector<vk::UniqueSemaphore> mImageAvailableSemaphores{};
    std::vector<vk::UniqueSemaphore> mRenderFinishedSemaphores{};

    std::vector<vk::UniqueFence> mInFlightFences{};

    std::size_t mCurrentFrame{0};
    bool mFramebufferResized{false};

    vk::UniqueBuffer mVertexBuffer;
    vk::UniqueDeviceMemory mVertexBufferMemory;

    vk::UniqueBuffer mIndexBuffer;
    vk::UniqueDeviceMemory mIndexBufferMemory;
};
