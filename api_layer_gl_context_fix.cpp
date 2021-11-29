#include "loader_interfaces.h"

#include <cstring>
#include <iostream>

static const char *gl_api_string = nullptr;

class GLApi
{
public:
	virtual void
	make_current() = 0;
	virtual ~GLApi(){};
};

const char *_layerName = "gl_context_fix";
#define log std::cout << _layerName << ":: "

#define XR_USE_GRAPHICS_API_OPENGL

// #define VERBOSE

#ifdef __linux__

// Required headers for windowing, as well as the XrGraphicsBindingOpenGLXlibKHR struct.
#include <X11/Xlib.h>
#include <GL/glx.h>

#define XR_USE_PLATFORM_XLIB

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

class GLXApi : public GLApi
{
	struct XrGraphicsBindingOpenGLXlibKHR binding;

public:
	GLXApi(struct XrGraphicsBindingOpenGLXlibKHR *glx_binding)
	{
		// copy values in case clients frees binding info
		this->binding = *glx_binding;
		log << "GLX graphics binding: display " << binding.xDisplay << ", drawable " << binding.glxDrawable
		    << ", context " << binding.glxContext << std::endl;
	}

	void
	make_current() override
	{
		glXMakeCurrent(binding.xDisplay, None, NULL);
		glXMakeCurrent(binding.xDisplay, binding.glxDrawable, binding.glxContext);
	}
};
#else
#error Only linux supported
#endif

GLApi *gl_api;

static PFN_xrCreateSession _nextxrCreateSession;
static PFN_xrEndFrame _nextxrEndFrame;
static PFN_xrCreateSwapchain _nextxrCreateSwapchain;
static PFN_xrAcquireSwapchainImage _nextxrAcquireSwapchainImage;
static PFN_xrWaitSwapchainImage _nextxrWaitSwapchainImage;
static PFN_xrReleaseSwapchainImage _nextxrReleaseSwapchainImage;

// load next function pointers in _xrCreateApiLayerInstance
PFN_xrGetInstanceProcAddr _nextXrGetInstanceProcAddr = NULL;

// cache create infos
static XrInstanceCreateInfo instanceInfo;
static XrInstance xrInstance;


static XRAPI_ATTR XrResult XRAPI_CALL
_xrCreateSession(XrInstance instance, const XrSessionCreateInfo *createInfo, XrSession *session)
{
	log << __FUNCTION__ << std::endl;

	const XrBaseInStructure *s = (const XrBaseInStructure *)createInfo;
	while (s) {
		if (s->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR) {
			gl_api = new GLXApi((struct XrGraphicsBindingOpenGLXlibKHR *)s);
		}
		s = s->next;
	}

	XrResult res = _nextxrCreateSession(instance, createInfo, session);
	return res;
}


static XRAPI_ATTR XrResult XRAPI_CALL
_xrEndFrame(XrSession session, const XrFrameEndInfo *frameEndInfo)
{
#ifdef VERBOSE
	log << __FUNCTION__ << std::endl;
#endif

	XrResult res = _nextxrEndFrame(session, frameEndInfo);
	if (gl_api)
		gl_api->make_current();
	return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrCreateSwapchain(XrSession session, const XrSwapchainCreateInfo *createInfo, XrSwapchain *swapchain)
{
#ifdef VERBOSE
	log << __FUNCTION__ << std::endl;
#endif

	XrResult res = _nextxrCreateSwapchain(session, createInfo, swapchain);
	if (gl_api)
		gl_api->make_current();
	return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrAcquireSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageAcquireInfo *acquireInfo, uint32_t *index)
{
#ifdef VERBOSE
	log << __FUNCTION__ << std::endl;
#endif

	XrResult res = _nextxrAcquireSwapchainImage(swapchain, acquireInfo, index);
	if (gl_api)
		gl_api->make_current();
	return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo *waitInfo)
{
#ifdef VERBOSE
	log << __FUNCTION__ << std::endl;
#endif

	XrResult res = _nextxrWaitSwapchainImage(swapchain, waitInfo);
	if (gl_api)
		gl_api->make_current();
	return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrReleaseSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageReleaseInfo *releaseInfo)
{
#ifdef VERBOSE
	log << __FUNCTION__ << std::endl;
#endif

	XrResult res = _nextxrReleaseSwapchainImage(swapchain, releaseInfo);
	if (gl_api)
		gl_api->make_current();
	return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrGetInstanceProcAddr(XrInstance instance, const char *name, PFN_xrVoidFunction *function)
{
	// log << ": " << name << std::endl;

	std::string func_name = name;

	if (func_name == "xrCreateSession") {
		*function = (PFN_xrVoidFunction)_xrCreateSession;
		return XR_SUCCESS;
	}

	if (func_name == "xrEndFrame") {
		*function = (PFN_xrVoidFunction)_xrEndFrame;
		return XR_SUCCESS;
	}

	if (func_name == "xrCreateSwapchain") {
		*function = (PFN_xrVoidFunction)_xrCreateSwapchain;
		return XR_SUCCESS;
	}

	if (func_name == "xrAcquireSwapchainImage") {
		*function = (PFN_xrVoidFunction)_xrAcquireSwapchainImage;
		return XR_SUCCESS;
	}

	if (func_name == "xrWaitSwapchainImage") {
		*function = (PFN_xrVoidFunction)_xrWaitSwapchainImage;
		return XR_SUCCESS;
	}

	if (func_name == "xrxrReleaseSwapchainImage") {
		*function = (PFN_xrVoidFunction)_xrReleaseSwapchainImage;
		return XR_SUCCESS;
	}


	return _nextXrGetInstanceProcAddr(instance, name, function);
}

// xrCreateInstance is a special case that we can't hook. We get this amended call instead.
static XrResult XRAPI_PTR
_xrCreateApiLayerInstance(const XrInstanceCreateInfo *info,
                          const XrApiLayerCreateInfo *apiLayerInfo,
                          XrInstance *instance)
{
	_nextXrGetInstanceProcAddr = apiLayerInfo->nextInfo->nextGetInstanceProcAddr;
	XrResult result;


	// first let the instance be created
	result = apiLayerInfo->nextInfo->nextCreateApiLayerInstance(info, apiLayerInfo, instance);
	if (XR_FAILED(result)) {
		log << "Failed to load xrCreateActionSet" << std::endl;
		return result;
	}

	// remember which opengl extension is enabled
	for (uint32_t i = 0; i < info->enabledExtensionCount; i++) {
		if (strcmp(info->enabledExtensionNames[i], XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) == 0) {
			log << "graphics binding: " << XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
			gl_api_string = XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
		}
	}


	// then use the created instance to load next function pointers
	result = _nextXrGetInstanceProcAddr(*instance, "xrCreateSession", (PFN_xrVoidFunction *)&_nextxrCreateSession);
	if (XR_FAILED(result)) {
		log << "Failed to load xrCreateSession" << std::endl;
		return result;
	}


	// the 5 functions that need to be fixed up https://developer.blender.org/T92723#1257606
	result = _nextXrGetInstanceProcAddr(*instance, "xrEndFrame", (PFN_xrVoidFunction *)&_nextxrEndFrame);
	if (XR_FAILED(result)) {
		log << "Failed to load xrEndFrame" << std::endl;
		return result;
	}

	result =
	    _nextXrGetInstanceProcAddr(*instance, "xrCreateSwapchain", (PFN_xrVoidFunction *)&_nextxrCreateSwapchain);
	if (XR_FAILED(result)) {
		log << "Failed to load xrCreateSwapchain" << std::endl;
		return result;
	}

	result = _nextXrGetInstanceProcAddr(*instance, "xrAcquireSwapchainImage",
	                                    (PFN_xrVoidFunction *)&_nextxrAcquireSwapchainImage);
	if (XR_FAILED(result)) {
		log << "Failed to load xrAcquireSwapchainImage" << std::endl;
		return result;
	}

	result = _nextXrGetInstanceProcAddr(*instance, "xrWaitSwapchainImage",
	                                    (PFN_xrVoidFunction *)&_nextxrWaitSwapchainImage);
	if (XR_FAILED(result)) {
		log << "Failed to load xrWaitSwapchainImage" << std::endl;
		return result;
	}

	result = _nextXrGetInstanceProcAddr(*instance, "xrReleaseSwapchainImage",
	                                    (PFN_xrVoidFunction *)&_nextxrReleaseSwapchainImage);
	if (XR_FAILED(result)) {
		log << "Failed to load xrReleaseSwapchainImage" << std::endl;
		return result;
	}


	instanceInfo = *info;
	xrInstance = *instance;
	log << ": Created api layer instance for app " << info->applicationInfo.applicationName << std::endl;

	return result;
}

extern "C" {

XrResult
xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo *loaderInfo,
                                   const char *layerName,
                                   XrNegotiateApiLayerRequest *apiLayerRequest)
{
	_layerName = strdup(layerName);

	log << ": Using API layer: " << layerName << std::endl;

	log << ": loader API version min: " << XR_VERSION_MAJOR(loaderInfo->minApiVersion) << "."
	    << XR_VERSION_MINOR(loaderInfo->minApiVersion) << "." << XR_VERSION_PATCH(loaderInfo->minApiVersion) << "."
	    << " max: " << XR_VERSION_MAJOR(loaderInfo->maxApiVersion) << "."
	    << XR_VERSION_MINOR(loaderInfo->maxApiVersion) << "." << XR_VERSION_PATCH(loaderInfo->maxApiVersion) << "."
	    << std::endl;

	log << ": loader interface version min: " << XR_VERSION_MAJOR(loaderInfo->minInterfaceVersion) << "."
	    << XR_VERSION_MINOR(loaderInfo->minInterfaceVersion) << "."
	    << XR_VERSION_PATCH(loaderInfo->minInterfaceVersion) << "."
	    << " max: " << XR_VERSION_MAJOR(loaderInfo->maxInterfaceVersion) << "."
	    << XR_VERSION_MINOR(loaderInfo->maxInterfaceVersion) << "."
	    << XR_VERSION_PATCH(loaderInfo->maxInterfaceVersion) << "." << std::endl;



	// TODO: proper version check
	apiLayerRequest->layerInterfaceVersion = loaderInfo->maxInterfaceVersion;
	apiLayerRequest->layerApiVersion = loaderInfo->maxApiVersion;
	apiLayerRequest->getInstanceProcAddr = _xrGetInstanceProcAddr;
	apiLayerRequest->createApiLayerInstance = _xrCreateApiLayerInstance;

	return XR_SUCCESS;
}
}
