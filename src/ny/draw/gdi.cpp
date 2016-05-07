#include <ny/draw/gdi.hpp>
#include <ny/draw/font.hpp>
#include <ny/base/log.hpp>

#include <locale>
#include <codecvt>
using namespace Gdiplus;

namespace ny
{

std::wstring toUTF16(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

//Font
GdiFontHandle::GdiFontHandle(const Font& font) : GdiFontHandle(font.name(), font.fromFile())
{
}

GdiFontHandle::GdiFontHandle(const std::string& name, bool fromFile)
{
	if(fromFile) {
		int found;

		collection_.AddFontFile(toUTF16(name).c_str());

		handle_.reset(new FontFamily());
		collection_.GetFamilies(1, handle_.get(), &found);
		if(found < 1) {
			warning("Gdi+: Failed to load font from ", name);
			handle_.reset(new FontFamily(L"Times New Roman"));
			return;
		}
	} else {
		handle_.reset(new FontFamily(toUTF16(name).c_str()));
	}
}

GdiFontHandle::GdiFontHandle(const GdiFontHandle& other)
{
	handle_.reset(other.handle().Clone());
}
GdiFontHandle& GdiFontHandle::operator=(const GdiFontHandle& other)
{
handle_.reset(other.handle().Clone());
}

//DrawContext
GdiDrawContext::GdiDrawContext(Gdiplus::Image& gdiimage)
{
	graphics_.reset(new Graphics(&gdiimage));
}

GdiDrawContext::GdiDrawContext(HDC hdc)
{
	hdc_ = hdc;
	//graphics_.reset(new Graphics(hdc));
}

GdiDrawContext::GdiDrawContext(HDC hdc, HANDLE handle)
{
	graphics_.reset(new Graphics(hdc, handle));
}

GdiDrawContext::GdiDrawContext(HWND window, bool adjust)
{
	graphics_.reset(new Graphics(window, adjust));
}

GdiDrawContext::~GdiDrawContext()
{
}

void GdiDrawContext::clear(const Brush& b)
{
	auto c = b.color();
	//graphics().Clear(Gdiplus::Color(c.r, c.g, c.b, c.a));
	//SelectObject(hdc_, GetStockObject(WHITE_BRUSH));
	//SetDCBrushColor(hdc_, RGB(c.r, c.g, c.b));
	//::Rectangle(hdc_, 0, 0, 500, 500);

	::RECT rect {0, 0, 2000, 1000};
	auto brush = CreateSolidBrush(RGB(100, 100, 100));
	FillRect(hdc_, &rect, brush);
}
void GdiDrawContext::paint(const Brush& alphaMask, const Brush& brush)
{
	//TODO
}

void GdiDrawContext::gdiFill(const Path& obj, const Gdiplus::Brush& brush)
{
	setTransform(obj);
	//TODO
	graphics().ResetTransform();
}
void GdiDrawContext::gdiFill(const Text& obj, const Gdiplus::Brush& brush)
{
	if(!obj.font()) return;

	setTransform(obj);

	auto string = toUTF16(obj.string());
	auto famHandle = static_cast<GdiFontHandle*>(obj.font()->cache("ny::GdiFontHandle"));
	if(!famHandle)
	{
		auto cache = std::make_unique<GdiFontHandle>(*obj.font());
		famHandle = &obj.font()->cache("ny::GdiFontHandle", std::move(cache));
	}

	Gdiplus::Font font(&famHandle->handle(), obj.size(), FontStyleRegular, UnitPixel);

	RectF extent;
	graphics().MeasureString(string.c_str(), -1, &font, {0.f, 0.f}, &extent);

	PointF pos {obj.position().x , obj.position().y};

	if(obj.horzBounds() == Text::HorzBounds::center) pos.X -= extent.Width / 2;
	else if(obj.horzBounds() == Text::HorzBounds::right) pos.X -= extent.Width;

	if(obj.vertBounds() == Text::VertBounds::middle) pos.Y -= extent.Height / 2;
	else if(obj.vertBounds() == Text::VertBounds::bottom) pos.Y -= extent.Height;

	graphics().DrawString(string.c_str(), -1, &font, pos, &brush);
	graphics().ResetTransform();
}
void GdiDrawContext::gdiFill(const Rectangle& obj, const Gdiplus::Brush& brush)
{
	setTransform(obj);
	RectF rect(obj.position().x, obj.position().y, obj.size().x, obj.size().y);
	graphics().FillRectangle(&brush, rect);
	graphics().ResetTransform();
}
void GdiDrawContext::gdiFill(const Circle& obj, const Gdiplus::Brush& brush)
{
	setTransform(obj);
	auto start = obj.center() - obj.radius();
	auto end = obj.center() + obj.radius();
	graphics().FillEllipse(&brush, start.x, start.y, end.x, end.y);
	graphics().ResetTransform();
}

void GdiDrawContext::gdiStroke(const Path& obj, const Gdiplus::Pen& pen)
{

}
void GdiDrawContext::gdiStroke(const Text& obj, const Gdiplus::Pen& pen)
{

}
void GdiDrawContext::gdiStroke(const Rectangle& obj, const Gdiplus::Pen& pen)
{

}
void GdiDrawContext::gdiStroke(const Circle& obj, const Gdiplus::Pen& pen)
{

}

void GdiDrawContext::fillPreserve(const Brush& brush)
{
	return;

	auto col = brush.color();
	Gdiplus::SolidBrush bb(Gdiplus::Color(col.a, col.r, col.g, col.b));

	for(auto& path : storedMask())
	{
		switch(path.type())
		{
			case PathBase::Type::text: gdiFill(path.text(), bb); break;
			case PathBase::Type::circle: gdiFill(path.circle(), bb); break;
			case PathBase::Type::rectangle: gdiFill(path.rectangle(), bb); break;
			case PathBase::Type::path: gdiFill(path.path(), bb); break;
		}
	}
}

void GdiDrawContext::strokePreserve(const Pen& pen)
{
	auto col = pen.brush().color();
	Gdiplus::SolidBrush bb(Gdiplus::Color(col.a, col.r, col.g, col.b));
	Gdiplus::Pen pp(&bb, pen.width());

	for(auto& path : storedMask())
	{
		switch(path.type())
		{
			case PathBase::Type::text: gdiStroke(path.text(), pp); break;
			case PathBase::Type::circle: gdiStroke(path.circle(), pp); break;
			case PathBase::Type::rectangle: gdiStroke(path.rectangle(), pp); break;
			case PathBase::Type::path: gdiStroke(path.path(), pp); break;
		}
	}
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

void GdiDrawContext::setTransform(const Transform2& transform)
{
	setTransform(transform.transformMatrix());
}

void GdiDrawContext::setTransform(const Mat3f& m)
{
	Gdiplus::Matrix matrix(m[0][0], m[1][0], m[0][1], m[1][1], m[0][2], m[1][2]);
	graphics().SetTransform(&matrix);
}

}
