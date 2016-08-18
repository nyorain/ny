ny::winapi::com
===============

This page documents the winapi::com implementation of the various ny data exchange interfaces.
There are the two ways of exchanging data with other programs on the system that are abstracted by
ny:
 - Clipboards. Most modern systems have at least one clipboard that can be used by all programs
   to store or retrieve data.
   The method of accessing the clipboard in ny is over the App/AppContext. The function
   implemented by the different backends is ny::AppContext::clipboard.
 - Drag-and-Drop. Many modern window systems allow the user to just select data in one program
   and then drag it into another one and drop it there.
   The DnD interfaces are described in data.hpp, mainly DataOffer and DataSource.

The Winapi backend uses COM to implement this functionality.
It tries to convert between ny::dataType and the default Clipboard formats
(https://msdn.microsoft.com/en-us/library/windows/desktop/ff729168(v=vs.85).aspx).

For custom types, it uses the custom registered format "ny::customDataFormat" and a global
memory buffer as medium. The first 4 bytes of this memory buffer hold the dataType id of the
custom data type. There are also some formats (like ny::dataTypes::time) that are transferred
using a raw buffer, but converted from/to the api representation (nytl::TimePoint).
