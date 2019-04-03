- winapi last commit with multi-threaded clipboard, SetClipboardListener, dialog stuff:
	- 5add38fc66ca19100f9e0db2416ad8604aff2e77

- resource for winapi keyboard stuff:
https://handmade.network/wiki/2823-keyboard_inputs_-_scancodes,_raw_input,_text_input,_key_names

- commit that removed child functionality
	- inclusive (bad) implementations and wayland popup stuff
	- not all bad, just not really polished/finished/tested
	- 72f206ce9b690078ab6a1e08063aea589af78ba8

- commit that removed wayland::Output (since it wasn't used anywhere):
  e1b82d9

# Events

- KeyEvents are only guaranteed to contain the valid utf8 value when
  they are a key pressed event (may or may not contain it on key release)
- On window creation events are only sent if a property does not match
  the value specified in WindowSettings (i.e. is changed -> Event).
  	- They will also be sent for unknown default values, like
	  defaultPosition/defaultSize (i guess that is TODO, not always done)
	- This means if no size event is sent after window creation it has
	  exactly the size specified in WindowSettings
