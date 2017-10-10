// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt
// Copyright (c) 2017 nyorain

#include <ny/android/appContext.hpp>
#include <ny/android/windowContext.hpp>
#include <ny/android/bufferSurface.hpp>
#include <ny/android/input.hpp>
#include <ny/android/activity.hpp>
#include <dlg/dlg.hpp>

#ifdef NY_WithVulkan
 #define VK_USE_PLATFORM_ANDROID_KHR
 #include <ny/android/vulkan.hpp>
 #include <vulkan/vulkan.h>
#endif // Vulkan

#ifdef NY_WithEgl
 #include <ny/common/egl.hpp>
 #include <ny/android/egl.hpp>
#endif // Egl

#include <android/native_activity.h>
#include <android/input.h>
#include <jni.h>

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

/// NOTE: The most complex part of this implementation is the synchronization between
///   activity callback and main (event) thread.
///	  We will receive activity events such as window created/destroyed/redraw/size as
///	  callbacks in the activity thread and have to process them in the main event thread.
///   Therefore the impl structure holds an interal event queue, mutex and condition variable.
///   For some events, we also have to make sure to not return until the event
///   was processed. There we use the condition variable and an atomic flag.
///
/// It is rather important to NOT lock the activity mutex in any functions
///    that might be called by the application. Otherwise it might be locked
///    during initialization which would result in a deadlock since the activity thread
///    waits during that phase that we process its events. See e.g. savedState().

namespace ny {

struct AndroidAppContext::Impl {
	std::queue<android::ActivityEvent> activityEvents;
	std::mutex eventQueueMutex;
	std::condition_variable eventQueueCV;

#ifdef NY_WithEgl
	bool eglFailed {};
	EglSetup eglSetup {};
#endif //WithEGL
};

// AndroidAppContext
AndroidAppContext::AndroidAppContext(android::Activity& activity)
	: activity_(activity)
{
	static const std::string funcName = "ny::AndroidAppContext: ";
	impl_ = std::make_unique<Impl>();
	looper_ = ALooper_prepare(0);

	// check for valid activity
	// also register this as appContext
	// after this block the callbacks will be called
	{
		std::lock_guard<std::mutex>(activity_.mutex());
		if(!activity_.activity_) {
			throw std::runtime_error(funcName + "native activity already invalid");
		}

		if(activity_.appContext_) {
			throw std::logic_error(funcName + "there can only be one android AppContext");
		}

		activity_.appContext_ = this;
		nativeActivity_ = activity_.nativeActivity();
		nativeWindow_ = activity_.nativeWindow();
		inputQueue_ = activity_.inputQueue();
	}

	// attach this thread to the java vm to get
	// the JNIEnv object for making java calls
	JavaVMAttachArgs attachArgs;
	attachArgs.version = JNI_VERSION_1_6;
	attachArgs.name = "NativeThread:ny";
	attachArgs.group = NULL;

	auto result = nativeActivity_->vm->AttachCurrentThread(&jniEnv_, &attachArgs);
	if(result == JNI_ERR) {
		dlg_warn("attaching the thread to the java vm failed");
		jniEnv_ = nullptr;
	}

	mouseContext_ = std::make_unique<AndroidMouseContext>(*this);
	keyboardContext_ = std::make_unique<AndroidKeyboardContext>(*this);

	initQueue();
}

AndroidAppContext::~AndroidAppContext()
{
	// clear the event queue
	// this will also unblock the activity thread
	{
		std::lock_guard<std::mutex> lock(impl_->eventQueueMutex);
		while(!impl_->activityEvents.empty()) {
			auto event = impl_->activityEvents.front();
			impl_->activityEvents.pop();

			if(event.signalCV)
				event.signalCV->store(true);
		}
	}

	// notify waiting activity thread
	impl_->eventQueueCV.notify_all();

	{
		std::lock_guard<std::mutex>(activity_.mutex());
		activity_.appContext_ = nullptr;
	}

	keyboardContext_ = {};
	mouseContext_ = {};

	if(jniEnv_) {
		if(!nativeActivity_) {
			dlg_warn("ny::~AndroidAppContext: jniEnv_ but not no native activity.");
		} else {
			nativeActivity_->vm->DetachCurrentThread();
		}
	}

	if(inputQueue_) {
		AInputQueue_detachLooper(inputQueue_);
	}
}

WindowContextPtr AndroidAppContext::createWindowContext(const WindowSettings& settings)
{
	const static std::string funcName = "ny::AndroidAppContext::createWindowContext: ";

	if(windowContext_)
		throw std::logic_error(funcName + " android can only create one windowContext");

	AndroidWindowSettings androidSettings;
	const auto* ws = dynamic_cast<const AndroidWindowSettings*>(&settings);

	if(ws) androidSettings = *ws;
	else androidSettings.WindowSettings::operator=(settings);

	std::unique_ptr<AndroidWindowContext> ret;
	if(settings.surface == SurfaceType::vulkan) {
		#ifdef NY_WithVulkan
			ret = std::make_unique<AndroidVulkanWindowContext>(*this, androidSettings);
		#else
			throw std::logic_error(funcName + "ny was built without vulkan support");
		#endif
	} else if(settings.surface == SurfaceType::gl) {
		#ifdef NY_WithEgl
			if(!eglSetup()) throw std::runtime_error(funcName + "initializing egl failed");
			ret = std::make_unique<AndroidEglWindowContext>(*this, *eglSetup(), androidSettings);
		#else
			throw std::logic_error(funcName + "ny was built without egl support");
		#endif
	} else if(settings.surface == SurfaceType::buffer) {
		ret = std::make_unique<AndroidBufferWindowContext>(*this, androidSettings);
	} else if(settings.surface == SurfaceType::none) {
		ret = std::make_unique<AndroidWindowContext>(*this, androidSettings);
	}

	windowContext_ = ret.get();
	return std::move(ret);
}

KeyboardContext* AndroidAppContext::keyboardContext()
{
	return keyboardContext_.get();
}

MouseContext* AndroidAppContext::mouseContext()
{
	return mouseContext_.get();
}

void AndroidAppContext::pollEvents()
{
	checkActivity();
	handleActivityEvents();
	checkActivity();

	int outFd, outEvents;
	void* outData;
	auto ret = ALooper_pollAll(0, &outFd, &outEvents, &outData);
	if(ret == ALOOPER_POLL_ERROR) {
		dlg_warn("ALooper_pollOnce: I/O error");
	}

	checkActivity();
}

void AndroidAppContext::waitEvents()
{
	checkActivity();
	handleActivityEvents();
	checkActivity();

	int outFd, outEvents;
	void* outData;

	auto ret = ALooper_pollAll(-1, &outFd, &outEvents, &outData);
	if(ret == ALOOPER_POLL_ERROR) {
		dlg_warn("ALooper_pollAll: I/O error");
	}

	checkActivity();
}

void AndroidAppContext::wakeupWait()
{
	ALooper_wake(looper_);
}

std::vector<const char*> AndroidAppContext::vulkanExtensions() const
{
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif
}

GlSetup* AndroidAppContext::glSetup() const
{
	#ifdef NY_WithEgl
		return eglSetup();
	#else
		return nullptr;
	#endif // WithEgl
}

EglSetup* AndroidAppContext::eglSetup() const
{
	#ifdef NY_WithEgl
		if(impl_->eglFailed) return nullptr;

		if(!impl_->eglSetup.valid()) {
			try { impl_->eglSetup = {nullptr}; }
			catch(const std::exception& error) {
				dlg_warn("creating eglSetup failed: ", error.what());
				impl_->eglFailed = true;
				impl_->eglFailed = {};
				return nullptr;
			}
		}

		return &impl_->eglSetup;

	#else
		return nullptr;
	#endif // WithEgl
}

bool AndroidAppContext::showKeyboard() const
{
	if(!nativeActivity_)
		return false;

	ANativeActivity_showSoftInput(nativeActivity_, ANATIVEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT);
	return true;
}

bool AndroidAppContext::hideKeyboard() const
{
	if(!nativeActivity_)
		return false;

	ANativeActivity_hideSoftInput(nativeActivity_, ANATIVEACTIVITY_HIDE_SOFT_INPUT_NOT_ALWAYS);
	return true;
}

const char* AndroidAppContext::internalPath() const
{
	if(!nativeActivity_)
		return "";

	return nativeActivity_->internalDataPath;
}

const char* AndroidAppContext::externalPath() const
{
	if(!nativeActivity_)
		return "";

	return nativeActivity_->externalDataPath;
}

void AndroidAppContext::stateSaver(const std::function<void*(std::size_t&)>& saver)
{
	stateSaver_ = saver;
}

std::vector<uint8_t> AndroidAppContext::savedState()
{
	// we can safely access (even move from) this since it is only touched on
	// activity creation by someone else than this thread
	// it is important on the other hand to not use a mutex here
	// since this function might be called while the activity thread is
	// waiting for us to process its events
	return std::move(activity().savedState_);
}

// private interface
void AndroidAppContext::inputReceived()
{
	if(!inputQueue_) {
		dlg_warn("invalid input queue");
		return;
	}

	if(!nativeActivity_) {
		dlg_warn("no native activity");
		return;
	}

	auto ret = 0;
	while((ret = AInputQueue_hasEvents(inputQueue_)) > 0) {
		AInputEvent* event {};
		ret = AInputQueue_getEvent(inputQueue_, &event);
		if(ret < 0) {
			dlg_warn("getEvent returned error code {}", ret);
			continue;
		}

		ret = AInputQueue_preDispatchEvent(inputQueue_, event);
		if(ret != 0) {
			continue;
		}

		auto handled = false;
		auto type = AInputEvent_getType(event);
		if(type == AINPUT_EVENT_TYPE_KEY) {
			handled = keyboardContext_->process(*event);
		} else if(type == AINPUT_EVENT_TYPE_MOTION) {
			handled = mouseContext_->process(*event);
		}

		AInputQueue_finishEvent(inputQueue_, event, handled);
	}

	if(ret < 0) {
		dlg_warn("input queue returned error code {}", ret);
		return;
	}
}

void AndroidAppContext::handleActivityEvents()
{
	using android::ActivityEventType;

	std::unique_lock<std::mutex> lock(impl_->eventQueueMutex);
	while(!impl_->activityEvents.empty() && nativeActivity_) {
		auto event = impl_->activityEvents.front();
		impl_->activityEvents.pop();
		lock.unlock();

		ANativeWindow* window;
		AInputQueue* queue;
		nytl::Vec2i32 size;
		void** dataPtr;
		std::size_t* sizePtr;

		// TODO: handle (e.g. expose public callbacks) for start/stop/pause/resume functions

		switch(event.type) {
			case ActivityEventType::windowCreated:
				std::memcpy(&window, event.data, sizeof(window));
				windowCreated(*window);
				break;
			case ActivityEventType::queueCreated:
				std::memcpy(&queue, event.data, sizeof(queue));
				inputQueueCreated(*queue);
				break;
			case ActivityEventType::windowDestroyed:
				windowDestroyed();
				break;
			case ActivityEventType::queueDestroyed:
				inputQueueDestroyed();
				break;
			case ActivityEventType::windowRedraw:
				windowRedrawNeeded();
				break;
			case ActivityEventType::windowFocus:
				windowFocusChanged(event.data[0]);
				break;
			case ActivityEventType::windowResize:
				std::memcpy(&size, event.data, sizeof(size));
				windowResized(size);
				break;
			case ActivityEventType::destroy:
				activityDestroyed();
				break;
			case ActivityEventType::saveState:
				std::memcpy(&dataPtr, event.data, sizeof(dataPtr));
				std::memcpy(&sizePtr, event.data + sizeof(dataPtr), sizeof(sizePtr));
				if(!dataPtr || !sizePtr) {
					dlg_warn("invalid stateSave event");
				} else if(stateSaver_) {
					*dataPtr = stateSaver_(*sizePtr);
				}
				break;
			default:
				break;
		}

		if(event.signalCV) {
			*event.signalCV = true;
			impl_->eventQueueCV.notify_one();
		}

		lock.lock();
	}
}

void AndroidAppContext::initQueue()
{
	if(inputQueue_) {
		auto inputCallback = [](int, int, void* data) {
			auto ac = static_cast<AndroidAppContext*>(data);
			ac->inputReceived();
			return 1;
		};

		constexpr auto inputIndent = 1u;
		AInputQueue_attachLooper(inputQueue_, looper_, inputIndent,
			inputCallback, static_cast<void*>(this));
	}
}

void AndroidAppContext::checkActivity()
{
	if(!nativeActivity_) {
		throw std::runtime_error("Native activity was destoyed");
	}
}

// the following two functions are only called from the activity thread
void AndroidAppContext::pushEvent(const android::ActivityEvent& ev) noexcept
{
	try {
		std::lock_guard<std::mutex> lock(impl_->eventQueueMutex);
		impl_->activityEvents.push(ev);
		ALooper_wake(looper_);
	} catch(const std::exception& error) {
		dlg_error("ny::AndroidAppContext::pushEvent: ", error.what());
	}
}

void AndroidAppContext::pushEventWait(android::ActivityEvent ev) noexcept
{
	try {
		std::atomic<bool> flag;
		ev.signalCV = &flag;

		std::unique_lock<std::mutex> lock(impl_->eventQueueMutex);
		impl_->activityEvents.push(ev);
		ALooper_wake(looper_);
		impl_->eventQueueCV.wait(lock, [&]{ return flag.load(); });
	} catch(const std::exception& error) {
		dlg_error("ny::AndroidAppContext::pushEventWait: ", error.what());
	}
}

void AndroidAppContext::windowContextDestroyed()
{
	windowContext_ = {};
}

void AndroidAppContext::windowFocusChanged(bool gained)
{
	if(!windowContext_)
		return;

	ny::FocusEvent focus;
	focus.gained = gained;
	windowContext_->listener().focus(focus);
}
void AndroidAppContext::windowResized(nytl::Vec2i32 size)
{
	if(!windowContext_) {
		return;
	}

	if(size[0] <= 0 || size[1] <= 0) {
		dlg_warn("invalid size of resize event");
		return;
	}

	SizeEvent se;
	se.size = static_cast<nytl::Vec2ui>(size);
	windowContext_->listener().resize(se);
}
void AndroidAppContext::windowRedrawNeeded()
{
	if(!windowContext_) {
		return;
	}

	windowContext_->listener().draw({});
}
void AndroidAppContext::windowCreated(ANativeWindow& window)
{
	nativeWindow_ = &window;
	if(windowContext_) {
		windowContext_->nativeWindow(&window);
	}
}
void AndroidAppContext::windowDestroyed()
{
	nativeWindow_ = nullptr;
	if(windowContext_) {
		windowContext_->nativeWindow(nullptr);
	}
}
void AndroidAppContext::inputQueueCreated(AInputQueue& queue)
{
	inputQueue_ = &queue;
	initQueue();
}
void AndroidAppContext::inputQueueDestroyed()
{
	if(!inputQueue_) {
		dlg_warn("invalid input queue");
		return;
	}

	AInputQueue_detachLooper(inputQueue_);
	inputQueue_ = nullptr;
}
void AndroidAppContext::activityDestroyed()
{
	if(!nativeActivity_) {
		dlg_warn("there was no activity");
		return;
	}

	if(jniEnv_) {
		nativeActivity_->vm->DetachCurrentThread();
		jniEnv_ = nullptr;
	}

	nativeActivity_ = nullptr;
	mouseContext_ = {};
	keyboardContext_ = {};
}

} // namespace ny
