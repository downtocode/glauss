physengine
==========

[![WIP video](http://i.imgur.com/msokMAR.png)](https://www.youtube.com/watch?v=qL0gOHs5-2k)

Overview
--------
This program aims to implement a physics simulation engine capable of doing simulations on various scales, from molecular dynamics to planetary motion, all whilst maintaining maximum performance. Implemented using the SDL2, Freetype 2, Lua 5.2 and the OpenGL ES 2.0 libraries. Uses POSIX threads to implement multi-threading. Written in standard C and released under the GPLv3 License.

Compiling
---------
Complete list of dependencies:

 * gcc (4.9 or newer)  or clang (3.5 or newer)
 * OpenGL ES 2.0 development libraries (Debian: libgles2-mesa-dev; Fedora: mesa-libGLES)
 * Freetype 2 development library (Debian: libfreetype6-dev; Arch, Fedora: freetype2)
 * Fontconfig development library(Debian: libfontconfig1-dev)
 * PNG 1.2 development library (Debian: libpng12-dev)
 * SDL 2.0 development library (Debian: libsdl2-dev; Arch, Fedora: sdl2)
 * Lua 5.2 (Debian: liblua5.2-dev)

All are available on any up-to-date Linux distribution's package repositories. To compile and install:

`./bootstrap.py`

`./waf configure`

`./waf build`

`./waf install`

The program can be run in any directory as it has no dependence on any external files. For developer convenience, it's possible to avoid program installation. Just build and run the shell script without installing.

This program **absolutely requires** the newest stable GCC or Clang releases to build. Vector extensions have always been a tricky things in C, but in the last few years the major compilers have roughly agreed on a standard. When using GCC, check that your processors has AVX extensions or performance will probably be affected.

Controls and Usage
------------------
Run the program with the --help flag and/or consult the [manual](DOCS/physengine.rst#controls).

Running
-------
Run the program with the argument `-f (filename)`, with an appropriate file. A demo file(usually it's for testing the latest feature) called simconf.lua is present. A demo scripts directory will be created at a later version. The executable needs to be ran into the root directory of the build. Modify the options in simconf.conf to change the system being simulated. Modify program behaviour with the arguments listed further below. Pressing 'z' will create an XYZ file snapshot of the current system, which you can open with any molecule viewer (like VMD or GDIS).

Input file
----------
The filepath/name specified through the `-f` flag determines the system to simulate. It's a Lua script, so make sure to follow the syntax. All Lua libraries(including math) are included and initialized, feel free to use them. Error handling is done by Lua interpreter.

Physengine reads the settings table first, reads the global objects table and then calls the spawn_objects function, which returns its own local objects table along with a counter how many objects exist in it. Usually the global table contains any molecule files(in pdb or xyz formats) and any standalone objects. The function spawn_objects is for having your own distribution of objects.

For a list of all variables look in the appropriate section of the [manual](DOCS/physengine.rst#configuration-files).

Contacts
--------
Want to chat? Life counselling? Psychology? Classical/wooden philosophy? Metaphysics? Pataphysics? Perfect. Though try to stay on topic. In between sessions.

 * **IRC Channel**: `#physengine` on `chat.freenode.net`
 * **Email**: `atomnuker@gmail.com`

Contributing
------------
Got a bug fix? New feature? Sent a pull request here on Github. Just please use the project's coding style (which is almost the same as the Linux kernel's coding style except without the 80 characters per line limit). You can use the `--bench` option to benchmark the performance and see how it's affected.
