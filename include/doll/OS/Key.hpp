#pragma once

namespace doll {

	// Keyboard key scancode
	enum class EKey
	{
		None						= 0,

		Escape						= 1,				//!< escape key (US KBD)
		One							= 2,				//!< '1'/'!' key (US KBD)
		Two							= 3,				//!< '2'/'@' key (US KBD)
		Three						= 4,				//!< '3'/'#' key (US KBD)
		Four						= 5,				//!< '4'/'$' key (US KBD)
		Five						= 6,				//!< '5'/'%' key (US KBD)
		Six							= 7,				//!< '6'/'^' key (US KBD)
		Seven						= 8,				//!< '7'/'&' key (US KBD)
		Eight						= 9,				//!< '8'/'*' key (US KBD)
		Nine						= 10,				//!< '9'/'(' key (US KBD)
		Zero						= 11,				//!< '0'/')' key (US KBD)
		Minus						= 12,				//!< '-'/'_' key (US KBD)
		Equals						= 13,				//!< '='/'+' key (US KBD)
		Back						= 14,				//!< backspace key (US KBD)
		Tab							= 15,				//!< tab key (US KBD)
		Q							= 16,				//!< 'q'/'Q' key (US KBD)
		W							= 17,				//!< 'w'/'W' key (US KBD)
		E							= 18,				//!< 'e'/'E' key (US KBD)
		R							= 19,				//!< 'r'/'R' key (US KBD)
		T							= 20,				//!< 't'/'T' key (US KBD)
		Y							= 21,				//!< 'y'/'Y' key (US KBD)
		U							= 22,				//!< 'u'/'U' key (US KBD)
		I							= 23,				//!< 'i'/'I' key (US KBD)
		O							= 24,				//!< 'o'/'O' key (US KBD)
		P							= 25,				//!< 'p'/'P' key (US KBD)
		LBracket					= 26,				//!< '['/'{' key (US KBD)
		RBracket					= 27,				//!< ']'/'}' key (US KBD)
		Return						= 28,				//!< enter/return key (US KBD)
		LControl					= 29,				//!< left control key (US KBD)
		A							= 30,				//!< 'a'/'A' key (US KBD)
		S							= 31,				//!< 's'/'S' key (US KBD)
		D							= 32,				//!< 'd'/'D' key (US KBD)
		F							= 33,				//!< 'f'/'F' key (US KBD)
		G							= 34,				//!< 'g'/'G' key (US KBD)
		H							= 35,				//!< 'h'/'H' key (US KBD)
		J							= 36,				//!< 'j'/'J' key (US KBD)
		K							= 37,				//!< 'k'/'K' key (US KBD)
		L							= 38,				//!< 'l'/'L' key (US KBD)
		Semicolon					= 39,				//!< ';'/':' key (US KBD)
		Apostrophe					= 40,				//!< '''/'"' key (US KBD)
		Grave						= 41,				//!<
		LShift						= 42,				//!< left shift key (US KBD)
		Backslash					= 43,				//!< '\'/'|' key (US KBD)
		Z							= 44,				//!< 'z'/'Z' key (US KBD)
		X							= 45,				//!< 'x'/'X' key (US KBD)
		C							= 46,				//!< 'c'/'C' key (US KBD)
		V							= 47,				//!< 'v'/'V' key (US KBD)
		B							= 48,				//!< 'b'/'B' key (US KBD)
		N							= 49,				//!< 'n'/'N' key (US KBD)
		M							= 50,				//!< 'm'/'M' key (US KBD)
		Comma						= 51,				//!< ','/'<' key (US KBD)
		Period						= 52,				//!< '.'/'>' key (US KBD)
		Slash						= 53,				//!< '/'/'?' key (US KBD)
		RShift						= 54,				//!< right shift key (US KBD)
		Multiply					= 55,				//!< '*' (numpad) key (US KBD)
		LMenu						= 56,				//!< left alt key (US KBD)
		Space						= 57,				//!< space bar key (US KBD)
		Capital						= 58,				//!< caps-lock key (US KBD)
		F1							= 59,				//!< F1 key (US KBD)
		F2							= 60,				//!< F2 key (US KBD)
		F3							= 61,				//!< F3 key (US KBD)
		F4							= 62,				//!< F4 key (US KBD)
		F5							= 63,				//!< F5 key (US KBD)
		F6							= 64,				//!< F6 key (US KBD)
		F7							= 65,				//!< F7 key (US KBD)
		F8							= 66,				//!< F8 key (US KBD)
		F9							= 67,				//!< F9 key (US KBD)
		F10							= 68,				//!< F10 key (US KBD)
		NumLock						= 69,				//!< num-lock key (US KBD)
		Scroll						= 70,				//!< scroll-lock key (US KBD)
		NumPad7						= 71,				//!< '7' (numpad) key (US KBD)
		NumPad8						= 72,				//!< '8' (numpad) key (US KBD)
		NumPad9						= 73,				//!< '9' (numpad) key (US KBD)
		Subtract					= 74,				//!< '-' (numpad) key (US KBD)
		NumPad4						= 75,				//!< '4' (numpad) key (US KBD)
		NumPad5						= 76,				//!< '5' (numpad) key (US KBD)
		NumPad6						= 77,				//!< '6' (numpad) key (US KBD)
		Add							= 78,				//!< '+' (numpad) key (US KBD)
		NumPad1						= 79,				//!< '1' (numpad) key (US KBD)
		NumPad2						= 80,				//!< '2' (numpad) key (US KBD)
		NumPad3						= 81,				//!< '3' (numpad) key (US KBD)
		NumPad0						= 82,				//!< '0' (numpad) key (US KBD)
		Decimal						= 83,				//!< '.' (numpad) key (US KBD)
		Oem102						= 86,				//!<
		F11							= 87,				//!< F11 key (US KBD)
		F12							= 88,				//!< F12 key (US KBD)
		F13							= 100,				//!< F13 key (US KBD)
		F14							= 101,				//!< F14 key (US KBD)
		F15							= 102,				//!< F15 key (US KBD)
		Kana						= 112,				//!<
		AbntC1						= 115,				//!<
		Convert						= 121,				//!<
		NoConvert					= 123,				//!<
		Yen							= 125,				//!<
		AbntC2						= 126,				//!<
		NumPadEquals				= 141,				//!< '=' (numpad) key (US KBD)
		PrevTrack					= 144,				//!<
		At							= 145,				//!< '@' (alternate) key (US KBD)
		Colon						= 146,				//!< ':' (alternate) key (US KBD)
		Underline					= 147,				//!< '_' (alternate) key (US KBD)
		Kanji						= 148,				//!<
		Stop						= 149,				//!<
		ax							= 150,				//!<
		Unlabeled					= 151,				//!<
		NextTrack					= 153,				//!<
		NumPadEnter					= 156,				//!< enter (numpad) key (US KBD)
		RControl					= 157,				//!< right control key (US KBD)
		Mute						= 160,				//!<
		Calculator					= 161,				//!<
		PlayPause					= 162,				//!<
		MediaStop					= 164,				//!<
		VolumeDown					= 174,				//!<
		VolumeUp					= 176,				//!<
		WebHome						= 178,				//!<
		NumPadComma					= 179,				//!< ',' (numpad) key (US KBD)
		Divide						= 181,				//!< '/' (numpad) key (US KBD)
		SysRq						= 183,				//!<
		RMenu						= 184,				//!< right alt key (US KBD)
		Pause						= 197,				//!< pause key (US KBD)
		Home						= 199,				//!< home key (US KBD)
		Up							= 200,				//!< up arrow key (US KBD)
		Prior						= 201,				//!< page up key (US KBD)
		Left						= 203,				//!< left arrow key (US KBD)
		Right						= 205,				//!< right arrow key (US KBD)
		End							= 207,				//!< end key (US KBD)
		Down						= 208,				//!< down key (US KBD)
		Next						= 209,				//!< page down key (US KBD)
		Insert						= 210,				//!< insert key (US KBD)
		Delete						= 211,				//!< delete key (US KBD)
		LWin						= 219,				//!< left Windows key (US KBD)
		RWin						= 220,				//!< right Windows key (US KBD)
		Apps						= 221,				//!<
		Power						= 222,				//!<
		Sleep						= 223,				//!<
		Wake						= 227,				//!<
		WebSearch					= 229,				//!<
		WebFavorites				= 230,				//!<
		WebRefresh					= 231,				//!<
		WebStop						= 232,				//!<
		WebForward					= 233,				//!<
		WebBack						= 234,				//!<
		MyComputer					= 235,				//!<
		Mail						= 236,				//!<
		MediaSelect					= 237,				//!<

		Esc							= Escape,
		Backspace					= Back,
		NumPadStar					= Multiply,
		LAlt						= LMenu,
		CapsLock					= Capital,
		NumPadMinus					= Subtract,
		NumPadPlus					= Add,
		NumPadPeriod				= Decimal,
		NumPadSlash					= Divide,
		RAlt						= RMenu,
		UpArrow						= Up,
		PgUp						= Prior,
		LeftArrow					= Left,
		RightArrow					= Right,
		DownArrow					= Down,
		PgDn						= Next,
		Circumflex					= PrevTrack,
		LCtrl						= LControl,
		RCtrl						= RControl,
		LSuper						= LWin,
		RSuper						= RWin,
		LCommand					= LWin,
		RCommand					= RWin,
		LCmd						= LWin,
		RCmd						= RWin,
		LApple						= LWin,
		RApple						= RWin,
		Enter						= Return
	};

	// Mouse button code
	enum class EMouse
	{
		None						= 0,

		Button1						= 1,
		Button2						= 2,
		Button3						= 3,
		Button4						= 4,
		Button5						= 5,
		Button6                     = 6,
		Button7                     = 7,
		Button8                     = 8,

		Left						= Button1,
		Right						= Button2,
		Middle						= Button3,
		Thumb1						= Button4,
		Thumb2						= Button5,

		Primary						= Button1,
		Secondary					= Button2
	};

}
