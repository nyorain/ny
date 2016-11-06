#include <ny/android/backend.hpp>
#include <ny/android/appContext.hpp>

namespace ny
{

AndroidBackend AndroidBackend::instance_;

AppContextPtr AndroidBackend::createAppContext()
{
	return AndroidAppContext();
}

}
