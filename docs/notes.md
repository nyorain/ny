- winapi last commit with multi-threaded clipboard, SetClipboardListener, dialog stuff:
	- 5add38fc66ca19100f9e0db2416ad8604aff2e77

# Events

- KeyEvents are only guaranteed to contain the valid utf8 value when
  they are a key pressed event (may or may not contain it on key release)
- On windows creation events are only sent if a property does not match
  the value specified in WindowSettings (i.e. is changed -> Event).
  	- They will also be sent for unknown default values, like
	  defaultPosition/defaultSize
	- This means if no size event is sent after window creation it has
	  exactly the size specified in WindowSettings
