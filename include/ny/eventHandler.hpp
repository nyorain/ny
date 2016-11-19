// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>

namespace ny
{

///Interface for classes that are able to handle ny::Event objects.
///Can be used as synchronization and polymorphic dispatch method.
class EventHandler
{
public:
    EventHandler() = default;
    virtual ~EventHandler() = default;

	///Handle the given event.
	///\return true when the event was processed, false otherwise
    virtual bool handleEvent(const Event&) { return false; }
};

}
