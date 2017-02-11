// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/windowContext.hpp>
#include <ny/android/appContext.hpp>
#include <ny/log.hpp>

namespace ny {

AndroidWindowContext::AndroidWindowContext(AndroidAppContext& ac, const AndroidWindowSettings& ws)
	: appContext_(ac), nativeWindow_(ac.nativeWindow())
{
	if(ws.listener)
		listener(*ws.listener);
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
	warning("ny::AndroidWindowContext::show: has no capability");
}

void AndroidWindowContext::hide()
{
	warning("ny::AndroidWindowContext::show: has no capability");
}

void AndroidWindowContext::minSize(nytl::Vec2ui)
{
	warning("ny::AndroidWindowContext::minSize: has no capability");
}
void AndroidWindowContext::maxSize(nytl::Vec2ui)
{
	warning("ny::AndroidWindowContext::maxSize: has no capability");
}

void AndroidWindowContext::size(nytl::Vec2ui)
{
	warning("ny::AndroidWindowContext::size: has no capability");
}
void AndroidWindowContext::position(nytl::Vec2i)
{
	warning("ny::AndroidWindowContext::position: has no capability");
}

void AndroidWindowContext::cursor(const Cursor&)
{
	warning("ny::AndroidWindowContext::cursor: has no capability");
}
void AndroidWindowContext::refresh()
{
	if(!nativeWindow_) {
		warning("ny::AndroidWindowContext::refresh: no native window");
		return;
	}

	listener().draw({});
}

void AndroidWindowContext::maximize()
{
	warning("ny::AndroidWindowContext::maximize: has no capability");
}
void AndroidWindowContext::minimize()
{
	warning("ny::AndroidWindowContext::minimize: has no capability");
}
void AndroidWindowContext::fullscreen()
{
	warning("ny::AndroidWindowContext::fullscreen: has no capability");
}
void AndroidWindowContext::normalState()
{
	warning("ny::AndroidWindowContext::normalState: has no capability");
}

void AndroidWindowContext::beginMove(const EventData*)
{
	warning("ny::AndroidWindowContext::beginMove: has no capability");
}
void AndroidWindowContext::beginResize(const EventData*, WindowEdges)
{
	warning("ny::AndroidWindowContext::beginResize: has no capability");
}

void AndroidWindowContext::title(nytl::StringParam)
{
	warning("ny::AndroidWindowContext::title: has no capability");
}
void AndroidWindowContext::icon(const Image&)
{
	warning("ny::AndroidWindowContext::icon: has no capability");
}
void AndroidWindowContext::customDecorated(bool)
{
	warning("ny::AndroidWindowContext::customDecorated: has no capability");
}

void AndroidWindowContext::nativeWindow(ANativeWindow* nativeWindow)
{
	if(nativeWindow_ && nativeWindow)
		warning("ny::AndroidWindowContext::nativeWindow: already has native window");

	nativeWindow_ = nativeWindow;

	if(nativeWindow_)
		refresh();
}

} // namespace ny
