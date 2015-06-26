#include <ny/backends/x11/defs.hpp>

#include <ny/backends/x11/appContext.hpp>
#include <ny/backends/x11/windowContext.hpp>
#include <ny/backends/x11/cairo.hpp>

#ifdef NY_WithGL
#include <ny/backends/x11/glx.hpp>
#include <ny/backends/x11/egl.hpp>
#endif // NY_WithGL

namespace ny
{

x11AppContext* asX11(appContext* c){ return dynamic_cast<x11AppContext*>(c); };
x11WindowContext* asX11(windowContext* c){ return dynamic_cast<x11WindowContext*>(c); };
x11ToplevelWindowContext* asX11(toplevelWindowContext* c){ return dynamic_cast<x11ToplevelWindowContext*>(c); };
x11ChildWindowContext* asX11(childWindowContext* c){ return dynamic_cast<x11ChildWindowContext*>(c); };
x11ChildWindowContext* asX11Child(windowContext* c){ return dynamic_cast<x11ChildWindowContext*>(c); };
x11ToplevelWindowContext* asX11Toplevel(windowContext* c){ return dynamic_cast<x11ToplevelWindowContext*>(c); };

x11CairoToplevelWindowContext* asX11Cairo(toplevelWindowContext* c){ return dynamic_cast<x11CairoToplevelWindowContext*>(c); };
x11CairoChildWindowContext* asX11Cairo(childWindowContext* c){ return dynamic_cast<x11CairoChildWindowContext*>(c); };
x11CairoContext* asX11Cairo(windowContext* c){ return dynamic_cast<x11CairoContext*>(c); };

#ifdef NY_WithGL
glxToplevelWindowContext* asGLX(toplevelWindowContext* c){ return dynamic_cast<glxToplevelWindowContext*>(c); };
glxChildWindowContext* asGLX(childWindowContext* c){ return dynamic_cast<glxChildWindowContext*>(c); };
glxWindowContext* asGLX(windowContext* c){ return dynamic_cast<glxWindowContext*>(c); };
glxContext* asGLX(glContext* c){ return dynamic_cast<glxContext*>(c); };

x11EGLToplevelWindowContext* asX11EGL(toplevelWindowContext* c){ return dynamic_cast<x11EGLToplevelWindowContext*>(c); };
x11EGLChildWindowContext* asX11EGL(childWindowContext* c){ return dynamic_cast<x11EGLChildWindowContext*>(c); };
x11EGLContext* asX11EGL(windowContext* c){ return dynamic_cast<x11EGLContext*>(c); };
#endif // NY_WithGL


namespace x11
{
Atom WindowDelete = 0;
Atom MwmHints = 0;

Atom State = 0;
Atom StateMaxVert = 0;
Atom StateMaxHorz = 0;
Atom StateFullscreen = 0;
Atom StateHidden = 0;
Atom StateModal = 0;
Atom StateFocused = 0;
Atom StateDemandAttention = 0;
Atom StateAbove = 0;
Atom StateBelow = 0;
Atom StateShaded = 0;
Atom StateSticky = 0;
Atom StateSkipPager = 0;
Atom StateSkipTaskbar = 0;

Atom Type = 0;
Atom TypeDesktop = 0;
Atom TypeDock = 0;
Atom TypeToolbar = 0;
Atom TypeMenu = 0;
Atom TypeUtility = 0;
Atom TypeSplash = 0;
Atom TypeDialog = 0;
Atom TypeDropdownMenu = 0;
Atom TypePopupMenu = 0;
Atom TypeTooltip = 0;
Atom TypeNotification = 0;
Atom TypeCombo = 0;
Atom TypeDnd = 0;
Atom TypeNormal = 0;

Atom AllowedActions = 0; //allowedallowedActions
Atom AllowedActionMinimize = 0;
Atom AllowedActionMaximizeVert = 0;
Atom AllowedActionMaximizeHorz = 0;
Atom AllowedActionMove = 0;
Atom AllowedActionResize = 0;
Atom AllowedActionClose = 0;
Atom AllowedActionFullscreen = 0;
Atom AllowedActionBelow = 0;
Atom AllowedActionAbove = 0;
Atom AllowedActionShade = 0;
Atom AllowedActionStick = 0;
Atom AllowedActionChangeDesktop = 0;

Atom FrameExtents = 0;
Atom Strut = 0;
Atom StrutPartial = 0;
Atom Desktop = 0;
Atom MoveResize = 0;

Atom DndEnter = 0;
Atom DndPosition = 0;
Atom DndStatus = 0;
Atom DndTypeList = 0;
Atom DndActionCopy = 0;
Atom DndDrop = 0;
Atom DndLeave = 0;
Atom DndFinished = 0;
Atom DndSelection = 0;
Atom DndProxy = 0;
Atom DndAware = 0;

Atom Primary = 0;
Atom Clipboard = 0;
Atom Targets = 0;

Atom TypeImagePNG = 0;
Atom TypeImageJPG = 0;
Atom TypeImageBMP = 0;
Atom TypeImageTIFF = 0;
Atom TypeTextUri = 0;
Atom TypeTextUriList = 0;
Atom TypeText = 0;
Atom TypeTextPlain = 0;
Atom TypeUTF8 = 0;

/////////////////////////////////////////////////

property getWindowProperty(Window w, Atom prop)
{
    if(!getXDisplay() || !w || !prop) return property();
    Atom actualType;
	int actualFormat;
	unsigned long count;
	unsigned long bytesAfter;
	unsigned char *data = nullptr;
	int readBytes = 1024;

	do
	{
		if(data) XFree(data);
		XGetWindowProperty(getXDisplay(), w, prop, 0, readBytes, False, AnyPropertyType, &actualType, &actualFormat, &count, &bytesAfter, &data);
        readBytes *= 2;

	} while(bytesAfter);

	return {data, (unsigned int)actualFormat, (unsigned int)count, actualType};
}

}

}
