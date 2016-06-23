#include <ny/backend/x11/backend.hpp>
#include <ny/config.hpp>

#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/appContext.hpp>

#ifdef NY_WithCairo
  #include <ny/backend/x11/cairo.hpp>
#endif //Cairo

#include <X11/Xlib.h>
#include <memory>

namespace ny
{

X11Backend X11Backend::instance_;

X11Backend::X11Backend()
{
}

bool X11Backend::available() const
{
    //XInitThreads(); //todo, make this optional
    Display* dpy = XOpenDisplay(nullptr);
    if(!dpy) return 0;
    XCloseDisplay(dpy);

    return 1;
}

std::unique_ptr<AppContext> X11Backend::createAppContext()
{
    return std::make_unique<X11AppContext>();
}

std::unique_ptr<WindowContext> 
X11Backend::createWindowContext(AppContext& ctx, const WindowSettings& s)
{
	//window settings
    X11WindowSettings settings;
    const X11WindowSettings* ws = dynamic_cast<const X11WindowSettings*>(&s);

    if(ws)
    {
        settings = *ws;
    }
    else
    {
		settings.WindowSettings::operator=(s);
    }

	//appContext
	auto xac = dynamic_cast<X11AppContext*>(&ctx);
	if(!xac)
	{
		throw std::logic_error("ny::X11Backend: trying to create window with invalid appContext");
	}

	//type
	if(s.draw == DrawType::vulkan)
	{
		#ifdef NY_WithVulkan
		 return nullptr;
		#endif
	}
	else if(s.draw == DrawType::opengl)
	{
		#ifdef NY_WithGL	
		 //return std::make_unique<X11GlWindowContext>(*xac, settings);
		#else
		 throw std::logic_error("ny::X11Backend::createWC: cannot match draw type");
		#endif
	}
	else if(s.draw == DrawType::software || s.draw == DrawType::dontCare)
	{
		#ifdef NY_WithCairo
		 return std::make_unique<X11CairoWindowContext>(*xac, settings);
		#else
		 throw std::logic_error("ny::X11Backend::createWC: cannot match draw type");
		#endif
	}

	return std::make_unique<X11WindowContext>(*xac, settings);
}

}
