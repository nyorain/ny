#include "backends/wayland/defs.hpp"

#include "backends/wayland/appContext.hpp"
#include "backends/wayland/windowContext.hpp"
#include "backends/wayland/cairo.hpp"

#ifdef WithGL
#include "backends/wayland/gl.hpp"
#endif // WithGL

namespace ny
{

waylandAppContext* asWayland(appContext* c){ return dynamic_cast<waylandAppContext*>(c); };
waylandWindowContext* asWayland(windowContext* c){ return dynamic_cast<waylandWindowContext*>(c); };
waylandToplevelWindowContext* asWayland(toplevelWindowContext* c){ return dynamic_cast<waylandToplevelWindowContext*>(c); };
waylandChildWindowContext* asWayland(childWindowContext* c){ return dynamic_cast<waylandChildWindowContext*>(c); };
waylandChildWindowContext* asWaylandChild(windowContext* c){ return dynamic_cast<waylandChildWindowContext*>(c); };
waylandToplevelWindowContext* asWaylandToplevel(windowContext* c){ return dynamic_cast<waylandToplevelWindowContext*>(c); };

waylandCairoToplevelWindowContext* asWaylandCairo(toplevelWindowContext* c){ return dynamic_cast<waylandCairoToplevelWindowContext*>(c); };
waylandCairoChildWindowContext* asWaylandCairo(childWindowContext* c){ return dynamic_cast<waylandCairoChildWindowContext*>(c); };
waylandCairoContext* asWaylandCairo(windowContext* c){ return dynamic_cast<waylandCairoContext*>(c); };

#ifdef WithGL
waylandGLToplevelWindowContext* asWaylandGL(toplevelWindowContext* c){ return dynamic_cast<waylandGLToplevelWindowContext*>(c); };
waylandGLChildWindowContext* asWaylandGL(childWindowContext* c){ return dynamic_cast<waylandGLChildWindowContext*>(c); };
waylandGLContext* asWaylandGL(windowContext* c){ return dynamic_cast<waylandGLContext*>(c); };
#endif // WithGL

}
