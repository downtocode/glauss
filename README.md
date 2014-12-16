physengine
==========

Overview
--------
This program aims to implement a physics simulation engine capable of doing simulations on various scales, from molecular dynamics to planetary motion to Monte Carlo, all whilst maintaining maximum performance. A user-friendly Lua API is provided to experiment and quickly test and deploy algorithms. Implemented minimalistically using the SDL2, Freetype 2, Lua 5.2 and the OpenGL ES 2.0 libraries. Uses POSIX threads to implement multi-threading. Written in standard C and released under the GPLv3 License.

Compiling
---------
Complete list of dependencies:

 * gcc (4.9 or newer)  or clang (3.5 or newer)
 * OpenGL ES 2.0 development libraries (Debian: libgles2-mesa-dev, Arch: libgles(mesa))
 * Freetype 2 development library (Debian: libfreetype6-dev; Arch, Fedora: freetype2)
 * Fontconfig development library(Debian: libfontconfig1-dev, Arch: fontconfig)
 * SDL 2.0 development library (Debian: libsdl2-dev; Arch, Fedora: sdl2)
 * Lua 5.2 (Debian: liblua5.2-dev, Arch: lua)

All are available on any up-to-date Linux distribution's package repositories. To compile and install:

`./bootstrap.py`

`./waf configure`

`./waf build`

`./waf install`

In case program has been installed, use the `physengine` command to run, with appropriate arguments. To uninstall, use `./waf uninstall` in the original project folder.

The program can also be run in any directory as it has no dependence on any external files. For developer convenience, it's possible to avoid program installation. Just build and run the shell script without installing.

This program **absolutely requires** the newest stable GCC or Clang releases to build. Vector extensions have always been a tricky things in C, but in the last few years the major compilers have roughly agreed on a standard. When using GCC, check that your processors has AVX extensions or performance will probably be degraded.

Controls and Usage
------------------
Run the program with the --help flag and/or consult the [manual](DOCS/physengine.rst#controls).

Running
-------
Run the program with the argument `-f (filename)`, with an appropriate file. A demo file(usually it's for testing the latest feature) called simconf.lua is present. A demo scripts directory will be created at a later version. The executable needs to be ran into the root directory of the build. Modify the options in simconf.conf to change the system being simulated. Modify program behaviour with the arguments listed further below. Pressing 'z' will create an XYZ file snapshot of the current system, which you can open with any molecule viewer (like VMD or GDIS).

Command line
------------
A command line is included if libreadline is installed. It allows to change almost every aspect of the simulation. Type "help" to list all possible commands and variables(with values printed out). Typing a variable alone will print its current value. Typing `<variable> <value>` changes its current value. Note that for some options like `threads` a restart is needed in order for them to take effect. Using Lua to change a variable is possible as well using the `set_option("<variable>", value)` function inside Lua.

Input file
----------
The filepath/name specified through the `-f` flag determines the system to simulate. It's a Lua script, so make sure to follow the syntax. All Lua libraries(including math) are included and initialized, feel free to use them. Error handling is done by Lua interpreter. You'll probably want this to be the first argument, as any option put before will get overwritten if it exists in the file.

Physengine reads the settings table first, then calls the spawn_objects function, which returns the objects table. That table is then read, counted and scanned for any files that need importing. Once the final count is known, memory is allocated and the table is read again, importing all objects.

A function to be called on every timestep can be defined, with its frequency adjusted. The statistics and optionally, all objects will be sent as arguments. The objects send to this function have the exact same structure as the ones send from Lua to C during spawning except for the addition of their internal `id` variable. A non-nil table of objects returned by this function will be read back and will overwrite the internal objects, by using the `id` variable of each object's table. Use it to program your own algorithm entirely in Lua.

For a list of all variables look in the appropriate section of the [manual](DOCS/physengine.rst#configuration-files).

Bug reports
-----------
Use either the Github Issues section of this project or the contact section below.

Contacts
--------
Want to chat? Life counselling? Psychology? Classical/wooden philosophy? Need a partner to rob a bank? Metaphysics? Pataphysics?

 * **IRC Channel**: `#physengine` on `chat.freenode.net`
 * **Email**: `atomnuker@gmail.com`

Contributing
------------
Got a bug fix? New feature? Sent a pull request here on Github. Demo Lua project scripts are accepted. Just please use the project's coding style (pretty much the Linux kernel coding style, tabs are 4 spaces). You can use the `--bench` option to benchmark the performance and see how it's affected.
