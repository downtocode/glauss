physengine
=================

This program aims to implement a physics simulations engine whilst aiming for maximum performance. Written in C, using the SDL 2, OpenGL ES 2.0 and the Freetype 2 libraries.

Description
-----------
This physics engine uses a variant of the Velocity Verlet integration method to simulate object movement and SDL2 to display whatever's happening. The forces capable of being simulated are: gravitational, electrostatic and bonding.

Compiling
---------
You need make, a sane C compiler(which supports C99), SDL2(and whatever that requires - like OpenGL dev libraries) and the freetype library. Keep in mind many distributions like to place the SDL2 development libraries in a variety of sublocations within the /usr/include directory. In Debian's case, the headers were within a directory called "SDL2", so just take a look down at your headers directory and if required modify main.c which contains the only SDL2 includes.
Once you have those three things, run make.

Running
-------
Just run the physengine executable. Modify the options in simconf.conf and the object position data (posdata.dat) to change the system being simulated.

Controls
--------
LMB   - Select object under cursor

RMB   - Deselect

Left/Right/Up/Down - Translate camera around the X and Y axis.

Mouse wheel - Translate camera on the Z axis.

WS/AD/QE - Rotate camera around the point set by translation.

Keypad +  - Halve the time dt

Keypad - - Double the time dt

Space - Pause

Command line arguments
----------------------
**--quiet** - Suppresses most informational messages. Errors are still printed to stderr

**--novid** - Disables SDL window creation and any OpenGL functions in the main loop. OpenGL and Freetype are still initialized.

**-f (filename)** - Specifies a posdata.dat to use. In case no such file exists, the configuration file's posdata option is used. If both are missing, the program exists before anything has been initialized.

simconf.conf
------------
This file contains the program configuration. Each option has to have its value in quotation marks.

posdata.dat
-----------
This file determines the positions, velocities, mass, charge and particles that are linked. Links must be seperated by commas without any spaces. Keep in mind that in case there are two objects in the same place/a line is duplicated, the physics engine will silently scatter all the objects to infinity.

Planned features
----------------
Right now getting basic molecular dynamics simulations and an N-body system of orbiting bodies is the aim of the project. In the future, getting a binary space partitioning would be nice.
Being developed right now is an algorithm to determine the linked objects, find their center of mass/charge and use that center to reduce the interobject force calculations.

Progress/Possible applications
------------------------------
Could probably be modified into a Doom clone in a week.

~~Can be modified into an Angry Birds clone in about an hour.~~

~~Can be modified into a Space Invaders clone in less than 10 minutes.~~

~~Can be modified into a PONG clone in less than 10 minutes.~~
