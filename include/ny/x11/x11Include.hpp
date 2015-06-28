#include <ny/include.hpp>

namespace ny
{
	class x11WindowContext;
	class x11AppContext;
	class x11ToplevelWindowContext;
	class x11ChildWindowContext;
	class x11CairoChildWindowContext;
	class x11CairoToplevelWindowContext;
	class x11CairoContext;

	typedef x11WindowContext x11WC;
	typedef x11AppContext x11AC;
	typedef x11ChildWindowContext x11ChildWC;
	typedef x11ToplevelWindowContext x11ToplevelWC;

#ifdef NY_WithGL
	class glxContext;
	class glxWindowContext;
	class glxToplevelWindowContext;
	class glxChildWindowContext;

	class x11EGLContext;
	class x11EGLWindowContext;
	class x11EGLToplevelWindowContext;
	class x11EGLChildWindowContext;
#endif //WithGL
}
