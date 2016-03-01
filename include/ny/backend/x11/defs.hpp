#pragma once

#include <cstdint>

namespace ny
{

namespace x11
{

constexpr unsigned long MwmDecoBorder = (1L << 1);
constexpr unsigned long MwmDecoResize = (1L << 2);
constexpr unsigned long MwmDecoTitle = (1L << 3);
constexpr unsigned long MwmDecoMenu = (1L << 4);
constexpr unsigned long MwmDecoMinimize = (1L << 5);
constexpr unsigned long MwmDecoMaximize = (1L << 5);
constexpr unsigned long MwmDecoAll = 
	MwmDecoBorder | MwmDecoResize | MwmDecoTitle | MwmDecoMenu | MwmDecoMinimize | MwmDecoMaximize;

constexpr unsigned long MwmFuncResize = MwmDecoBorder;
constexpr unsigned long MwmFuncMove = MwmDecoResize;
constexpr unsigned long MwmFuncMinimize = MwmDecoTitle;
constexpr unsigned long MwmFuncMaximize = MwmDecoMenu;
constexpr unsigned long MwmFuncClose = MwmDecoMinimize;
constexpr unsigned long MwmFuncAll = 
	MwmFuncResize | MwmFuncMove | MwmFuncMaximize | MwmFuncMinimize | MwmFuncClose;

constexpr unsigned long MwmHintsFunc = (1L << 0);
constexpr unsigned long MwmHintsDeco = MwmDecoBorder;
constexpr unsigned long MwmHintsInput = MwmDecoResize;
constexpr unsigned long MwmHintsStatus = MwmDecoTitle;

constexpr unsigned long MwmInputModeless = (1L << 0);
constexpr unsigned long MwmInputPrimaryAppModal = MwmDecoBorder; //MWM_INPUT_PRIMARY_APPLICATION_MODAL
constexpr unsigned long MwmInputSystemModal = MwmDecoResize;
constexpr unsigned long MwmInputFullAppModal = MwmDecoTitle; //MWM_INPUT_FULL_APPLICATION_MODAL

constexpr unsigned long MwmTearoffWindow = (1L << 0);

constexpr unsigned long MwmInfoStartupStandard = (1L << 0);
constexpr unsigned long MwmInfoStartupCustom = MwmDecoBorder;

constexpr unsigned char MoveResizeSizeTopLeft = 0;
constexpr unsigned char MoveResizeSizeTop = 1;
constexpr unsigned char MoveResizeSizeTopRight = 2;
constexpr unsigned char MoveResizeSizeRight = 3;
constexpr unsigned char MoveResizeSizeBottomRight = 4;
constexpr unsigned char MoveResizeSizeBottom = 5;
constexpr unsigned char MoveResizeSizeBottomLeft = 6;
constexpr unsigned char MoveResizeSizeLeft = 7;
constexpr unsigned char MoveResizeMove = 8;
constexpr unsigned char MoveResizeSizeKeyboard = 9;
constexpr unsigned char MoveResizeMoveKeyboard = 10;
constexpr unsigned char MoveResizeCancel = 11;

struct MwmHints
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long inputMode;
    unsigned long status;
};

struct MwmInfo
{
    long flags;
	std::uint32_t wm_window;
};

}

}

