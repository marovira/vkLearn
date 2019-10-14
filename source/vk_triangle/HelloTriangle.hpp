#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

class HelloTriangleApplication
{
public:
    void run();

private:
    void initWindow();

    void initVulkan();
    void checkExtensions();
    bool checkValidationLayers();
    std::vector<const char*> getRequiredExtensions();
    void createInstance();

    void mainLoop();
    void cleanup();

    GLFWwindow* mWindow;

    // This is the connection between the application and the Vulkan library.
    // Should always be created first.
    vk::Instance mInstance;
};
