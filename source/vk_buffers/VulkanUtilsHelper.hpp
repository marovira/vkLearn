#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace vkutils
{
    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 colour;

        static vk::VertexInputBindingDescription getBindingDescription();
        static std::array<vk::VertexInputAttributeDescription, 2>
        getAttributeDescriptions();
    };
} // namespace vkutils

namespace globals
{
#if defined(NDEBUG)
    static constexpr auto isDebugBuild{false};
#else
    static constexpr auto isDebugBuild{true};
#endif

    static const std::vector<const char*> validationLayers{
        "VK_LAYER_KHRONOS_validation"};
    static const std::vector<const char*> deviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    std::vector<vkutils::Vertex> const vertices{
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
} // namespace globals

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* callbackData, void* userData);

namespace vkutils
{
    struct QueueFamilyIndices
    {
        std::optional<std::uint32_t> graphicsFamily;
        std::optional<std::uint32_t> presentFamily;

        bool isComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    struct Pipeline
    {
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };

    struct VertexBuffer
    {
        vk::Buffer buffer;
        vk::DeviceMemory bufferMemory;
    };

    template<typename T>
    using ComparatorFn = std::function<bool(std::string_view const&, T const&)>;

    using CStringVector = std::vector<char const*>;

    template<typename T>
    bool validateProperties(std::vector<char const*> const& required,
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

    bool compareLayers(std::string_view const& layerName,
                       vk::LayerProperties const& layer);
    bool compareExtensions(std::string_view const& extensionName,
                           vk::ExtensionProperties const& extension);

    std::vector<char const*> getRequiredExtensions();
    void loadDebugFunctions(vk::Instance const& instance);
    vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();
    bool isDeviceSuitable(vk::PhysicalDevice const& device,
                          vk::SurfaceKHR const& surface);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice const& device,
                                         vk::SurfaceKHR const& surface);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice const& device);
    SwapchainSupportDetails
    querySwapchainSupport(vk::PhysicalDevice const& device,
                          vk::SurfaceKHR const& surface);

    vk::UniqueSwapchainKHR
    createSwapchainKHR(vk::PhysicalDevice const& physicalDevice,
                       vk::SurfaceKHR const& surface, vk::Device const& device,
                       GLFWwindow* window);
    vk::SurfaceFormatKHR
    chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& formats);
    vk::PresentModeKHR
    chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& presentModes);
    vk::Extent2D
    chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities,
                     GLFWwindow* window);
    std::vector<vk::UniqueImageView>
    createImageViews(vk::Device const& device,
                     std::vector<vk::Image> const& images,
                     vk::Format const& format);
    vk::UniqueRenderPass createRenderPass(vk::Device const& device,
                                          vk::Format const& format);

    std::vector<char> readFile(std::string const& filename);
    Pipeline createPipeline(vk::Device const& device,
                            vk::Extent2D const& extent,
                            vk::RenderPass const& renderPass);
    vk::UniqueShaderModule createShaderModule(vk::Device const& device,
                                              std::vector<char> const& code);

    std::vector<vk::UniqueFramebuffer>
    createFramebuffers(vk::Device const& device,
                       std::vector<vk::UniqueImageView> const& imageViews,
                       vk::RenderPass const& renderPass,
                       vk::Extent2D const& extent);

    VertexBuffer createVertexBuffer(vk::Device const& device,
                                    vk::PhysicalDevice const& physicalDevice);
    std::uint32_t findMemoryType(std::uint32_t typeFilter,
                                 vk::MemoryPropertyFlags const& properties,
                                 vk::PhysicalDevice const& physicalDevice);

    vk::UniqueCommandPool
    createCommandPool(vk::Device const& device,
                      vk::PhysicalDevice const& physicalDevice,
                      vk::SurfaceKHR const& surface);

    std::vector<vk::UniqueCommandBuffer> createCommandBuffers(
        vk::Device const& device,
        std::vector<vk::UniqueFramebuffer> const& framebuffers,
        vk::CommandPool const& commandPool, vk::RenderPass const& renderPass,
        vk::Extent2D const& extent, vk::Pipeline const& pipeline,
        vk::Buffer const& buffer);

} // namespace vkutils
