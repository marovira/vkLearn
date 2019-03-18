#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fmt/printf.h>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.hpp>

const std::vector<const char*> kValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"};

constexpr auto kWidth{800};
constexpr auto kHeight{600};

#if defined    NDEBUG
constexpr bool enableValidationLayers{false};
#else
constexpr bool enableValidationLayers{true};
#endif

// Vulkan does not load the debug messenger by default, so we must load the
// function pointer for both creation and destruction ourselves.
VkResult createDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT const* pCreateInfo,
    VkAllocationCallbacks const* pAllocator,
    VkDebugUtilsMessengerEXT*    pDebugMessenger)
{
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (fn != nullptr)
    {
        return fn(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroyDebugUtilsMessengerEXT(VkInstance                   instance,
                                   VkDebugUtilsMessengerEXT     debugMessenger,
                                   VkAllocationCallbacks const* pAllocator)
{
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (fn != nullptr)
    {
        fn(instance, debugMessenger, pAllocator);
    }
}

class TriangleApp
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        shutdown();
    }

private:
    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        mWindow = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(mWindow))
        {
            glfwPollEvents();
        }
    }

    void shutdown()
    {
        // Make sure that we cleanup after ourselves.
        if constexpr (enableValidationLayers)
        {
            destroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
        }

        vkDestroyInstance(mInstance, nullptr);

        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }

    void createInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error{
                "Validation layers requested, but not available!"};
        }

        // Contains information about our application. Technically, this is
        // optional, but it allows the driver to further optimize our program.
        // Make sure that the versions of Vulkan match. Also, we must
        // specify the type of the structure.
        VkApplicationInfo appInfo{};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 1, 0);
        appInfo.pEngineName        = "No engine";
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 1, 0);
        appInfo.apiVersion         = VK_API_VERSION_1_1;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Grab the required extensions. Since Vulkan is platform agnostic,
        // these must be specified manually.
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount =
            static_cast<std::uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // This sets the number of global layers to enable.
        if constexpr (enableValidationLayers)
        {
            createInfo.enabledLayerCount =
                static_cast<std::uint32_t>(kValidationLayers.size());
            createInfo.ppEnabledLayerNames = kValidationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        // Create the instance and make sure it works.
        if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
        {
            throw std::runtime_error{"Failed to create Vulkan instance"};
        }
    }

    void setupDebugMessenger()
    {
        if constexpr (!enableValidationLayers)
        {
            return;
        }

        // This sets up the debug callback in a very similar way to the
        // debug callback of OpenGL (albeit in a much more verbose way).
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        if (createDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr,
                                         &mDebugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error{"Failed to set up debug messenger"};
        }
    }

    std::vector<const char*> getRequiredExtensions()
    {
        // We first grab the extensions that GLFW requires to function
        // correctly.
        std::uint32_t glfwExtensionCount = 0;
        const char**  glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(
            glfwExtensions, glfwExtensions + glfwExtensionCount);

        // Now check if we are using validation layers. If we are, then
        // add the debug extension from LunarVK.
        if constexpr (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkValidationLayerSupport()
    {
        std::uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        // Grab all of the available layers.
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // Now check that the validation layers that we want currently exist
        // on the device. If they do, then we're good and may proceed as usual.
        for (auto layerName : kValidationLayers)
        {
            bool layerFound{false};

            for (auto const& layerProperties : availableLayers)
            {
                if (std::string_view{layerName} ==
                    std::string_view{layerProperties.layerName})
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    // Vulkan debug callback function
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT        messageType,
        VkDebugUtilsMessengerCallbackDataEXT const*             pCallbackdata,
        [[maybe_unused]] void*                                  pUserData)
    {
        fmt::print("Validation layer: {}\n", pCallbackdata->pMessage);
        return VK_FALSE;
    }

    GLFWwindow*              mWindow;
    VkInstance               mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
};

int main()
{
    TriangleApp app;

    try
    {
        app.run();
    }
    catch (std::exception const& e)
    {
        fmt::print("{}\n", e.what());
        return 1;
    }

    return 0;
}
