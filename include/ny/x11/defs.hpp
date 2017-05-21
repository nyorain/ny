#pragma once

#include <cstdint>

namespace ny::x11 {

// All Motif WM Hints and structures.
constexpr unsigned long mwmDecoBorder = (1L << 1);
constexpr unsigned long mwmDecoResize = (1L << 2);
constexpr unsigned long mwmDecoTitle = (1L << 3);
constexpr unsigned long mwmDecomenu = (1L << 4);
constexpr unsigned long mwmDecominimize = (1L << 5);
constexpr unsigned long mwmDecomaximize = (1L << 5);
constexpr unsigned long mwmDecoAll = 1u;

constexpr unsigned long mwmFuncResize = mwmDecoBorder;
constexpr unsigned long mwmFuncmove = mwmDecoResize;
constexpr unsigned long mwmFuncminimize = mwmDecoTitle;
constexpr unsigned long mwmFuncmaximize = mwmDecomenu;
constexpr unsigned long mwmFuncClose = mwmDecominimize;
constexpr unsigned long mwmFuncAll = 1u;

constexpr unsigned long mwmHintsFunc = (1L << 0);
constexpr unsigned long mwmHintsDeco = mwmDecoBorder;
constexpr unsigned long mwmHintsInput = mwmDecoResize;
constexpr unsigned long mwmHintsStatus = mwmDecoTitle;

constexpr unsigned long mwmInputmodeless = (1L << 0);
constexpr unsigned long mwmInputPrimaryAppmodal = mwmDecoBorder;
constexpr unsigned long mwmInputSystemmodal = mwmDecoResize;
constexpr unsigned long mwmInputFullAppmodal = mwmDecoTitle;

constexpr unsigned long mwmTearoffWindow = (1L << 0);

constexpr unsigned long mwmInfoStartupStandard = (1L << 0);
constexpr unsigned long mwmInfoStartupCustom = mwmDecoBorder;

constexpr unsigned char moveResizeSizeTopLeft = 0;
constexpr unsigned char moveResizeSizeTop = 1;
constexpr unsigned char moveResizeSizeTopRight = 2;
constexpr unsigned char moveResizeSizeRight = 3;
constexpr unsigned char moveResizeSizeBottomRight = 4;
constexpr unsigned char moveResizeSizeBottom = 5;
constexpr unsigned char moveResizeSizeBottomLeft = 6;
constexpr unsigned char moveResizeSizeLeft = 7;
constexpr unsigned char moveResizemove = 8;
constexpr unsigned char moveResizeSizeKeyboard = 9;
constexpr unsigned char moveResizemoveKeyboard = 10;
constexpr unsigned char moveResizeCancel = 11;

struct MwmHints {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long inputMode;
	unsigned long status;
};

struct MwmInfo {
	long flags;
	std::uint32_t window;
};

} // namespace ny:x11
