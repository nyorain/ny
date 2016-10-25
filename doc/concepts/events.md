# EventTypeRegister

The idea of a static event type register that totally prevents multiple events having the same
number. The problem would be that multiple processes could have different numbers for
different event types. Solution: make them serializable using names.
The numbers/names could be generated/claimed statically (since EventTypeRegister would be
a singleton) as well as at runtime dynamically.

```

class EventTypeRegister
{
public:
	EventTypeRegister& instance();

public:
	std::string name(unsigned int type);
	unsigned int id(const std::string& name);

	bool registered(unsigned int type);
	unsigned int register(const std::string& name, unsigned int pref = 0);
	unsigned int registerPref(const std::string& name, unsigned int pref);
};

static auto eventTypeClose = EventTypeRegister::instance().register("ny::eventType::close");

```
