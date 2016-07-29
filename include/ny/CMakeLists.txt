install(DIRECTORY app DESTINATION include/ny)
install(DIRECTORY backend DESTINATION include/ny)
install(DIRECTORY window DESTINATION include/ny)
install(DIRECTORY base DESTINATION include/ny)
install(DIRECTORY gui DESTINATION include/ny)

install(FILES
	ny.hpp 
	include.hpp 
	backend.hpp 
	app.hpp 
	window.hpp
	gui.hpp
	fwd.hpp
 DESTINATION include/ny)
