#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <atlas/utils/Cameras.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
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
    glm::vec3 pos;
    glm::vec3 colour;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 3>
    getAttributeDescriptions();
};

struct UniformMatrices
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
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
                    std::find_if(available.begin(),
                                 available.end(),
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
                      vk::Buffer& buffer,
                      vk::DeviceMemory& bufferMemory);
    void copyBuffer(vk::Buffer const& srcBuffer,
                    vk::Buffer const& dstBuffer,
                    vk::DeviceSize const& size);

    void createIndexBuffer();

    void createDescriptorSetLayout();
    void createUniformBuffers();
    void updateUniformBuffer(std::uint32_t currentImage);

    void createDescriptorPool();
    void createDescriptorSets();

    void createTextureImage();
    void createImage(std::uint32_t width,
                     std::uint32_t height,
                     std::uint32_t mipLevel,
                     vk::SampleCountFlagBits const& numSamples,
                     vk::Format const& format,
                     vk::ImageTiling const& tiling,
                     vk::ImageUsageFlags const& usage,
                     vk::MemoryPropertyFlags const& properties,
                     vk::Image& image,
                     vk::DeviceMemory& imageMemory);
    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer const& commandBuffer);
    void transitionImageLayout(vk::Image const& image,
                               vk::Format const& format,
                               vk::ImageLayout const& oldLayout,
                               vk::ImageLayout const& newLayout,
                               std::uint32_t mipLevels);
    void copyBufferToImage(vk::Buffer const& buffer,
                           vk::Image const& image,
                           std::uint32_t width,
                           std::uint32_t height);

    void createTextureImageView();
    vk::ImageView createImageView(vk::Image const& image,
                                  vk::Format const& format,
                                  vk::ImageAspectFlags const& aspectFlags,
                                  std::uint32_t mipLevels);
    void createTextureSampler();

    void createDepthResources();
    vk::Format findSupportedFormat(std::vector<vk::Format> const& candidates,
                                   vk::ImageTiling const& tiling,
                                   vk::FormatFeatureFlags const& features);
    vk::Format findDepthFormat();
    bool hasStencilComponent(vk::Format const& format);

    void loadModel();

    void generateMipmaps(vk::Image const& image,
                         vk::Format const& format,
                         std::int32_t texWidth,
                         std::int32_t texHeight,
                         std::uint32_t mipLevels);

    vk::SampleCountFlagBits getMaxUsableSampleCount();
    void createColourResources();

    GLFWwindow* mWindow{nullptr};

    vk::UniqueInstance mInstance;
    vk::UniqueDebugUtilsMessengerEXT mDebugMessenger;

    vk::PhysicalDevice mPhysicalDevice;
    vk::UniqueDevice mDevice;

    vk::Queue mGraphicsQueue;

    vk::UniqueSurfaceKHR mSurface;
    vk::Queue mPresentQueue;

    vk::UniqueSwapchainKHR mSwapchain;
    std::vector<vk::Image> mSwapchainImages;
    vk::Format mSwapchainImageFormat;
    vk::Extent2D mSwapchainExtent;
    std::vector<vk::UniqueImageView> mSwapchainImageViews;
    vk::UniqueRenderPass mRenderPass;
    vk::UniqueDescriptorSetLayout mDescriptorSetLayout;
    vk::UniquePipelineLayout mPipelineLayout;
    vk::UniquePipeline mGraphicsPipeline;
    std::vector<vk::UniqueFramebuffer> mSwapchainFramebuffers;

    vk::UniqueCommandPool mCommandPool;
    vk::UniqueCommandPool mBufferPool;
    std::vector<vk::UniqueCommandBuffer> mCommandBuffers;

    std::vector<vk::UniqueSemaphore> mImageAvailableSemaphores;
    std::vector<vk::UniqueSemaphore> mRenderFinishedSemaphores;

    std::vector<vk::UniqueFence> mInFlightFences;

    std::size_t mCurrentFrame{0};
    bool mFramebufferResized{false};

    vk::UniqueBuffer mVertexBuffer;
    vk::UniqueDeviceMemory mVertexBufferMemory;

    vk::UniqueBuffer mIndexBuffer;
    vk::UniqueDeviceMemory mIndexBufferMemory;

    std::vector<vk::UniqueBuffer> mUniformBuffers;
    std::vector<vk::UniqueDeviceMemory> mUniformBuffersMemory;

    vk::UniqueDescriptorPool mDescriptorPool;
    std::vector<vk::UniqueDescriptorSet> mDescriptorSets;

    vk::UniqueImage mTextureImage;
    vk::UniqueDeviceMemory mTextureImageMemory;

    vk::UniqueImageView mTextureImageView;
    vk::UniqueSampler mTextureSampler;

    vk::UniqueImage mDepthImage;
    vk::UniqueDeviceMemory mDepthImageMemory;
    vk::UniqueImageView mDepthImageView;

    std::vector<Vertex> mVertices;
    std::vector<std::uint32_t> mIndices;

    std::uint32_t mMipLevels;

    vk::SampleCountFlagBits mMSAASamples{vk::SampleCountFlagBits::e1};
    vk::UniqueImage mColourImage;
    vk::UniqueDeviceMemory mColourImageMemory;
    vk::UniqueImageView mColourImageView;
};
