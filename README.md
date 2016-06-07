# nedit-ng

nedit-ng is a Qt port of the Nirvana Editor (NEdit) version 5.6. It is intended
to be a **drop in replacement** for nedit in every practical way, just as on 
many systems `/usr/bin/vi` is now a symlink to `/usr/bin/vim`.

Because it is a true port of the original code, it (at least for now) inherits 
some (but not all) of the limitations of the original. On the other hand, some 
aspects have been improved simply by the fact that it is now a Qt application.

### Inherited Limitations:

* Text display is still ASCII only (for now).
* Regex engine is the original nedit proprietary implementation. This was kept 
  for a few reasons:
    1. NEdit's syntax is **slightly** non-standard, and I wanted to keep 
	   things backwards compatible for now.
	2. NEdit's syntax highligher has very carefully created regex's which 
	   result in things being highlighted in a way that I (and I assume other 
	   nedit users) have grown to appreciate. A change in regex engine would
	   likely require a rework on the syntax highlighting algorithm.
	3. The original highligher has some insider information of the regex
	   implementation which is uses in order to be more efficient. I could 
	   fake this information, but at the cost of efficiency.

### Improvements already available:

* Bug fixes! Yes, NEdit 5.6 unfortunately has some bugs, some of which can 
  result in a crash. Where possible, the new code has these fixed.
* Anti-aliased font rendering.
* Modern look and feel.
* Internally, counted strings are used instead of NUL terminated strings. This
  removes the need to have code that played games with substituting NUL 
  characters mid-stream.
* Use of some C++ containers means many internal size limits are no longer
  present.
* Code as been reworked using modern C++ techniques using a toolkit with an 
  active developer community, making it significantly easier for contributions
  to be made by the open source community.

### Some things which couldn't be faithfully ported :-(

There aren't many of them, but some things just can't be done in Qt to my 
knowlege.

* In NEdit, some menus have accelerators which indicate 2 shortcuts. For 
  example: "[Shift]Ctrl + F" meaning that the shift is optional and will
  alter the behavior of the shortcut. I am not aware of a way to do this
  using Qt, short of making a completely custom menu system.
* NEdit has several points of customization which are controlled by XResource
  files. I intend to create a new dialog for these "advanced" options.

Version 1.0's goal is to be a nearly 1:1 port of the original NEdit. Once that 
is complete some post 1.0 goals are already planned:

* Internationalization!
* Python scripting support in addition to the built-in macro system
* An improved preferences system
* Extensibility though plugins
* Sessions, meaning that you can save and restore an edit session (such as 
  several open tabs)
