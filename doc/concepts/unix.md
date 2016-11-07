Common Unix abstraction
=======================

The idea is to have a common abstraction above all unix backends to make integrating it
with already existing applications/libaries easier.
All unix backends have in common that they somehow operator on file descriptors (since
this is the usual way for communicating with other application like e.g. a window manager)
and therefore the main aspect of the unix abstraction deals with file descriptors which
can be really powerful for applications.

```cpp
///Gives unix applications the possibility to integrate ny with existing event loops or
///other file descriptors that have to be monitored.
///Note that this abstraction does only apply to unix since there is no equivalent of the same
///value and functionality as file descriptors for non-unix (i.e. windows) backends.
///The AppContext implementations derived by this abstract base class do usually already
///make good use of the fdCallback functionality e.g. for timers, filesystem monitoring or
///simply synchronization.
class UnixAppContext : public AppContext
{
public:
	using FdCallback = nytl::CompFunc<void(nytl::ConnectionRef, int fd, unsigned int events)>;

public:
	///The event dispatching functions will now monitor the given file descriptor for the given
	///events. If one of the events is met, calls the given callback function.
	///If monitoring the fd is no longer needed or the fd is about to get closed, unregister
	///it by disconnecting the return Connection.
	virtual nytl::Connection fdCallback(int fd, unsigned int events, cosnt FdCallback& func) = 0;

	///Returns a number of file descriptors and their associated events that have to be monitored
	///if the applications only wants to call the event dispatch functions if it is needed.
	///So if for any of the returned file descriptors (pair.first) the associated event is met
	///(pair.second) the backend has potentially work to do.
	///Note that this call also returns all registered fds by fdCallback.
	virtual std::vector<std::pair<int, unsigned int>> fd() const = 0;
};
```
