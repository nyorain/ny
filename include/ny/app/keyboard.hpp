#pragma once

#include <ny/include.hpp>
#include <ny/backend/keyboard.hpp>

#include <nytl/callback.hpp>

namespace ny
{


///Represents a physical keyboard.
class Keyboard
{
public:
	Keyboard(App& app, KeyboardContext& context);
	~Keyboard() = default;

	bool pressed(Key key) const { return keyboardContext_.pressed(key); }
	std::string text(Key key) const { return keyboardContext_.text(key); }
	KeyboardContext& context() const { return keyboardContext_; }
	WindowContext* focus() const;

	///Will be called every time a key status changes.
	Callback<void(Key key, bool pressed)> onKey;

protected:
	KeyboardContext& keyboardContext_;
	App& app_;
};

}
