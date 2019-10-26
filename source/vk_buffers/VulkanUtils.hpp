#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

namespace vkutils
{
    struct LogicalDevice
    {
        vk::Device device;
        vk::Queue graphicsQueue;
        vk::Queue presentQueue;
    };

    struct Swapchain
    {
        vk::UniqueSwapchainKHR chain;
        std::vector<vk::Image> images;
        vk::Format format;
        vk::Extent2D extent;
        std::vector<vk::UniqueImageView> imageViews;
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline graphicsPipeline;
        std::vector<vk::UniqueFramebuffer> framebuffers;
        vk::UniqueCommandPool commandPool;
        std::vector<vk::UniqueCommandBuffer> commandBuffers;
    };

    struct SyncObjects
    {
        std::vector<vk::UniqueSemaphore> imageAvailable;
        std::vector<vk::UniqueSemaphore> renderFinished;
        std::vector<vk::UniqueFence> inFlight;
    };

    void listAvailableInstanceExtensions();
    vk::Instance createVkInstance();
    vk::DebugUtilsMessengerEXT
    createDebugMessenger(vk::Instance const& instance);
    vk::SurfaceKHR createSurfaceKHR(vk::Instance const& instance,
                                    GLFWwindow* window);
    vk::PhysicalDevice pickPhysicalDevice(vk::Instance const& instance,
                                          vk::SurfaceKHR const& surface);

    LogicalDevice createLogicalDevice(vk::PhysicalDevice const& physicalDevice,
                                      vk::SurfaceKHR const& surface);

    Swapchain createSwapchain(vk::PhysicalDevice const& physicalDevice,
                              vk::SurfaceKHR const& surface,
                              vk::Device const& device, GLFWwindow* window);

    SyncObjects createSyncObjects(std::size_t maxFrames,
                                  vk::Device const& device);

    void recreateSwapchain(Swapchain& swapchain, GLFWwindow* window,
                           vk::Device const& device,
                           vk::PhysicalDevice const& physicalDevice,
                           vk::SurfaceKHR const& surface);
    void cleanupSwapchain(Swapchain& swapchain, vk::Device const& device);

} // namespace vkutils
