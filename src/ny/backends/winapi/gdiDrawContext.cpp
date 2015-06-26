#include "gdiDrawContext.hpp"

#include "utils.hpp"

namespace ny
{

gdiDrawContext::gdiDrawContext(winapiWindowContext* wc) : drawContext( *((surface2*)wc->getWindow()) ), m_painting(0), m_hdc(0), m_graphics(0), m_windowContext(wc)
{

}

void gdiDrawContext::clear(color col)
{
    if(!m_painting)
        return;

    m_graphics->Clear(colorToWinapi(col));
}

void gdiDrawContext::beginDraw()
{
    m_hdc = BeginPaint(m_windowContext->getHandle(), &m_ps);
    m_graphics = new Graphics(m_hdc);

    m_painting = 1;
}

void gdiDrawContext::finishDraw()
{
    EndPaint(m_windowContext->getHandle(), &m_ps);

    delete m_graphics;

    m_graphics = 0;
    m_hdc = 0;

    m_painting = 0;
}

}
