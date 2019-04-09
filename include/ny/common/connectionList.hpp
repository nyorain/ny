// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <nytl/connection.hpp>
#include <vector>
#include <algorithm>

namespace ny {

/// Utility template that allows a generic list of connectable objects.
template<typename T>
class ConnectionList : public nytl::Connectable {
public:
	struct Value : public T {
		Value(const T& val, nytl::ConnectionID id) : T(val), clID_(id) {}
		nytl::ConnectionID clID_;
	};

	std::vector<Value> items;
	nytl::ConnectionID highestID;

public:
	bool disconnect(const nytl::ConnectionID& id) override {
		// NOTE: binary search would be faster but not possible since highestID might wrap
		for(auto it = items.begin(); it != items.end(); ++it) {
			if(it->clID_.get() == id.get()) {
				items.erase(it);
				return true;
			}
		}

		return false;
	}

	nytl::Connection add(const T& value) {
		items.emplace_back(value, nextID());
		return {*this, items.back().clID_};
	}

	nytl::ConnectionID nextID() {
		++highestID.value;
		return highestID;
	}
};

} // namespace ny
