#pragma once

#include "VulkanUtils.hpp"

class Application
{
public:
    void init();
    void run();
    void exit();
    void framebufferResized();
    void redrawWindow();

private:
    void initGLFW();
    void drawFrame();

    GLFWwindow* mWindow;

    vk::UniqueInstance mInstance;
    vk::UniqueDebugUtilsMessengerEXT mDebugMessenger;
    vk::UniqueSurfaceKHR mSurface;

    vk::PhysicalDevice mPhysicalDevice;

    vk::UniqueDevice mLogicalDevice;
    vk::Queue mGraphicsQueue;
    vk::Queue mPresentQueue;

    vkutils::Swapchain mSwapchain;
    vkutils::SyncObjects mSyncObjects;

    std::size_t mCurrentFrame{0};
    bool mFramebufferResized{false};
    const std::size_t mMaxFramesInFlight{2};
};
