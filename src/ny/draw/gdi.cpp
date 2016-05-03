#include <ny/draw/gdi.hpp>

#include <locale>
#include <codecvt>
using namespace Gdiplus;

namespace ny
{

GdiDrawContext::GdiDrawContext(Gdiplus::Image& gdiimage) : graphics_(&gdiimage)
{
}

GdiDrawContext::GdiDrawContext(HDC hdc) : graphics_(hdc)
{
}

GdiDrawContext::GdiDrawContext(HDC hdc, HANDLE handle) : graphics_(hdc, handle)
{
}

GdiDrawContext::GdiDrawContext(HWND window, bool adjust) : graphics_(window, adjust)
{
}

GdiDrawContext::~GdiDrawContext()
{
}

void GdiDrawContext::clear(const Brush& b)
{
	auto c = b.color();
	graphics_.Clear(Gdiplus::Color(c.r, c.g, c.b, c.a));
}
void GdiDrawContext::paint(const Brush& alphaMask, const Brush& brush)
{
	//TODO
}

void GdiDrawContext::mask(const Path& obj)
{
	//TODO
}
void GdiDrawContext::mask(const Text& obj)
{
	Gdiplus::FontFamily family(L"Times New Roman");
	PointF pos {obj.position().x , obj.position().y};

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	auto string = converter.from_bytes(obj.string());

	currentPath_.AddString(string.c_str(), -1, &family, 0, 48, pos, nullptr);
}
void GdiDrawContext::mask(const Rectangle& obj)
{
	RectF rect(obj.position().x, obj.position().y, obj.size().x, obj.size().y);
	currentPath_.AddRectangle(rect);
}
void GdiDrawContext::mask(const Circle& obj)
{
	auto start = obj.center() - obj.radius();
	auto end = obj.center() + obj.radius();
	currentPath_.AddEllipse(start.x, start.y, end.x, end.y);
}
void GdiDrawContext::resetMask()
{
	currentPath_.Reset();
}

void GdiDrawContext::fill(const Brush& brush)
{
	auto col = brush.color();
	Gdiplus::SolidBrush bb(Gdiplus::Color(col.a, col.r, col.g, col.b));
	graphics().FillPath(&bb, &currentPath_);
	resetMask();
}
void GdiDrawContext::fillPreserve(const Brush& brush)
{
	auto col = brush.color();
	Gdiplus::SolidBrush bb(Gdiplus::Color(col.a, col.r, col.g, col.b));
	graphics().FillPath(&bb, &currentPath_);
}
void GdiDrawContext::stroke(const Pen& pen)
{
	auto col = pen.brush().color();
	Gdiplus::SolidBrush bb(Gdiplus::Color(col.a, col.r, col.g, col.b));
	Gdiplus::Pen pp(&bb, pen.width());
	graphics().DrawPath(&pp, &currentPath_);
	resetMask();
}
void GdiDrawContext::strokePreserve(const Pen& pen)
{
	auto col = pen.brush().color();
	Gdiplus::SolidBrush bb(Gdiplus::Color(col.a, col.r, col.g, col.b));
	Gdiplus::Pen pp(&bb, pen.width());
	graphics().DrawPath(&pp, &currentPath_);
}

Rect2f GdiDrawContext::rectangleClip() const
{
	RectF ret;
	graphics().GetClipBounds(&ret);
	return {ret.X, ret.Y, ret.Width, ret.Height};
}
void GdiDrawContext::clipRectangle(const Rect2f& obj)
{
	RectF rect(obj.position.x, obj.position.y, obj.size.x, obj.size.y);
	graphics().SetClip(rect);
}
void GdiDrawContext::resetRectangleClip()
{
	graphics().ResetClip();
}

}
