physengine
================

This program aims to implement a physics engine whilst aiming for maximum performance. Uses SDL2 and the Freetype libraries for display. Implemented in pure C.

Description
-----------
This physics engine uses the Velocity Verlet integration method to simulate particle movement and SDL2 to display whatever's happening. Right now, Gravity and Electromagnetism are simulated, along with a spring link feature.

Compiling
---------
You need make, a sane C compiler(which supports C99), SDL2(and whatever that requires - like OpenGL dev libraries) and the freetype libarry. Keep in mind many distributions like to place the SDL2 development libraries in a variety of sublocations within the /usr/include directory. In Debian's case, the headers were within a directory called "SDL2", so just take a look down at your headers directory and if required modify main.c which contains the only SDL2 includes.
Once you have those three things, run make.

Running
-------
Just run the physengine executable. Modify the posdata.dat to whatever you want to simulate.

Controls
--------
LMB   - Select object under cursor

RMB   - Deselect

Left  - Halve the time dt

Right - Double the time dt

Space - Pause

posdata.dat
-----------
This file determines the positions, velocities, accerelation(current), mass, charge and particles that are linked. Currently, only two dimensions are supported, so for any vectorial parameter you have to provide the two components. Links must be seperated by commas without any spaces.

Development comment
-------------------
Using SDL2 was a bother at the start as there is no proper tutorial(yet). SDL2_TTF was also used but due to a memory hole it created was eventually dropped completely out. Using freetype2 directly shall be used for OSD in the future.
You might see a lot of comments about black magic in commit messages and actual comments, but worry not, this program contains 0% blasphemous code. Rather, those words were chosen because the author was unable to figure out how something works, like how global variables propagate throughout functions in different files.

Possible applications
---------------------
Can be modified into a Space Invaders clone in less than 10 minutes.

~~Can be modified into a PONG clone in less than 10 minutes.~~
