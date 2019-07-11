#include <ny/backend.hpp> // ny::Backend
#include <ny/appContext.hpp> // ny::AppContext
#include <ny/windowContext.hpp> // ny::WindowContext
#include <ny/windowListener.hpp> // ny::WindowListener
#include <ny/windowSettings.hpp> // ny::WindowSettings
#include <ny/keyboardContext.hpp> // ny::KeyboardContext
#include <ny/bufferSurface.hpp> // ny::BufferSurface
#include <ny/key.hpp> // ny::Keycode
#include <ny/mouseButton.hpp> // ny::MouseButton
#include <ny/image.hpp> // ny::Image
#include <ny/event.hpp> // ny::*Event
#include <dlg/dlg.hpp> // logging

#include <nytl/vecOps.hpp> // print nytl::Vec
#include <cairo.h>
#include <cstring> // std::memset

// NOTE: include examples taken from cairo:
// see https://github.com/freedesktop/cairo for license information
// drawMode = 1: /doc/tutorial/src/twin.c
// drawMode = 2: /doc/tutorial/src/singular.c

// Use 0, 1 and 2 keys to toggle what is drawn
// - d: try to toggle server decorations
// - f: toggle fullscreen
// - m: toggle maximized state
// - i: iconize (minimize) the window
// - n: reset to normal toplevel state
// - Escape: close the window
class MyWindowListener : public ny::WindowListener {
public:
	ny::AppContext* appContext {};
	ny::WindowContext* windowContext {};
	ny::BufferSurface* bufferSurface {};
	ny::ToplevelState toplevelState = ny::ToplevelState::normal;
	nytl::Vec2ui windowSize {};
	bool* run {};
	int drawMode {1};

public:
	void draw(const ny::DrawEvent&) override;
	void mouseButton(const ny::MouseButtonEvent&) override;
	void mouseWheel(const ny::MouseWheelEvent&) override;
	void key(const ny::KeyEvent&) override;
	void state(const ny::StateEvent&) override;
	void close(const ny::CloseEvent&) override;
	void resize(const ny::SizeEvent&) override;
	void focus(const ny::FocusEvent&) override;
	void touchBegin(const ny::TouchBeginEvent&) override;
	void touchUpdate(const ny::TouchUpdateEvent&) override;
	void touchEnd(const ny::TouchEndEvent&) override;
	void touchCancel(const ny::TouchCancelEvent&) override;
	void surfaceCreated(const ny::SurfaceCreatedEvent&) override;
	void surfaceDestroyed(const ny::SurfaceDestroyedEvent&) override;
};

int main(int, char**) {
	// The same setup as in the first (intro) example
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	auto listener = MyWindowListener {};

	ny::BufferSurface* bufferSurface {};
	auto ws = ny::WindowSettings {};

	ws.listener = &listener;
	ws.surface = ny::SurfaceType::buffer;
	ws.buffer.storeSurface = &bufferSurface;
	ws.initState = listener.toplevelState;
	ws.transparent = true;
	auto wc = ac->createWindowContext(ws);

	auto run = true;
	listener.appContext = ac.get();
	listener.windowContext = wc.get();
	listener.bufferSurface = bufferSurface;
	listener.appContext = ac.get();
	listener.run = &run;

	dlg_info("Entering main loop");
	while(run) {
		ac->waitEvents();
	}
	dlg_info("Returning from main with grace");
}

static void
get_singular_values (const cairo_matrix_t *matrix,
		     double *major,
		     double *minor)
{
    double xx = matrix->xx, xy = matrix->xy;
    double yx = matrix->yx, yy = matrix->yy;

    double a = xx*xx+yx*yx;
    double b = xy*xy+yy*yy;
    double k = xx*xy+yx*yy;

    double f = (a+b) * .5;
    double g = (a-b) * .5;
    double delta = sqrt (g*g + k*k);

    if (major)
	*major = sqrt (f + delta);
    if (minor)
	*minor = sqrt (f - delta);
}

static void
get_pen_axes (cairo_t *cr,
	      double *major,
	      double *minor)
{
    double width;
    cairo_matrix_t matrix;

    width = cairo_get_line_width (cr);
    cairo_get_matrix (cr, &matrix);

    get_singular_values (&matrix, major, minor);

    if (major)
	*major *= width;
    if (minor)
	*minor *= width;
}

void MyWindowListener::draw(const ny::DrawEvent&) {
	if(!bufferSurface) {
		dlg_info("draw: no bufferSurface");
		return;
	}

	static int i = 0;
	dlg_debug("draw {}", ++i);

{
	auto guard = bufferSurface->buffer();
	auto image = guard.get();

	// TODO: support other formats?
	dlg_assert(image.format == ny::ImageFormat::argb8888);
	auto data = reinterpret_cast<unsigned char*>(image.data);
	auto surface = cairo_image_surface_create_for_data(data,
		CAIRO_FORMAT_ARGB32, image.size.x, image.size.y,
		image.stride / 8);

	auto cr = cairo_create(surface);
	cairo_set_source_rgba(cr, 0.8, 0.9, 0.6, 0.8);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);

	if(drawMode == 1) {
		// just shows all chars in different sizes
		cairo_set_source_rgb (cr, 0, 0, 0);
		cairo_select_font_face (cr, "@cairo:",
			CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

		unsigned char s[2] = {0, 0};
		auto h = 2;
		for (auto i = 8; i < 48; i >= 24 ? i+=3 : i++) {
			cairo_set_font_size (cr, i);
			for (auto j = 33; j < 128; j++) {
				if (j == 33 || (j == 80 && i > 24)) {
					h += i + 2;
					cairo_move_to (cr, 10, h);
				}
				s[0] = j;
				cairo_show_text (cr, (const char *) s);
			}
		}
	} else if(drawMode == 2) {
		double major_width, minor_width;
		auto W = windowSize.x;
		auto H = windowSize.y;
		auto B = (windowSize.x + windowSize.y) / 16;

		/* the spline we want to stroke */
		cairo_move_to  (cr, W-B, B);
		cairo_curve_to (cr, -W,   B,
					2*W, H-B,
				B,   H-B);

		/* the effect is show better with round caps */
		cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

		/* set the skewed pen */
		cairo_rotate (cr, +.7);
		cairo_scale  (cr, .5, 2.);
		cairo_rotate (cr, -.7);
		cairo_set_line_width (cr, B);

		get_pen_axes (cr, &major_width, &minor_width);

		/* stroke with "major" pen in translucent red */
		cairo_save (cr);
		cairo_identity_matrix (cr);
		cairo_set_line_width (cr, major_width);
		cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, .9);
		cairo_stroke_preserve (cr);
		cairo_restore (cr);

		/* stroke with skewed pen in translucent black */
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, .9);
		cairo_stroke_preserve (cr);

		/* stroke with "minor" pen in translucent yellow */
		cairo_save (cr);
		cairo_identity_matrix (cr);
		cairo_set_line_width (cr, minor_width);
		cairo_set_source_rgba (cr, 1.0, 1.0, 0.0, .9);
		cairo_stroke_preserve (cr);
		cairo_restore (cr);

		/* stroke with hairline in black */
		cairo_save (cr);
		cairo_identity_matrix (cr);
		cairo_set_line_width (cr, 1);
		cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
		cairo_stroke_preserve (cr);
		cairo_restore (cr);
	} else if(drawMode == 3) {
		auto w = float(windowSize.x / 6);
		auto h = windowSize.y;

		cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
		cairo_rectangle(cr, 0 * w, 0, 1 * w, h);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
		cairo_rectangle(cr, 1 * w, 0, 2 * w, h);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 1.0);
		cairo_rectangle(cr, 2 * w, 0, 3 * w, h);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, 1.0);
		cairo_rectangle(cr, 3 * w, 0, 4 * w, h);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 1.0);
		cairo_rectangle(cr, 4 * w, 0, 5 * w, h);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 1.0, 0.0, 1.0, 1.0);
		cairo_rectangle(cr, 5 * w, 0, 6 * w, h);
		cairo_fill(cr);
	} else if(drawMode == 4) {
		auto offx = 20.f;
		auto offy = 20.f;
		auto y = offy;

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);

		cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.5);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, 10.f);
		cairo_move_to(cr, offx, y);
		cairo_line_to(cr, offx + windowSize.x - offx, y);
		cairo_stroke(cr);
		y += 10 + offy;

		cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.5);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, 10.f);
		cairo_move_to(cr, offx, y);
		cairo_line_to(cr, offx + windowSize.x - offx, y);
		cairo_stroke(cr);
		y += 10 + offy;

		cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.5);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, 10.f);
		cairo_move_to(cr, offx, y);
		cairo_line_to(cr, offx + windowSize.x - offx, y);
		cairo_stroke(cr);
		y += 10 + offy;

		cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, 0.5);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, 10.f);
		cairo_move_to(cr, offx, y);
		cairo_line_to(cr, offx + windowSize.x - offx, y);
		cairo_stroke(cr);
		y += 10 + offy;

		cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.5);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, 10.f);
		cairo_move_to(cr, offx, y);
		cairo_line_to(cr, offx + windowSize.x - offx, y);
		cairo_stroke(cr);
		y += 10 + offy;

		cairo_set_source_rgba(cr, 1.0, 0.0, 1.0, 0.5);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, 10.f);
		cairo_move_to(cr, offx, y);
		cairo_line_to(cr, offx + windowSize.x - offx, y);
		cairo_stroke(cr);
		y += 10 + offy;
	}

	cairo_destroy(cr);
	cairo_surface_flush(surface);
	cairo_surface_destroy(surface);
}

	// schedule next frame
	// windowContext->frameCallback();
	windowContext->refresh();
}

void MyWindowListener::key(const ny::KeyEvent& keyEvent) {
	std::string name = "<unknown>";
	if(appContext->keyboardContext()) {
		auto utf8 = appContext->keyboardContext()->utf8(keyEvent.keycode);
		if(!utf8.empty() && !ny::specialKey(keyEvent.keycode)) name = utf8;
		else name = "<unprintable>";
	}

	auto utf8 = (keyEvent.utf8.empty() || ny::specialKey(keyEvent.keycode)) ?
		"<unprintable>" : keyEvent.utf8;
	dlg_info("Key {} with keycode ({}: {}) {}, generating: {} {}", name,
		(unsigned int) keyEvent.keycode, ny::name(keyEvent.keycode),
		keyEvent.pressed ? "pressed" : "released", utf8,
		keyEvent.repeat ? "(repeated)" : "");

	if(keyEvent.pressed) {
		auto keycode = keyEvent.keycode;
		if(keycode == ny::Keycode::f) {
			dlg_info("Toggling fullscreen");
			if(toplevelState != ny::ToplevelState::fullscreen) {
				windowContext->fullscreen();
				toplevelState = ny::ToplevelState::fullscreen;
			} else {
				windowContext->normalState();
				toplevelState = ny::ToplevelState::normal;
			}
		} else if(keycode == ny::Keycode::n) {
			dlg_info("Resetting window to normal state");
			toplevelState = ny::ToplevelState::normal;
			windowContext->normalState();
		} else if(keycode == ny::Keycode::escape) {
			dlg_info("Closing window and exiting");
			*run = false;
		} else if(keycode == ny::Keycode::m) {
			dlg_info("Toggle window maximize");
			if(toplevelState != ny::ToplevelState::maximized) {
				windowContext->maximize();
				toplevelState = ny::ToplevelState::maximized;
			} else {
				windowContext->normalState();
				toplevelState = ny::ToplevelState::normal;
			}
		} else if(keycode == ny::Keycode::i) {
			dlg_info("Minimizing window");
			toplevelState = ny::ToplevelState::minimized;
			windowContext->minimize();
		} else if(keycode == ny::Keycode::d) {
			dlg_info("Trying to toggle decorations");
			windowContext->customDecorated(!windowContext->customDecorated());
			windowContext->refresh();
		} else if(keycode == ny::Keycode::k0) {
			dlg_info("DrawMode 0");
			drawMode = 0;
			windowContext->refresh();
		} else if(keycode == ny::Keycode::k1) {
			dlg_info("DrawMode 1");
			drawMode = 1;
			windowContext->refresh();
		} else if(keycode == ny::Keycode::k2) {
			dlg_info("DrawMode 2");
			drawMode = 2;
			windowContext->refresh();
		} else if(keycode == ny::Keycode::k3) {
			dlg_info("DrawMode 3");
			drawMode = 3;
			windowContext->refresh();
		} else if(keycode == ny::Keycode::k4) {
			dlg_info("DrawMode 4");
			drawMode = 4;
			windowContext->refresh();
		}
	}
}

void MyWindowListener::close(const ny::CloseEvent&) {
	dlg_info("Window was closed by server side. Exiting");
	*run = false;
}

void MyWindowListener::mouseButton(const ny::MouseButtonEvent& event) {
	dlg_info("mouseButton {} {} at {}", ny::mouseButtonName(event.button),
		event.pressed ? "pressed" : "released", event.position);
	if(event.pressed && event.button == ny::MouseButton::left) {
		if(toplevelState != ny::ToplevelState::normal ||
				event.position[0] < 0 || event.position[1] < 0 ||
				static_cast<unsigned int>(event.position[0]) > windowSize[0] ||
				static_cast<unsigned int>(event.position[1]) > windowSize[1]) {
			dlg_info("Not resizing: {} {}", event.position, int(toplevelState));
			return;
		}

		ny::WindowEdges resizeEdges = ny::WindowEdge::none;
		if(event.position[0] < 100) {
			resizeEdges |= ny::WindowEdge::left;
		} else if(static_cast<unsigned int>(event.position[0]) > windowSize[0] - 100) {
			resizeEdges |= ny::WindowEdge::right;
		}

		if(event.position[1] < 100) {
			resizeEdges |= ny::WindowEdge::top;
		} else if(static_cast<unsigned int>(event.position[1]) > windowSize[1] - 100) {
			resizeEdges |= ny::WindowEdge::bottom;
		}

		auto caps = windowContext->capabilities();
		if(resizeEdges != ny::WindowEdge::none && caps & ny::WindowCapability::beginResize) {
			dlg_info("Starting to resize window");
			windowContext->beginResize(event.eventData, resizeEdges);
		} else if(caps & ny::WindowCapability::beginMove) {
			dlg_info("Starting to move window");
			windowContext->beginMove(event.eventData);
		}
	}
}

void MyWindowListener::mouseWheel(const ny::MouseWheelEvent& ev) {
	dlg_info("mouse wheel: {}", ev.value);
}

void MyWindowListener::focus(const ny::FocusEvent& ev) {
	dlg_info("focus: {}", ev.gained);
}

void MyWindowListener::state(const ny::StateEvent& stateEvent) {
	dlg_info("window state changed: {}", (int) stateEvent.state);
	toplevelState = stateEvent.state;
}

void MyWindowListener::resize(const ny::SizeEvent& sizeEvent) {
	dlg_info("window resized to {}", sizeEvent.size);
	windowSize = sizeEvent.size;
}

void MyWindowListener::touchBegin(const ny::TouchBeginEvent& ev) {
	dlg_info("Touch begin: {} at {}", ev.id, ev.pos);
}

void MyWindowListener::touchUpdate(const ny::TouchUpdateEvent& ev) {
	dlg_info("Touch update: {} at {}", ev.id, ev.pos);
}

void MyWindowListener::touchEnd(const ny::TouchEndEvent& ev) {
	dlg_info("Touch end: {} at {}", ev.id, ev.pos);
}

void MyWindowListener::touchCancel(const ny::TouchCancelEvent&) {
	dlg_info("Touch cancel");
}

void MyWindowListener::surfaceCreated(const ny::SurfaceCreatedEvent& se) {
	bufferSurface = se.surface.buffer;
	windowContext->refresh();
}

void MyWindowListener::surfaceDestroyed(const ny::SurfaceDestroyedEvent&) {
	bufferSurface = nullptr;
}

