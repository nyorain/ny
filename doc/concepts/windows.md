WindowSettings
==============

Additional capabitlity suggestions:
----------------------------------

 - change cursor
 - change icon
 - make droppable
 - beginMove
 - beginResize
 - title

WindowType
---------

Specifies the type of the window, i.e. the intention of its display.
This might change how the window is presented to the user by the backend.

enum class WindowType : unsigned int
{
	none =  0,
	toplevel,
	dialog,
//...
};


- DialogSettings
