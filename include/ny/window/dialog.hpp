#pragma once

#include <ny/window/toplevel.hpp>

namespace ny
{

class Dialog : public ToplevelWindow
{
public:
	enum class Result
	{
		ok,
		abort
	};

public:
	virtual Result modal();
};

class FileDialog : public Dialog
{
public:
	enum class Options
	{
		save,
		open,
		mustExist
	};

public:
	FileDialog(Options options);
	std::string path() const;
};

class ColorDialog : public Dialog
{
public:
	Color color() const;
};

class MessageBox : public Dialog
{
public:
	enum class Button
	{
		ok,
		cancel,
		yes,
		no,
		help
	};

	using ButtonFlags = Flags<Button>;
	constexpr auto yesNo = Button::yes | Button::no;
	constexpr auto okCancel = Button::ok | Button::cancel;

public:
	MessageBox(const std::string& text, ButtonFlags buttons);
	Button resultButton() const;
};

NYTL_ENABLE_ENUM_OPS(ny::MessageBox::Button);

}
