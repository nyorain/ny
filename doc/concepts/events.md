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

# WindowImpl

```

///Abstract base class for handling WindowContext events.
///This is usually implemented by applications and associated with all window-relevant
///state such as drawing contexts or widget logic.
class WindowImpl
{
public:
	enum class Hit
	{
		none,
		resize,
		move,
		close,
		max,
		min
	};

	struct HitResult
	{
		Hit hit;
		WindowEdges resize;
	};

	enum class OfferResult
	{
		none,
		copy,
		move,
		ask
	};

	///Returns a default WindowImpl object.
	static constexpr WindowImpl& defaultInstance();

public:
	///Custom implement this function for client side decorations
	virtual HitResult click(nytl::Vec2i pos) { return Hit::none; }

	///Override this function to make the Window accept offered data objects (e.g. dragndrop).
	///Only if this function returns something other than none a DataOfferEvent can
	///be generated.
	virtual OfferResult offer(nytl::Vec2i pos, const DataTypes&) { return OfferResult::none; }

	virtual void draw() {}; ///Redraw the window
	virtual void close() {}; ///Close the window at destroy the WindowContext

	virtual void position(const PositionEvent&) {}; ///Window was repositioned
	vritual void resize(const SizeEvent&) {}; ///Window was resized
	virtual void state(const ShowEvent&) {}; ///The Windows state was changed

	virtual void key(const KeyEvent&) {}; ///Key event occurred while the window had focus
	virtual void focus(const FocusEvent&) {}; ///Window lost/gained focus

	virtual void mouseButton(const MouseButtonEvent&) {}; ///MouseButton was clicked on window
	virtual void mouseMove(const MouseMoveEvent&) {}; ///Mouse moves over window
	virtual void mouseWheel(const MouseWheelEvent&) {}; ///Mouse wheel rotated over window
	virtual void mouseCross(const MouseCrossEvent&) {}; ///Mouse entered/left window

	virtual void dataOffer(const DataOfferEvent&) {}; ///Window received a DataOffer

	///This callback is called for backend-specific events that might be interesting for
	///the applications.
	///Use this function to check for different events are only sent one backend if they
	///should be handled by the application.
	virtual void backendEvent(const Event&) {};
};

```


