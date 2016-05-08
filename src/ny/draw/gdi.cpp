#include <ny/draw/gdi.hpp>
#include <ny/draw/font.hpp>
#include <ny/base/log.hpp>

#include <locale>
#include <codecvt>
#include <cstring>

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
	auto fontName = name;
	auto res = 1;

	if(fromFile)
	{
		res = AddFontResourceEx(name.c_str(), FR_PRIVATE, nullptr);
		if(!res)
		{
			warning("GdiFont: failed to load", name);
		}

		fontName = name.substr(name.find_last_of('/'), -1);
		fontName = fontName.substr(0, fontName.find_last_of('.'));
	}

	LOGFONT lf {};
	lf.lfHeight = 0;
	lf.lfWidth = 0;
	lf.lfWeight = FW_NORMAL;
	if(res) std::strcpy(lf.lfFaceName, fontName.c_str());

	handle_.reset(CreateFontIndirect(&lf));
}

//DrawContext
GdiDrawContext::GdiDrawContext(HDC xhdc) : hdc_(xhdc)
{
	::SetGraphicsMode(hdc(), GM_ADVANCED);
}

GdiDrawContext::~GdiDrawContext()
{
}

void GdiDrawContext::clear(const Brush& brush)
{
	auto c = brush.color();
	::SetDCBrushColor(hdc(), RGB(c.r, c.g, c.b));
	::SelectObject(hdc(), GetStockObject(NULL_PEN));
	::SelectObject(hdc(), GetStockObject(DC_BRUSH));
	::Rectangle(hdc(), 0, 0, 10000, 10000); ///XXX find correct dimensions
}
void GdiDrawContext::paint(const Brush& alphaMask, const Brush& brush)
{
	//TODO
}

void GdiDrawContext::gdiFill(const Path& obj, const Brush& brush)
{
	setTransform(obj);
	//TODO
}
void GdiDrawContext::gdiFill(const Text& obj, const Brush& brush)
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

	//SIZE extent;
	//GetTextExtentPoint32(hdc(), string.c_str(), string.size(), &extent);

	auto align = 0u;

	if(obj.horzBounds() == Text::HorzBounds::left) align |= TA_LEFT;
	else if(obj.horzBounds() == Text::HorzBounds::center) align |= TA_CENTER;
	else if(obj.horzBounds() == Text::HorzBounds::right) align |= TA_RIGHT;

	if(obj.vertBounds() == Text::VertBounds::top) align |= TA_TOP;
	else if(obj.vertBounds() == Text::VertBounds::middle) align |= TA_BASELINE;
	else if(obj.vertBounds() == Text::VertBounds::bottom) align |= TA_BOTTOM;

	auto c = brush.color();
	::SelectObject(hdc(), GetStockObject(NULL_PEN));
	::SelectObject(hdc(), GetStockObject(DC_BRUSH));
	::SetDCBrushColor(hdc(), RGB(c.r, c.g, c.b));
	::SelectObject(hdc(), famHandle->handle());
	::SetTextAlign(hdc(), align);

	auto pos = obj.position();
	::TextOut(hdc(), pos.x, pos.y, obj.string().c_str(), obj.string().size());
}
void GdiDrawContext::gdiFill(const Rectangle& obj, const Brush& brush)
{
	setTransform(obj);

	auto c = brush.color();
	::SelectObject(hdc(), GetStockObject(NULL_PEN));
	::SelectObject(hdc(), GetStockObject(DC_BRUSH));
	::SetDCBrushColor(hdc(), RGB(c.r, c.g, c.b));
	::Rectangle(hdc(), obj.position().x, obj.position().y, obj.size().x, obj.size().y);
}
void GdiDrawContext::gdiFill(const Circle& obj, const Brush& brush)
{
	setTransform(obj);

	auto c = brush.color();
	::SelectObject(hdc(), GetStockObject(NULL_PEN));
	::SelectObject(hdc(), GetStockObject(DC_BRUSH));
	::SetDCBrushColor(hdc(), RGB(c.r, c.g, c.b));

	auto start = obj.center() - obj.radius();
	auto end = obj.center() + obj.radius();
	::Ellipse(hdc(), start.x, start.y, end.x, end.y);
}

void GdiDrawContext::gdiStroke(const Path& obj, const Pen& pen)
{

}
void GdiDrawContext::gdiStroke(const Text& obj, const Pen& pen)
{

}
void GdiDrawContext::gdiStroke(const Rectangle& obj, const Pen& pen)
{

}
void GdiDrawContext::gdiStroke(const Circle& obj, const Pen& pen)
{

}

void GdiDrawContext::fillPreserve(const Brush& brush)
{
	for(auto& path : storedMask())
	{
		switch(path.type())
		{
			case PathBase::Type::text: gdiFill(path.text(), brush); break;
			case PathBase::Type::circle: gdiFill(path.circle(), brush); break;
			case PathBase::Type::rectangle: gdiFill(path.rectangle(), brush); break;
			case PathBase::Type::path: gdiFill(path.path(), brush); break;
		}
	}
}

void GdiDrawContext::strokePreserve(const Pen& pen)
{
	for(auto& path : storedMask())
	{
		switch(path.type())
		{
			case PathBase::Type::text: gdiStroke(path.text(), pen); break;
			case PathBase::Type::circle: gdiStroke(path.circle(), pen); break;
			case PathBase::Type::rectangle: gdiStroke(path.rectangle(), pen); break;
			case PathBase::Type::path: gdiStroke(path.path(), pen); break;
		}
	}
}

Rect2f GdiDrawContext::rectangleClip() const
{
	RECT ret;
	::GetClipBox(hdc(), &ret);
	return Rect2f(ret.left, ret.top, ret.right - ret.left, ret.bottom - ret.top);
}
void GdiDrawContext::clipRectangle(const Rect2f& obj)
{
	auto region = GdiPointer<HRGN>(CreateRectRgn(obj.left(), obj.top(), obj.right(), obj.bottom()));
	SelectClipRgn(hdc(), region.get());
}
void GdiDrawContext::resetRectangleClip()
{
	//SetMetaRgn(hdc());
	SelectClipRgn(hdc(), nullptr);
}

void GdiDrawContext::setTransform(const Transform2& transform)
{
	setTransform(transform.transformMatrix());
}

void GdiDrawContext::setTransform(const Mat3f& m)
{
	XFORM xform{m[0][0], m[1][0], m[0][1], m[1][1], m[0][2], m[1][2]};
	SetWorldTransform(hdc(), &xform);
}

void GdiDrawContext::resetTransform()
{
	XFORM xform{1, 0, 0, 1, 0, 0};
	SetWorldTransform(hdc(), &xform);
}

}
