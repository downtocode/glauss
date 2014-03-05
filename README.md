physengine
==========

[![Build Status](https://travis-ci.org/atomnuker/physengine.png?branch=master)](https://travis-ci.org/atomnuker/physengine)

Overview
--------
This program aims to implement a physics simulation engine capable of doing simulations on various scales, from molecular dynamics to planetary motion, all whilst maintaining maximum performance. Implemented using the SDL2, Freetype 2, and the OpenGL ES 2.0 libraries. Uses POSIX threads to implement multi-threading. Written in standard C and released under the MIT License.

Compiling
---------
Dependencies:

 * gcc (4.6 or newer)  or clang (3.3 or newer)
 * OpenGL ES 2.0 development libraries (Debian: libgles2-mesa-dev; Fedora: mesa-libGLES)
 * Freetype 2 development library (Debian: libfreetype6-dev; Arch, Fedora: freetype2) 
 * SDL 2.0 development library (Debian: libsdl2-dev; Arch, Fedora: sdl2)

All are available on any up-to-date Linux distribution's package repositories. Once all the dependencies are satisfied, run make in this directory to compile. An executable named physengine will be created.

Running
-------
Run the executable. Modify the options in simconf.conf and the object position data (posdata.dat) to change the system being simulated. Modify program behaviour with the arguments listed further below. Pressing 'z' will create an XYZ file snapshot of the current system, which you can open with any molecule viewer (like VMD or GDIS). You can run `make rem` to remove all XYZ files in the current directory or `make clean` to remove them along with the program and compiled files.

Controls
--------
Button | Action
-------|-------
**LMB**      | Drag to rotate camera.
**[ and ]**  | Increase/decrease time constant.
**1 and 2**  | Select previous/next object.
**n**        | Toggle wiping of OpenGL screen.
**z**        | Dump all objects to an XYZ file.
**Spacebar** | Pause.

Command line arguments
----------------------
Argument | Action
---------|-------
**-f (filename)** | Specifies a posdata.dat to use. Falls back on simconf.conf's setting.
**--threads (int)** | Use (int) threads to speed up force calculation. Splits objects between cores.
**--dumplevel (uint)** | Sets the dumplevel. 1=XYZ file every second. 2=every frame.
**--novid** | Disables SDL window creation and any OpenGL functions in the main loop.
**--fullogl** | Initialize full OpenGL instead of ES. Look in simconf.conf to change versions.
**--nosync** | Disables vertical synchronization.
**--quiet** | Suppresses all informational messages. Errors are still printed to stderr.
**--help** | Print possible arguments and exit.

simconf.conf
------------
This file contains the program configuration. Most of the settings have self-explanatory names, with any remarks written as comments. Quotation marks (`variable = "value"`) around the value are **absolutely neccessary**. Section tags (`[section]`) are not required.

posdata.dat
-----------
This file determines the properties of all the objects to be simulated. The comment at the top describes the correct order and structure. Object numbers **must** be sequential, starting from 1. The argument `-f (filename)` overrides the filename in simconf.conf, however, the program will fall back on the configuration file should no such filename exists. If both filenames do not exist, the program will exit with a return value of 1. Currently, no error checking is present for this file, therefore should the physics engine misbehave, inspect both the objects table printed on the terminal and your posdata file. In case nothing appears to be wrong, file an [issue][issue-tracker].

Contacts
-------

 * **IRC Channel**: `#physengine` on `chat.freenode.net`
 * **Email**: `atomnuker@gmail.com`

Contributing
------------
Got a bug fix? New feature? Sent a pull request here on Github. Just please use the project's coding style (which is the same as the Linux kernel's coding style except without the 80 characters per line limit).
