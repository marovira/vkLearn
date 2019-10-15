#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

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

    GLFWwindow* mWindow{nullptr};

    // This is the interface between our application and Vulkan.
    // Notice that by using vk::UniqueInstance, we no longer
    // have to worry about destroying the instance.
    vk::UniqueInstance mInstance{};

    // This holds the debug messenger function that will get called
    // whenever a validation layer issues a message. Similar to how
    // we made the instance, we don't have to worry about deleting this.
    vk::UniqueDebugUtilsMessengerEXT mDebugMessenger{};
};
