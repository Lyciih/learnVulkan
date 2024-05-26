#include <vulkan/vulkan.h>

#include <X11/Xlib.h>
// vulkan 的 xlib 相關模組，一定要在X11/Xlib.h 被 include 之後才能 include ，因為有用到其中的定義。
#include <vulkan/vulkan_xlib.h>



#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <fstream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 800;

// 宣告要使用的驗證層有哪些
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

// 宣告需要的設備擴展
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#define NDEBUG

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

// 動態載入外部的用來建立信使的函數
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger){
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	//如果成功載入則執行該函數
	if(func != nullptr){
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// 回收之前產生的調試信使
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator){
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if(func != nullptr){
		func(instance, debugMessenger, pAllocator);
	}
}





class HelloTriangleApplication {
public:
	void run(){
		initWindow();
		initVulkan();
		mainloop();
		cleanup();
	}

private:
	Display *display;
	Window window;
	XEvent event;
	int screen;
	XWindowAttributes windowAttributes;
	
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImageView> swapChainImageViews;


	
	void initWindow(){

		display = XOpenDisplay(nullptr);
		if(display == NULL){
			std::cerr << "無法連接到x伺服器" << std::endl;
			exit(1);
		}


		screen = DefaultScreen(display);

		window = XCreateWindow(display, RootWindow(display, screen), 100, 100, 640, 480, 1, CopyFromParent, InputOutput, CopyFromParent, 0, NULL);

		XSelectInput(display, window, ExposureMask | KeyPressMask);
		XMapWindow(display, window);
	}
	

	void initVulkan(){
		createInstance();
		// 調試信使必須在instance建立後才能創建，如果想看到 instance 創建時的資訊，必須在建立 instance 時也填入相應的調試資料
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createGraphicsPipeline();
	
	}

	
	void mainloop(){

		while(1){
			XNextEvent(display, &event);
			if(event.type == Expose){

			}
			if(event.type == KeyPress){
				break;
			}

		}
	}
	

	void cleanup(){
		for(auto imageView : swapChainImageViews){
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);

		if(enableValidationLayers){
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		
		XDestroyWindow(display, window);
		XCloseDisplay(display);
	}
	

	// 用來檢查想使用的驗證層是否支援
	bool checkValidationLayerSupport(){
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for(const char* layerName : validationLayers){
			bool layerFound = false;

			for(const auto& layerProperties : availableLayers){
				if(strcmp(layerName, layerProperties.layerName) == 0){
					layerFound = true;
					break;
				}
			}

			if(!layerFound){
				return false;
			}
		}

		return true;
	}



	// 取得需要的 extension 名，視需要加入 validation 層需要的擴展
	std::vector<const char*> getRequiredExtensions(){

		std::vector<const char*> extensions;
		extensions.push_back("VK_KHR_surface");
		extensions.push_back("VK_KHR_xlib_surface");
		
		if(enableValidationLayers){
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	// 用來查詢支援的 Extensions
	std::vector<VkExtensionProperties> querySupportExtensions(){
		uint32_t supportExtensionCount = 0;	
		vkEnumerateInstanceExtensionProperties(nullptr, &supportExtensionCount, nullptr);

		std::vector<VkExtensionProperties> supportExtensions(supportExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &supportExtensionCount, supportExtensions.data());
		
		return supportExtensions;
	}

	// 顯示debug消息的回調函數，可以依需求過濾顯示的層級
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageServerity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData
			){
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}


	// 創建 vulkan 實例
	void createInstance(){
		if(enableValidationLayers && !checkValidationLayerSupport()){
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto  extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();


		std::cout << "需要的實例擴展:" << std::endl;
		for(auto it = extensions.begin(); it != extensions.end(); it++){
			std::cout << *it << std::endl;
		}
		std::cout << std::endl;


		auto supportExtensions = querySupportExtensions();
		std::cout << "支援的實例擴展:" << std::endl;
		for(const auto& supportExtension : supportExtensions){
			std::cout << supportExtension.extensionName << std::endl;
		}
		std::cout << std::endl;



		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		// 如果有使用驗證層則填寫
		if(enableValidationLayers){
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			
			// 與驗證層無關，只是為了看到創建 instance 時的資訊
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
		}
		else{
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}


		
		if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS){
			throw std::runtime_error("fail to create instance!");
		}
	}


	// 填寫調試資料
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo){
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}


	// 設定調試信使
	void setupDebugMessenger(){
		if(!enableValidationLayers) return;

		// 填寫申請調試信使的資料
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		// 申請
		if(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS){
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}


	// 建立顯示的表面
	void createSurface(){
		VkXlibSurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		createInfo.dpy = display;
		createInfo.window = window;


		if(vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS){
			throw std::runtime_error("failed to create window surface!");
		}

	}


	// 檢查設備擴展是否支援
	bool checkDeviceExtensionSupport(VkPhysicalDevice device){
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requireExtensions(deviceExtensions.begin(), deviceExtensions.end());


		/*
		std::cout << "支援的設備擴展" << std::endl;
		for(const auto& extension : availableExtensions){
			std::cout << extension.extensionName << std::endl;
			requireExtensions.erase(extension.extensionName);
		}
		std::cout << std::endl;
		*/

		return requireExtensions.empty();
	}

	// 檢查 GPU 是否是適合的
	bool isDeviceSuitable(VkPhysicalDevice device){

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		std::cout << "名稱： " << deviceProperties.deviceName << std::endl;
		
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);		
		std::cout << "檢查特性支援" << std::endl;
		std::cout << "shaderFloat64:\t" << deviceFeatures.shaderFloat64 << std::endl;
		std::cout << "shaderInt64:\t" << deviceFeatures.shaderInt64 << std::endl;
		std::cout << "geometryShader:\t" << deviceFeatures.geometryShader << std::endl;

		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if(extensionsSupported){
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}


	// 存放隊列族索引的結構
	struct QueueFamilyIndices{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete(){
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};


	// 存放交換鏈資訊的結構
	struct SwapChainSupportDetails{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	// 尋找需要的隊列族
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device){
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for(const auto& queueFamily : queueFamilies){
			if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
				indices.graphicsFamily = i;
			}
		
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if(presentSupport){
				indices.presentFamily = i;
			}


			if(indices.isComplete()){
				break;
			}

			i++;
		}

		return indices;
	}

	// 查詢交換鏈資訊
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device){
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if(formatCount != 0){
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}
		
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if(presentModeCount != 0){
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}


	// 選擇適合的format
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats){
		for(const auto& availableFormat : availableFormats){
			if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	// 選擇適合的presenttation mode
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes){
		for(const auto& availablePresentMode : availablePresentModes){
			if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// 選擇extent
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities){
		if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()){
			return capabilities.currentExtent;
		}
		else{
			XGetWindowAttributes(display, window, &windowAttributes);
			int width = windowAttributes.width;
			int height = windowAttributes.height;

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}




	// 尋找可用的 gpu
	void pickPhysicalDevice(){

		std::set<std::string> requireExtensions(deviceExtensions.begin(), deviceExtensions.end());
		
		std::cout << "需要的設備擴展" << std::endl;
		for(const auto& extension : requireExtensions){
			std::cout << extension << std::endl;
		}
		std::cout << std::endl;

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if(deviceCount == 0){
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}



		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		std::cout << "可用的 GPU 數量: " << deviceCount << std::endl;
		int i = 0;
		for(const auto& device : devices){
			std::cout << std::endl << "GPU 編號: " << i << " ";
			isDeviceSuitable(device);
			i++;
		}
		
		std::cout << std::endl << "請選擇要使用的 GPU: " << std::endl;
		int select = 0;
		while(1){
			std::cin >> select;
			if(select >= 0 && select < (int)deviceCount){
				physicalDevice = devices[select];
				std::cout << "選擇成功" << std::endl;
				break;
			}
			else{
				std::cout << "沒有該 GPU 編號，請重新輸入" << std::endl;
			}
		}


		if(physicalDevice == VK_NULL_HANDLE){
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	// 產生邏輯設備
	void createLogicalDevice(){
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		float queuePriority = 1.0f;

		for(uint32_t queueFamily : uniqueQueueFamilies){
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if(enableValidationLayers){
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else{
			createInfo.enabledLayerCount = 0;
		}

		if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS){
			throw std::runtime_error("failed to create logical device!");
		}

		// 取得 queue 的 handle
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	// 產生 SwapChain
	void createSwapChain(){
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount){
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		if(indices.graphicsFamily != indices.presentFamily){
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR; // 如果想關閉透明效果要將 INHERIT 改為 OPAQUE
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;	// 超出surface大小的部份會被裁剪
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS){
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	// 產生image視圖
	void createImageViews(){
		swapChainImageViews.resize(swapChainImages.size());
		
		for(size_t i = 0; i < swapChainImages.size(); i++){
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS){
				throw std::runtime_error("failed to create image views!");
			}
		}
	}


	// 用來讀取 .spv 檔
	static std::vector<char> readFile(const std::string& filename){
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if(!file.is_open()){
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t) file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	// 用來產生shader module
	VkShaderModule createShaderModule(const std::vector<char>& code){
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS){
			throw std::runtime_error("failed to creat shader module!");
		}

		return shaderModule;
	}

	// 用來產生pipeline
	void createGraphicsPipeline(){
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};


		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}
};




int main(){
	HelloTriangleApplication app;

	try{
			app.run();
	}catch(const std::exception& e){
			std::cerr<<e.what()<<std::endl;
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

}
