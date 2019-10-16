#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <optional>

class HelloTriangleApplication
{
public:
    void run();

private:
    void initVulkan();
    void mainLoop();
    void cleanup();
    void initWindow();

    void createInstance();
    void listExtensions();

    // TODO: See if there is a clever way of templating this so it collapses
    // down to a single function, as they are virtually identical save for
    // the name of the variable that holds the property name.
    bool
    checkExtensions(std::vector<char const*> const& layers,
                    std::vector<vk::ExtensionProperties> const& properties);
    bool checkLayers(std::vector<char const*> const& layers,
                     std::vector<vk::LayerProperties> const& properties);

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

    GLFWwindow* mWindow{nullptr};

    // This is the interface between our application and Vulkan.
    // Notice that by using vk::UniqueInstance, we no longer
    // have to worry about destroying the instance.
    vk::UniqueInstance mInstance{};

    // This holds the debug messenger function that will get called
    // whenever a validation layer issues a message. Similar to how
    // we made the instance, we don't have to worry about deleting this.
    vk::UniqueDebugUtilsMessengerEXT mDebugMessenger{};

    // The physical device represents the actual physical graphics card that we
    // are going to use. Note that the device is implicitly destroyed when we
    // destroy the instance, so there's no need to do anything else with it.
    vk::PhysicalDevice mPhysicalDevice{};

    // If the physical device represents the graphics card, then the device (or
    // logical device) is the interface into the physical device. Note that at
    // no point will we be using the physical device directly. Instead, what
    // we're going to do is interface through the logical device. This does
    // mean that we can have multiple logical devices per physical device. This
    // can be useful if different subsets, plugins or systems require their own
    // Vulkan handles but don't need to be aware of the existence of other
    // instances.
    // Because the device doesn't interact with the instance, it needs to be
    // handled (and destroyed) separately.
    vk::UniqueDevice mDevice{};

    // This is the handle to the device queue that gets created along with the
    // logical device. Similarly to the physical device, it will get destroyed
    // when the logical device is destroyed. This queue allows us to submit
    // graphics commands.
    vk::Queue mGraphicsQueue{};

    // Vulkan is a platform agnostic API, so it has no direct way of interfacing
    // with whatever windowing system is used to render things (and in fact it
    // doesn't have to). To simplify the interface, Vulkan provides an
    // abstraction in the form of the Surface, which is a general interface for
    // rendering. It is important to note that while the surface itself is
    // cross-platform, the creation of the surface itself isn't.
    vk::UniqueSurfaceKHR mSurface{};

    // This queue handle is for presentation and is involved in getting things
    // on the surface that we created earlier.
    vk::Queue mPresentQueue{};

    // The swap chain is the one that holds the images that will be presented
    // to the screen, effectively creating the setup for the default
    // framebuffer and techniques like double buffering.
    vk::UniqueSwapchainKHR mSwapChain{};

    // This will hold the images from the swap chain. Since they are
    // cleaned up along with the swap chain, we don't need to do anything extra.
    std::vector<vk::Image> mSwapChainImages{};

    // The format of the swap chain images.
    vk::Format mSwapChainImageFormat{};
    vk::Extent2D mSwapChainExtent{};
};
