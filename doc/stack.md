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
					since std::any can be created from an object of any type.
				- additionally the gcc6/7 std::any implementation does not work with
					MinGW due to linkin issues (templates).

# Work Stacks

The idea of work stacks is to show the reason behind changes represented in their natural
way of happening. They are thought as addition to commits (and their comments) since
they easily show WHY things were changed (or at least what caused them to be changed).
