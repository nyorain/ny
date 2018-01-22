// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/windowContext.hpp>
#include <ny/android/appContext.hpp>
#include <dlg/dlg.hpp>

namespace ny {

AndroidWindowContext::AndroidWindowContext(AndroidAppContext& ac, const AndroidWindowSettings& ws)
	: appContext_(ac), nativeWindow_(ac.nativeWindow())
{
	if(ws.listener) {
		listener(*ws.listener);
	}
}

AndroidWindowContext::~AndroidWindowContext()
{
	appContext_.windowContextDestroyed();
}

Surface AndroidWindowContext::surface()
{
	return {};
}

WindowCapabilities AndroidWindowContext::capabilities() const
{
	return WindowCapability::serverDecoration;
}

void AndroidWindowContext::show()
{
	dlg_warn("show: not supported");
}

void AndroidWindowContext::hide()
{
	dlg_warn("show: not supported");
}

void AndroidWindowContext::minSize(nytl::Vec2ui)
{
	dlg_warn("minSize: not supported");
}
void AndroidWindowContext::maxSize(nytl::Vec2ui)
{
	dlg_warn("maxSize: not supported");
}

void AndroidWindowContext::size(nytl::Vec2ui)
{
	dlg_warn("size: not supported");
}
void AndroidWindowContext::position(nytl::Vec2i)
{
	dlg_warn("position: not supported");
}

void AndroidWindowContext::cursor(const Cursor&)
{
	dlg_warn("cursor: not supported");
}
void AndroidWindowContext::refresh()
{
	if(!nativeWindow_) {
		dlg_warn("refresh: not supported");
		return;
	}

	// TODO: redraw deferred
	// listener().draw({});
}

void AndroidWindowContext::maximize()
{
	dlg_warn("maximize: not supported");
}
void AndroidWindowContext::minimize()
{
	dlg_warn("minimize: not supported");
}
void AndroidWindowContext::fullscreen()
{
	dlg_warn("fullscreen: not supported");
}
void AndroidWindowContext::normalState()
{
	dlg_warn("normalState: not supported");
}

void AndroidWindowContext::beginMove(const EventData*)
{
	dlg_warn("beginMove: not supported");
}
void AndroidWindowContext::beginResize(const EventData*, WindowEdges)
{
	dlg_warn("beginResize: not supported");
}

void AndroidWindowContext::title(const char*)
{
	dlg_warn("title: not supported");
}
void AndroidWindowContext::icon(const Image&)
{
	dlg_warn("icon: not supported");
}
void AndroidWindowContext::customDecorated(bool)
{
	dlg_warn("customDecorated: not supported");
}

void AndroidWindowContext::nativeWindow(ANativeWindow* nativeWindow)
{
	if(nativeWindow_ && nativeWindow) {
		dlg_warn("nativeWindow: already has native window");
	}

	nativeWindow_ = nativeWindow;
}

} // namespace ny
