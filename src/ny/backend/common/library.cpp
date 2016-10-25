#include <ny/backend/common/library.hpp>

#ifdef _WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

namespace ny
{

namespace detail
{

#ifdef WIN32
	void* dlopen(const char* name, std::error_code* error)
	{
		return ::LoadLibrary(name);
	}

	void dlclose(void* handle)
	{
		if(!handle) return;
		::FreeLibrary(static_cast<HMODULE>(handle));
	}

	void* dlsym(void* handle, const char* name)
	{
		if(!handle) return nullptr;
		return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle), name));
	}

#else
	void* dlopen(const char* name, std::error_code* error)
	{
		return ::dlopen(name);
	}

	void dlclose(void* handle)
	{
		if(!handle) return;
		::dlclose(handle);
	}

	void* dlsym(void* handle, const char* name)
	{
		if(!handle) return nullptr;
		return dlsym(handle, name);
	}
#endif

}

Library::Library(nytl::StringParam name, std::error_code* error)
{
	handle_ = detail::dlopen(name, error);
}

Library::~Library()
{
	detail::dlclose(handle_);
}

Library::Library(Library&& other) noexcept : handle_(other.handle_)
{
	other.handle_ = nullptr;
}

Library& Library::operator=(Library&& other) noexcept
{
	detail::dlclose(handle_);
	handle_ = other.handle_;
	other.handle_ = nullptr;
}

void* Library::symbol(nytl::StringParam name) const
{
	return detail::dlsym(handle_, name);
}

}
