#include "Application.hpp"

using namespace vkutils;

static void onFramebufferSize(GLFWwindow* window, int, int)
{
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->framebufferResized();
}

static void onWindowRedraw(GLFWwindow* window)
{
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->redrawWindow();
}

void Application::init()
{
    // Initialize GLFW.
    initGLFW();

    // Now create the Vulkan instance and debug messenger.
    mInstance       = vk::UniqueInstance(createVkInstance());
    mDebugMessenger = vk::UniqueDebugUtilsMessengerEXT(
        createDebugMessenger(*mInstance), *mInstance);

    // Next create the surface we're rendering to.
    mSurface =
        vk::UniqueSurfaceKHR(createSurfaceKHR(*mInstance, mWindow), *mInstance);

    // Now comes the physical device.
    mPhysicalDevice = pickPhysicalDevice(*mInstance, *mSurface);

    // Create the logical device and the queues.
    auto logicalDevice = createLogicalDevice(mPhysicalDevice, *mSurface);
    mLogicalDevice     = vk::UniqueDevice(logicalDevice.device);
    mGraphicsQueue     = logicalDevice.graphicsQueue;
    mPresentQueue      = logicalDevice.presentQueue;

    // Next comes the swapchain.
    mSwapchain =
        createSwapchain(mPhysicalDevice, *mSurface, *mLogicalDevice, mWindow);

    // Finally, create the sync objects.
    mSyncObjects = createSyncObjects(mMaxFramesInFlight, *mLogicalDevice);
}

void Application::run()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        drawFrame();
        glfwPollEvents();
    }

    mLogicalDevice->waitIdle();
}

void Application::exit()
{
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void Application::framebufferResized()
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

void Application::drawFrame()
{
    mLogicalDevice->waitForFences({*mSyncObjects.inFlight[mCurrentFrame]},
                                  VK_TRUE,
                                  std::numeric_limits<std::uint64_t>::max());

    auto result = mLogicalDevice->acquireNextImageKHR(
        *mSwapchain.chain, std::numeric_limits<std::uint32_t>::max(),
        *mSyncObjects.imageAvailable[mCurrentFrame], {});

    if (result.result == vk::Result::eErrorOutOfDateKHR ||
        result.result == vk::Result::eSuboptimalKHR || mFramebufferResized)
    {
        if (mFramebufferResized && result.result == vk::Result::eSuccess)
        {
            mSyncObjects.imageAvailable[mCurrentFrame].reset();
            vk::SemaphoreCreateInfo semaphoreInfo;
            mSyncObjects.imageAvailable[mCurrentFrame] =
                mLogicalDevice->createSemaphoreUnique(semaphoreInfo);
        }

        mFramebufferResized = false;
        recreateSwapchain(mSwapchain, mWindow, *mLogicalDevice, mPhysicalDevice,
                          *mSurface);
        return;
    }
    else if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error{"error: unable to acquire swapchain image."};
    }

    std::uint32_t imageIndex = result.value;

    std::array<vk::Semaphore, 1> waitSemaphore{
        *mSyncObjects.imageAvailable[mCurrentFrame]};
    std::array<vk::PipelineStageFlags, 1> waitStages{
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    std::array<vk::Semaphore, 1> signalSemaphores{
        *mSyncObjects.renderFinished[mCurrentFrame]};

    vk::SubmitInfo submitInfo;
    submitInfo.waitSemaphoreCount =
        static_cast<std::uint32_t>(waitSemaphore.size());
    submitInfo.pWaitSemaphores    = waitSemaphore.data();
    submitInfo.pWaitDstStageMask  = waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &(*mSwapchain.commandBuffers[imageIndex]);
    submitInfo.signalSemaphoreCount =
        static_cast<std::uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    mLogicalDevice->resetFences({*mSyncObjects.inFlight[mCurrentFrame]});

    mGraphicsQueue.submit({submitInfo}, *mSyncObjects.inFlight[mCurrentFrame]);

    std::array<vk::SwapchainKHR, 1> swapchains{*mSwapchain.chain};

    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount =
        static_cast<std::uint32_t>(signalSemaphores.size());
    presentInfo.pWaitSemaphores = signalSemaphores.data();
    presentInfo.swapchainCount  = static_cast<std::uint32_t>(swapchains.size());
    presentInfo.pSwapchains     = swapchains.data();
    presentInfo.pImageIndices   = &imageIndex;

    mPresentQueue.presentKHR(presentInfo);
    mCurrentFrame = (mCurrentFrame + 1) % mMaxFramesInFlight;
}

void Application::initGLFW()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mWindow = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

    glfwSetWindowUserPointer(mWindow, this);
    glfwSetFramebufferSizeCallback(mWindow, onFramebufferSize);
    glfwSetWindowRefreshCallback(mWindow, onWindowRedraw);
}
