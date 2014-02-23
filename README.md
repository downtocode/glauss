physengine
=================

Description
-----------
This program aims to implement a physics simulation engine capable of doing simulations on various scales, from molecular dynamics to planetary motion, all whilst maintaining maximum performance. Implemented using the SDL2, Freetype 2 and the OpenGL ES 2.0 libraries and written in C. Using standard POSIX threads to implement multithreading.

Compiling
---------
Dependencies: libsdl2, libfreetype, and their header(development) files.
To compile, run make in the current folder. An executable called physengine will be created.

Running
-------
Run the executable. Modify the options in simconf.conf and the object position data (posdata.dat) to change the system being simulated.

Controls
--------
Q/E - Rotate camera around the point set by translation.

Keypad [  - Halve the time dt

Keypad ] - Double the time dt

n - Toggle nowipe mode, do not clean buffer up each frame. Useful for viewing trajectories.

Space - Pause

Command line arguments
----------------------
**--novid** - Disables SDL window creation and any OpenGL functions in the main loop. OpenGL and Freetype are still initialized.

**--quiet** - Suppresses most informational messages. Errors are still printed to stderr

**-f (filename)** - Specifies a posdata.dat to use. In case no such file exists, the configuration file's posdata option is used. If both are missing, the program exists before anything has been initialized.

**--nosync** - Disables vertical synchronization.

**--threads (int)** - Run the physics engine with (int) amount of threads. The objects are split equally between them. Currently using Round-Robin algorithm for synchronization.

**--help** - Print possible arguments and exit.

simconf.conf
------------
This file contains the program configuration. Each option has to have its value in quotation marks.

posdata.dat
-----------
This file determines the properties of all the objects to be simulated. The comment at the top describes the correct order and structure. Currently, no error detection is present, therefore should the objects misbehave, inspect the file for any oddities.

Progress/Possible applications
------------------------------
Could probably be modified into a Doom clone in a week.

~~Can be modified into an Angry Birds clone in about an hour.~~

~~Can be modified into a Space Invaders clone in less than 10 minutes.~~

~~Can be modified into a PONG clone in less than 10 minutes.~~
