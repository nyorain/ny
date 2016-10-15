nyorain work stack 15.10.2016
=============================

- wayland data impl
	- WaylandDataOffer
		- wayland data pipe call for receive
			- cloexec
		- formatToMimeType
			- mime types
		- wayland AppContext general fd wait/callback interface
			- callback registering
				- implement ConnectionList
				- nytl callback size type
					- fix cmake nytl generation
				- implement register functions
			- displayDispatch implementation
			- testing/fixing with ny-dev
		- wayland dataOffer interface implementatoin using memberCallback<>
		- testing/fixing
			- nytl CompFunc has a bug somewhere
				- after long searching, found its actually a feature (regarding any)
