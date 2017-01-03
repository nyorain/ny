# Code Style

Some general code style annotations/guidelines for ny.

## General code style

Generally, ny adopts the code style used by the most bigger modern libraries.

Class names, enums (generally all types) are always in CamelCase (first character capitalized).
Variable and function declarations are camelCase (first character non-capitalized).
Macros are defined like this: NY_SOME_MACRO, they always have the NY (and maybe additional)
prefix, are all capitalized and seperated by underscores.
Protected/private member variables end with an underscore like this `int count_ {};`.

Whitespace and empty lines are used sparingly:

```cpp
class Example {
public:
	// first static public stuff and typedefs

public:
	// then public variables

public:
	// then public methods, constructors and more significant functions first

	// then the same as above (seperated the same way) for protected and private
	// usually classes use protected members over private ones
};

if(condition1) {
	doSomething();
} else if(condition2) {
	doSomethingElse();
}

/// Document stuff like this
/// Prefer to use standardese tags like
/// \param, \module, \throws, \requires or \brief
for(auto c : string) {
	foo(c);
}

switch(var) {
	case 1: {
		foo();
		break;
	}
	case 2: {
		bar();
		break;
	}
	default:
		break;
}
```

## Documentation

All interfaces are documented if needed (e.g. they don't have to be documented if they
are obvious to someone reading the header for the first time).
Source code is documented, so that other ny developers (or readers trying to understand the
implementation) can understand how it is done, i.e. everything that is not obvious for
someone with basic knowledge about ny and the backend should be documented.
The general rule of thumb is do document things in source code if youself could maybe
not remember what the code is for/what it does some time (like a few years) after writing
it or if it was not trivial to figure out how to do it.

## Namespaces vs prefixes

All definitions done by ny should be in the ny namespace. Utility functions only
used in source files should be in an anonymous namespace.

Backend-specific classes deriving from common interfaces should have a prefix to make
their name unique, e.g. `class WaylandWindowContext : public WindowContext` so when the
name `WindowContext` used it will always refer to the interface (which would not be
the case if defining wayland::WindowContext instead). Other than that, utility functions
or definitions that do not implement public interfaces, i.e. will not take the
same name in a different namespace should be shifted to a seperate namespace.

Examples are `ny::winapi::lastErrorCode()` or `ny::wayland::ShmBuffer` since both entities
are not part of core ny but only of one specific backend. Those names are not used
in core ny (but could be one day).

General rule: When something is a central part of an entity (like Wayland for WaylandWindowContext)
and if one cannot image leaving the part of the name out in a certain context, it should be in
the name (as prefix).
On the other hand, if something just belongs into a group of entities (and might also only make
sense as part of this group) and in a certain context could be imagined without the prefix,
use a namespace.

Example:
Like presented above wayland::WindowContext would collide with the WindowContext interface
declaration and therefore should never be used without namespace which makes a namespace
a rather bad design choice, and therefore a prefix is preferred.
For winapi::lastErrorCode on the other hand, function names like winapiLastErrorCode or
lastWinapiErrorCode would be rather bad, since in winapi backend-specific functions
usage of the function lastErrorCode is pretty self-explanatory and therefore the entity
is better defined insdie the namespace.
