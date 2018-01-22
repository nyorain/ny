// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/key.hpp>

namespace ny {

//Does only map chars in the range 0-255 since most of the other ones are not really used.
constexpr struct Mapping {
	Keycode keycode;
	const char* name;
	bool nonSpecial {};
} mappings[] =
{
	{Keycode::escape, "escape"},

	{Keycode::k1, "1", true},
	{Keycode::k2, "2", true},
	{Keycode::k3, "3", true},
	{Keycode::k4, "4", true},
	{Keycode::k5, "5", true},
	{Keycode::k6, "6", true},
	{Keycode::k7, "7", true},
	{Keycode::k8, "8", true},
	{Keycode::k9, "9", true},
	{Keycode::k0, "0", true},
	{Keycode::minus, "minus", true},
	{Keycode::equals, "equals", true},
	{Keycode::backspace, "backspace"},
	{Keycode::tab, "tab", true},

	{Keycode::q, "q", true},
	{Keycode::w, "w", true},
	{Keycode::e, "e", true},
	{Keycode::r, "r", true},
	{Keycode::t, "t", true},
	{Keycode::y, "y", true},
	{Keycode::u, "u", true},
	{Keycode::i, "i", true},
	{Keycode::o, "o", true},
	{Keycode::p, "p", true},
	{Keycode::leftbrace, "leftbrace", true},
	{Keycode::rightbrace, "rightbrace", true},
	{Keycode::enter, "enter"},
	{Keycode::leftctrl, "leftctrl"},

	{Keycode::a, "a", true},
	{Keycode::s, "s", true},
	{Keycode::d, "d", true},
	{Keycode::f, "f", true},
	{Keycode::g, "g", true},
	{Keycode::h, "h", true},
	{Keycode::j, "j", true},
	{Keycode::k, "k", true},
	{Keycode::l, "l", true},
	{Keycode::semicolon, "semicolon", true},
	{Keycode::apostrophe, "apostrophe", true},
	{Keycode::grave, "grave", true},
	{Keycode::leftshift, "leftshift"},
	{Keycode::backslash, "backslash", true},

	{Keycode::x, "x", true},
	{Keycode::z, "z", true},
	{Keycode::c, "c", true},
	{Keycode::v, "v", true},
	{Keycode::b, "b", true},
	{Keycode::m, "m", true},
	{Keycode::n, "n", true},
	{Keycode::comma, "comma", true},
	{Keycode::period, "period", true},
	{Keycode::slash, "slash", true},
	{Keycode::rightshift, "rightshift"},
	{Keycode::kpmultiply, "kpmultiply", true},
	{Keycode::leftalt, "leftalt"},
	{Keycode::space, "space", true},
	{Keycode::capslock, "capslock"},

	{Keycode::f1, "f1"},
	{Keycode::f2, "f2"},
	{Keycode::f3, "f3"},
	{Keycode::f4, "f4"},
	{Keycode::f5, "f5"},
	{Keycode::f6, "f6"},
	{Keycode::f7, "f7"},
	{Keycode::f8, "f8"},
	{Keycode::f9, "f9"},
	{Keycode::f10, "f10"},
	{Keycode::numlock, "numlock"},
	{Keycode::scrollock, "scrolllock"},

	{Keycode::kp7, "kp7", true},
	{Keycode::kp8, "kp8", true},
	{Keycode::kp9, "kp9", true},
	{Keycode::kpminus, "kpminus", true},
	{Keycode::kp4, "kp4", true},
	{Keycode::kp5, "kp5", true},
	{Keycode::kp6, "kp6", true},
	{Keycode::kpplus, "kpplus", true},
	{Keycode::kp1, "kp1", true},
	{Keycode::kp2, "kp2", true},
	{Keycode::kp3, "kp3", true},
	{Keycode::kp0, "kp0", true},
	{Keycode::kpperiod, "kpperiod", true},

	{Keycode::zenkakuhankaku, "zenkakuhankaku"},
	{Keycode::nonushash, "nonushash"},
	{Keycode::f11, "f11"},
	{Keycode::f12, "f12"},

	{Keycode::katakana, "katakana"},
	{Keycode::hiragana, "hiragana"},
	{Keycode::henkan, "henkan"},
	{Keycode::katakanahiragana, "katakanahiragana"},
	{Keycode::muhenkan, "muhenkan"},
	{Keycode::kpjpcomma, "kpjpcomma"},
	{Keycode::kpenter, "kpenter"},
	{Keycode::rightctrl, "rightctrl"},
	{Keycode::kpdivide, "kpdivide"},
	{Keycode::sysrq, "sysrq"},
	{Keycode::rightalt, "rightalt"},
	{Keycode::linefeed, "linefeed"},
	{Keycode::home, "home"},
	{Keycode::up, "up"},
	{Keycode::pageup, "pageup"},
	{Keycode::left, "left"},
	{Keycode::right, "right"},
	{Keycode::end, "end"},
	{Keycode::down, "down"},
	{Keycode::pagedown, "pagedown"},
	{Keycode::insert, "insert"},
	{Keycode::del, "delete"},
	{Keycode::macro, "macro"},
	{Keycode::mute, "mute"},
	{Keycode::volumedown, "volumedown"},
	{Keycode::volumeup, "volumeup"},
	{Keycode::power, "power"},
	{Keycode::kpequals, "kpequals"},
	{Keycode::kpplusminus, "kpplusminus"},
	{Keycode::pause, "pause"},
	{Keycode::scale, "scale"},

	{Keycode::kpcomma, "kpcomma"},
	{Keycode::hangeul, "hangeul"},
	{Keycode::hanguel, "hanguel"},
	{Keycode::hanja, "hanja"},
	{Keycode::yen, "yen"},
	{Keycode::leftmeta, "leftmeta"},
	{Keycode::rightmeta, "rightmeta"},
	{Keycode::compose, "compose"},

	{Keycode::stop, "stop"},
	{Keycode::again, "again"},
	{Keycode::props, "props"},
	{Keycode::undo, "undo"},
	{Keycode::front, "front"},
	{Keycode::copy, "copy"},
	{Keycode::open, "open"},
	{Keycode::paste, "paste"},
	{Keycode::find, "find"},
	{Keycode::cut, "cut"},
	{Keycode::help, "help"},
	{Keycode::menu, "menu"},
	{Keycode::calc, "calc"},
	{Keycode::setup, "setup"},
	{Keycode::sleep, "sleep"},
	{Keycode::wakeup, "wakeup"},
	{Keycode::file, "file"},
	{Keycode::sendfile, "sendfile"},
	{Keycode::deletefile, "sendfildeletefile"},
	{Keycode::xfer, "xfer"},
	{Keycode::prog1, "prog1"},
	{Keycode::prog2, "prog2"},
	{Keycode::www, "www"},
	{Keycode::msdos, "msdos"},
	{Keycode::coffee, "coffee"},
	{Keycode::screenlock, "screenlock"},
	{Keycode::rotateDisplay, "rotateDisplay"},
	{Keycode::direction, "direction"},
	{Keycode::cyclewindows, "cyclewindows"},
	{Keycode::mail, "mail"},
	{Keycode::bookmarks, "bookmarks"},
	{Keycode::computer, "computer"},
	{Keycode::back, "back"},
	{Keycode::forward, "forward"},
	{Keycode::closecd, "closecd"},
	{Keycode::ejectcd, "ejectcd"},
	{Keycode::ejectclosecd, "ejectclosecd"},
	{Keycode::nextsong, "nextsong"},
	{Keycode::playpause, "playpause"},
	{Keycode::previoussong, "previoussong"},
	{Keycode::stopcd, "stopcd"},
	{Keycode::record, "record"},
	{Keycode::rewind, "rewind"},
	{Keycode::phone, "rewinphone"},
	{Keycode::iso, "iso"},
	{Keycode::config, "config"},
	{Keycode::homepage, "homepage"},
	{Keycode::refresh, "refresh"},
	{Keycode::exit, "exit"},
	{Keycode::move, "move"},
	{Keycode::edit, "edit"},
	{Keycode::scrollup, "scrollup"},
	{Keycode::scrolldown, "scrolldown"},
	{Keycode::kpleftparen, "kpleftparen"},
	{Keycode::kprightparen, "kprightparen"},
	{Keycode::knew, "knew"},
	{Keycode::redo, "redo"},

	{Keycode::f13, "f13"},
	{Keycode::f14, "f14"},
	{Keycode::f15, "f15"},
	{Keycode::f16, "f16"},
	{Keycode::f17, "f17"},
	{Keycode::f18, "f18"},
	{Keycode::f19, "f19"},
	{Keycode::f20, "f20"},
	{Keycode::f21, "f21"},
	{Keycode::f22, "f22"},
	{Keycode::f23, "f23"},
	{Keycode::f24, "f24"},

	{Keycode::playcd, "playcd"},
	{Keycode::pausecd, "pausecd"},
	{Keycode::prog3, "prog3"},
	{Keycode::prog4, "prog4"},
	{Keycode::dashboard, "dashboard"},
	{Keycode::suspend, "suspend"},
	{Keycode::close, "close"},
	{Keycode::play, "play"},
	{Keycode::fastforward, "fastforward"},
	{Keycode::bassboost, "bassboost"},
	{Keycode::print, "print"},
	{Keycode::hp, "hp"},
	{Keycode::camera, "camera"},
	{Keycode::sound, "sound"},
	{Keycode::question, "question"},
	{Keycode::email, "email"},
	{Keycode::chat, "chat"},
	{Keycode::search, "search"},
	{Keycode::connect, "connect"},
	{Keycode::finance, "finance"},
	{Keycode::sport, "sport"},
	{Keycode::shop, "shop"},
	{Keycode::alterase, "alterase"},
	{Keycode::cancel, "cancel"},
	{Keycode::brightnessdown, "brightnessdown"},
	{Keycode::brightnessup, "brightnessup"},
	{Keycode::media, "media"},

	{Keycode::switchvideomode, "switchvideomode"},
	{Keycode::kbdillumtoggle, "kbdillumtoggle"},
	{Keycode::kbdillumdown, "kbdillumdown"},
	{Keycode::kbdillumup, "kbdillumup"},

	{Keycode::send, "send"},
	{Keycode::reply, "reply"},
	{Keycode::forwardmail, "forwardmail"},
	{Keycode::save, "save"},
	{Keycode::documents, "documents"},
	{Keycode::battery, "battery"},
	{Keycode::bluetooth, "bluetooth"},
	{Keycode::wlan, "wlan"},
	{Keycode::uwb, "uwb"},
	{Keycode::unkown, "unkown"},
	{Keycode::videoNext, "videoNext"},
	{Keycode::videoPrev, "videoPrev"},
	{Keycode::brightnessCycle, "brightnessCycle"},
	{Keycode::brightnessAuto, "brightnessAuto"},
	{Keycode::brightnessZero, "brightnessZero"},
	{Keycode::displayOff, "displayOff"},
	{Keycode::wwan, "wwan"},
	{Keycode::wimax, "wimax"},
	{Keycode::rfkill, "rfkill"},
	{Keycode::micmute, "micmute"},
};

const char* name(Keycode keycode)
{
	for(auto m : mappings) {
		if(m.keycode == keycode) {
			return m.name;
		}
	}
	return "<unknown>";
}

Keycode keycodeFromName(const char* name)
{
	for(auto m : mappings) {
		if(name == m.name) {
			return m.keycode;
		}
	}
	return Keycode::none;
}

bool specialKey(Keycode keycode)
{
	for(auto m : mappings) {
		if(keycode == m.keycode) {
			return !m.nonSpecial;
		}
	}
	return false;
}

} // namespace ny
