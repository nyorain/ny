#pragma once

#include <ny/include.hpp>
#include <ny/window/toplevel.hpp>

namespace ny
{

///Dialog base class.
///A Dialog is some temporary window that may collect some data from the user.
class Dialog : public ToplevelWindow
{
public:
	virtual DialogResult modal();
	virtual DialogResult result();
};

///Dialog for getting a file path.
///May be used for save/open operations as well as generally letting the user enter a filepath.
class FileDialog : public Dialog
{
public:
	enum class Options
	{
		save,
		open,
		mustExist,
		directory,
		allExtensions
	};

public:
	FileDialog(Options options, const std::string& path, const std::string& extension);
	std::string path() const;
};

///Dialog for picking a color.
class ColorDialog : public Dialog
{
public:
	Color color() const;
};

///MessageBox that can be used to inform the user about some event.
///Stores which button was clicked.
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
