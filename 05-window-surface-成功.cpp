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

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 800;

// 宣告要使用的驗證層有哪些
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

//#define NDEBUG

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
	
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	
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


		std::cout << "needed extensions:" << std::endl;
		for(auto it = extensions.begin(); it != extensions.end(); it++){
			std::cout << *it << std::endl;
		}
		std::cout << std::endl;


		auto supportExtensions = querySupportExtensions();
		std::cout << "avaiable extensions:" << std::endl;
		for(const auto& supportExtension : supportExtensions){
			std::cout << supportExtension.extensionName << std::endl;
		}



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

		return indices.isComplete();
	}


	// 存放隊列族索引的結構
	struct QueueFamilyIndices{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete(){
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
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

	// 尋找可用的 gpu
	void pickPhysicalDevice(){
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
		createInfo.enabledExtensionCount = 0;

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
