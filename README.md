physengine
==========

Overview
--------
This program aims to implement a physics simulation engine capable of doing simulations on various scales, from molecular dynamics to planetary motion, all whilst maintaining maximum performance. Implemented using the SDL2, Freetype 2, and the OpenGL ES 2.0 libraries. Uses POSIX threads to implement multi-threading. Written in standard C and released under the GPLv3 License.

Compiling
---------
Dependencies:

 * gcc (4.8 or newer)  or clang (3.3 or newer)
 * OpenGL ES 2.0 development libraries (Debian: libgles2-mesa-dev; Fedora: mesa-libGLES)
 * Freetype 2 development library (Debian: libfreetype6-dev; Arch, Fedora: freetype2) 
 * SDL 2.0 development library (Debian: libsdl2-dev; Arch, Fedora: sdl2)

All are available on any up-to-date Linux distribution's package repositories. To compile, do the standard:

`./autogen.sh`

`./configure`

`make`

Running
-------
Run the executable. Modify the options in simconf.conf and the object position data (posdata.dat) to change the system being simulated. Modify program behaviour with the arguments listed further below. Pressing 'z' will create an XYZ file snapshot of the current system, which you can open with any molecule viewer (like VMD or GDIS). You can run `make rem` to remove all XYZ files in the current directory or `make clean` to remove them along with the program and compiled files.

This program **requires** the newest stable GCC or Clang releases. Vector extensions have always been a tricky things in C, but in the last few years the major compilers have roughly agreed on a standard. Currently the requirements are either GCC 4.8(or newer) or Clang 3.3(or newer). When using GCC, check that your processors has AVX extensions or performance will probably be affected.

Controls
--------
Button | Action
-------|-------
**LMB**      | Drag to rotate camera.
**MSCROLL**  | Control zoom.
**MMB**      | Drag to translate camera, deselects object.
**[ and ]**  | Increase/decrease time constant. Pause/unpause to put into effect.
**1 and 2**  | Select previous/next object.
**r**        | Reset view and deseclect object.
**z**        | Dump entire system to an XYZ file.
**TAB**      | Cycle - draw links/objects/both.
**Spacebar** | Pause/unpause.
**Esc**      | Quit the program. If running inside terminal with --novid, use Ctrl+C.

Command line arguments
----------------------
Argument | Action
---------|-------
**-f (filename)** | Specifies a posdata.dat to use. Falls back on simconf.conf's setting.
**--threads (int)** | Use (int) threads to speed up force calculation. Splits objects between cores.
**--dump** | Dump entire system to an XYZ file every second.
**--novid** | Disables SDL window creation and any OpenGL functions in the main loop.
**--fullogl** | Initialize full OpenGL instead of ES. Look in simconf.conf to change versions.
**--nosync** | Disables vertical synchronization.
**--log** | Log all output at current verbose level to physengine.log.
**--verb (uint)** | Sets how much to output to stout. 0 - nothing, 10 - everything.
**--help** | Print possible arguments and exit.

simconf.conf
------------
This file contains the program configuration. Most of the settings have self-explanatory names, with any remarks written as comments. Quotation marks (`variable = "value"`) around the value are **absolutely neccessary**. Section tags (`[section]`) are not required.

posdata.dat
-----------
This file determines the properties of all the objects to be simulated. The comment at the top describes the correct order and structure. Object numbers **must** be sequential, starting from 1. The argument `-f (filename)` overrides the filename in simconf.conf, however, the program will fall back on the configuration file should no such filename exists. If both filenames do not exist, the program will exit with a return value of 1. Currently, no error checking is present for this file, therefore should the physics engine misbehave, inspect both the objects table printed on the terminal and your posdata file. In case nothing appears to be wrong, file a bug report with the file.

Contacts
-------

 * **IRC Channel**: `#physengine` on `chat.freenode.net`
 * **Email**: `atomnuker@gmail.com`

Contributing
------------
Got a bug fix? New feature? Sent a pull request here on Github. Just please use the project's coding style (which is almost the same as the Linux kernel's coding style except without the 80 characters per line limit).
