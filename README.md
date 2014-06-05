physengine
==========

[![WIP video](http://i.imgur.com/msokMAR.png)](https://www.youtube.com/watch?v=qL0gOHs5-2k)

Overview
--------
This program aims to implement a physics simulation engine capable of doing simulations on various scales, from molecular dynamics to planetary motion, all whilst maintaining maximum performance. Implemented using the SDL2, Freetype 2, Lua 5.2 and the OpenGL ES 2.0 libraries. Uses POSIX threads to implement multi-threading. Written in standard C and released under the GPLv3 License.

Compiling
---------
Dependencies:

 * gcc (4.8.2 or newer)  or clang (3.5 or newer)
 * OpenGL ES 2.0 development libraries (Debian: libgles2-mesa-dev; Fedora: mesa-libGLES)
 * Freetype 2 development library (Debian: libfreetype6-dev; Arch, Fedora: freetype2) 
 * SDL 2.0 development library (Debian: libsdl2-dev; Arch, Fedora: sdl2)
 * Lua 5.2 (Debian: liblua5.2-dev)

All are available on any up-to-date Linux distribution's package repositories. To compile, do the standard:

`./autogen.sh`

`./configure`

`make`

Running
-------
Run the shell script. The executable needs to be ran into the root directory of the build. Modify the options in simconf.conf and the object position data (posdata.dat) to change the system being simulated. Modify program behaviour with the arguments listed further below. Pressing 'z' will create an XYZ file snapshot of the current system, which you can open with any molecule viewer (like VMD or GDIS). You can run `make rem` to remove all XYZ files in the current directory or `make clean` to remove them along with the program and compiled files.

This program **absolutely requires** the newest stable GCC or Clang releases to build. Vector extensions have always been a tricky things in C, but in the last few years the major compilers have roughly agreed on a standard. When using GCC, check that your processors has AVX extensions or performance will probably be affected.

Controls
--------
Button | Action
-------|-------
**LMB**      | Drag to rotate camera.
**MSCROLL**  | Control zoom.
**MMB**      | Drag to translate camera, deselects object.
**[ and ]**  | Increase/decrease time constant. Pause/unpause to put into effect.
**,(<) and .(>)**  | Select previous/next object.
**Enter**    | Press once to init object selection, input number and press again to select.
**r**        | Reset view and deseclect object.
**z**        | Dump entire system to an XYZ file.
**1**      | Draw objects as points/spheres.
**2**      | Enable/disable link drawing.
**Spacebar** | Pause/unpause.
**Esc**      | Quit the program. If running inside terminal with --novid, use Ctrl+C.

Usage:
------

`physengine -f (file) (arguments)`

Argument | Short | Action
---------|-------|-------
**--threads (int)** | **-t (int)** | Use this many threads to split the load. Overrides automatic thread detection.
**--dump** | none | Dump entire system to an XYZ file every second/timer update.
**--bench** | none | Benchmark mode, runs a single thread, no video, verbosity at 8, stops after 30 seconds(unless timer flag is set) and prints statistics. Useful for optimizations.
**--novid** | none | Disables SDL window creation and any OpenGL functions in the main loop.
**--nosync** | none | Disables vertical synchronization.
**--file (filename)** | **-f (filename)** | Specify which file to use for simulation.
**--algorithm (string)** | **-a (string)** | Specify which algorithm to use. Type `help` to list all.
**--log (filename)** | **-l (filename)** | Log all output at maximum verbosity to a file.
**--verb (int)** | **-v (int)** | Sets how much to output to stout. 0 - nothing, 10 - everything.
**--timer (float)** | **-r (float)** | Sets update rate for GUI/benchmark/xyz timer.
**--help** | **-h** | Print usage and flags.

simconf.lua
-----------
This file determines the system to simulate. It's a Lua script, so make sure to follow the syntax. All Lua libraries(including math) are included and initialized, feel free to use them. Error handling is done by Lua interpreter.

Physengine reads the settings table first, reads the global objects table and then calls the spawn_objects function, which returns its own local objects table along with a counter how many objects exist in it. Usually the global table contains any molecule files(in pdb or xyz formats) and any standalone objects. The function spawn_objects is for having your own distribution of objects.

Contacts
-------

 * **IRC Channel**: `#physengine` on `chat.freenode.net`
 * **Email**: `atomnuker@gmail.com`

Contributing
------------
Got a bug fix? New feature? Sent a pull request here on Github. Just please use the project's coding style (which is almost the same as the Linux kernel's coding style except without the 80 characters per line limit). You can use the `--bench` option to benchmark the performance and see how it's affected.
