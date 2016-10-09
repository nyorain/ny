Directions
==========

The most important ny concepts, guidelines and design rules.
All of those guidelines are changeable, but not instantly in the code, first it shall be
notices here and changes are only allowed after a constructive discussion.
Note that some of the ny guidelines may change slightly during releases.
Those points are not (yet) really ordered by importance.

- the implementation has to follow the interfaces. Of course must the interfaces be
  implementable, but they shall not be designed after the final implementations (which may
  change as new backend are implemented and old ones deprecated).
- good interfaces. Easy to use right, hard to use wrong. Everything in C++ is allowed, legacy
  things like macros or heavily relaying on the preprocessor are not desired. The interfaces
  shall be as easy as possible. C pods are as desirable as complex virtual template interface
  idioms. Always choose the best and most trivial solution on the whole.
- ny is built modular. The individual modules shall be as independent as possible and
  all of them shall be reasonable on their own. All modules shall be as generic as possible.
  Easy integration with other libraries/concepts/idioms/language techniques is always desirable.
  (note: this was a main direction between the different parts of ny were split up)
- ny is a highly reactive and changeable interfaces, if one wants a static stable library which
  does not change its core even if it would gain great profits they can still go with other, older
  gui libs.
- raii is usually the best way to go. There may be cases where it is not the best solution (rare),
  but there have to be serious reasons to not go with raii
  More general: write the code as safe as maintainable as possible. Things that could be easily
  forgotten/gotten wrong by other maintainers should not be used if possible (like e.g. having
  to know that certain resources have to be freed before earlier returning from a function).
- ny is exceptions safe, all interfaces are designed against exceptions and all implementations are
  written exception safe, unknowing which callbacks or handlers might throw 
  There is still loads of ny code that does not follow this direction
- ny is always as soon as possible updated to the new standards, tries to use stl where possible
  and tries to never reimplement or use reimplemented stl classes
- ny tries to go with as few external dependencies as possible. External dependencies are
  acceptable where some implementation-heavy features are needed but that should not be implemented
  by ny. A perfect example for a reasonably dependency is xkbcommon since all the manual keymap
  and state parsing would be too much to maintain (and probably worse than xkbcommon in the end).
- ny generally tries to go with existent standard where possible (where it makes sense)
  A counterexample is the coding style of ny where using the stl style has several drawbacks.
  Another important part is consitency with already established standars of ny
- everything shall be documented inside the code (mainly header files but impl docs are also
  desirable), for added features examples are always useful
- ny follows the concepts of open source and free software and shall not be the personal project
  by one (or few) people; it shall therefore always stay theoretically independent from larger
  projects (-> some desktop concept or DE shall not directly influence the library itself)
