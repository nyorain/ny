#pragma once

#include <functional>
#include <utility>
#include <algorithm>
#include <dlg/dlg.hpp>

namespace ny {

/// Simple function container.
/// Can be used to store functions that should be exectued
/// later on.
/// \tparam Signature The signature of registered functions
/// \tparam ID An id that may be associated with registered handlers
///   to remove them later on. This type must be default-constructible
///   and copy constructible/assignable. If you don't need
///   to remove functions later on, just leave the default parameter.
template<typename Signature, typename ID = unsigned int>
class DeferredOperator;

/// Specialization to allow signature template syntax.
template<typename ID, typename Ret, typename... Args>
class DeferredOperator<Ret(Args...), ID> {
public:
	using Func = std::function<Ret(Args...)>;

public:
	/// Adds the given function to the list of entries
	void add(Func func) {
		dlg_assert(func);
		entries_.push_back({ID{}, std::move(func)});
	}

	void add(Func func, ID id) {
		dlg_assert(func);
		entries_.push_back({std::move(id), std::move(func)});
	}

	/// Executes all registered entries.
	/// The called functions may add/remove entries (via
	/// the add/remove functions).
	void execute(Args... args) {
		// auto s = entries_.size();
		// auto i = 0u;
		// for(i = 0u; i < std::min(s, entries_.size()); ++i) {
		// 	auto func = std::move(entries_[i].second);
		// 	entries_[i].first = {}; // so it will not be removed
		// 	func(std::forward<Args>(args)...);
		// }
		// entries_.erase(entries_.begin(), entries_.begin() + i);

		// TODO...
		auto entries = std::move(entries_);
		for(auto& e : entries) {
			e.second(std::forward<Args>(args)...);
		}
	}

	/// Removes all registered functions with the given id.
	/// Note that the id must not be empty (i.e. default constructed),
	/// but a valid value (at least while executing)
	void remove(const ID& id) {
		auto end = std::remove_if(entries_.begin(), entries_.end(),
			[&](const auto& e){ return e.first == id && e.second; });
		entries_.erase(end, entries_.end());
	}

	/// Can be used to iterate manually e.g. to handle the
	/// return values.
	const auto& entries() const { return entries_; }

protected:
	std::vector<std::pair<ID, Func>> entries_;
};

} // namespace ny
