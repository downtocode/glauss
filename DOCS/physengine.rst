physengine
##########

############################
a physics simulations engine
############################

:Copyright: GPLv3
:Manual section: 1
:Manual group: science

SYNOPSIS
========

| **physengine** -f [file] [flags]

DESCRIPTION
===========

**physengine** is a physics simulations engine written in C, capable of being
used in a variety of situations, from n-body simulations on planetary orbits
to barnes-hut galactical simulations.

CONTROLS
========

The following list of controls has power when the program is run in graphical mode.

*Keyboard Control*
------------------

[ and ]
    Halve/double time constant.

, and .
    Select next/previous object.

ENTER
    Press and type a number to jump to a specific object.

R
    Reset view.

Z
    Save current system to an *XYZ* file.

S
    Create a *PNG* screenshot.

F
    Switch between fullscreen and windowed mode.

SPACEBAR
    Pause/unpause.

ESC or Q
    Quit program.

BACKSPACE
    Shut the physics threads completely.

MINUS
    Change the physics algorithm(threads need to be completely off).

*Mouse controls*
----------------

LMB
    Hold and drag to rotate camera.

SCROLL
    Zoom in and out.

MMB
    Hold and drag to move camera.

FLAGS
=====
The following flags can be used:

``--novid`` - No argument
    Will run the program without any video outputting enabled.

``--bench`` - No argument
    Will run the program in benchmark mode. Quits and reports stats after ``timer`` runs once.

``--version``, ``-V`` - No argument
    Will report version as well as compile time and date.

``--help``, ``-h`` - No argument
    Will report on flags and usage and try to convince the user to read this manual.

``--file``, ``-f`` - *String*
    Input file. See the section below for more information.

``--timer``, ``-r`` - *Float*
    Configures how often to update the frames per second and ``benchmark`` duration.

``--algorithm``, ``-a`` - *String*
    Sets the algorithm to use. Use "help" to list all available.

``--threads``, ``-t`` - *Unsigned Integer*
    Sets the amount of threads to use. "0" will try to autodetect and use that.

``-log``, ``-l`` - *String*
    Sets the log file to which to save all command line output at maximum ``verbosity``.

``--verb``, ``-v`` - *Short Unsigned Integer*
    Sets how much information to output to *STDOUT*.


These flags will always override anything set inside the configuration input file.


CONFIGURATION FILES
===================

Syntax
------

The program takes input in the form of a Lua script. It's used to configure both
the system being simulated as well as the programs by setting variables. The Lua
script **has** to contain a table named "settings", which is the only hardcoded object.
See below to set other Lua function names.

*Option variables*
------------------
Used to toggle and adjust options. Some may intersect with command line arguments, 
however most do not.

``threads`` - *Unsigned Integer*
    Set the amount of threads to use. Overridden by argument.
``dt`` - *Float*
    Set the time constant.
``algorithm`` - *String*
    Set the algorithm to use. Specify help here or in argument to list all.
``bh_ratio`` - *Float*
    Algorithm specific. Adjusts accuracy and speed.
``bh_lifetime`` - *Short Unsigned Integer*
    Algorithm specific. Set empty cell lifetime before its deletion.
``bh_heapsize_max`` - *Unsigned Integer(bytes, size_t)*
    Algorithm specific. Set limit on maximum octrees per thread.
``bh_tree_limit`` - *Short Unsigned Integer, 1 to 8*
    Algorithm specific. Sets limit on threads per octree. Increase to spread distribution.
``bh_single_assign`` - *Boolean*
    If only a single thread is used will still split the octree normally. Debugging.
``spawn_funct`` - *String*
    Name of function to read objects from
``timestep_funct`` - *String*
    Function to execute upon timestep completion
``exec_funct_freq`` - *Integer*
    Auto timestep_funct run frequency
``lua_expose_obj_array`` - *Boolean*
    Expose the object array(updated upon timestep completion), may affect performance.
``screenshot_template`` - *String*
    Template to use in screenshot file creation. Standard sprintf syntax.
``file_template`` - *String*
    Template to use in state file(XYZ) creation. Standard sprintf syntax.
``fontname`` - *String*
    Specify the font type to be used. Example: "Liberation Sans".
``fontsize`` - *Unsigned Integer*
    Fontsize adjustment.
``dump_sshot`` - *Unsigned Integer*
    Specify the frequency of screenshots taken. 1 - every step, 2 - every two steps.
``dump_xyz`` - *Unsigned Integer*
    Specify the frequency of state dumps created. 1 - every step, 2 - every two steps.
``width`` - *Integer*
    Set window width in pixels.
``height`` - *Integer*
    Set window height in pixels.
``epsno`` - *Double*
    Electric force constant. Set either this or ``elcharge`` to 0 to disable.
``elcharge`` - *Double*
    Electrical unit conversion, multiplies object charge.
``gconst`` - *Double*
    Gravitational force constant. Set to 0 to disable such force calculations.
``verbosity`` - *Integer, 0 to 10.*
    Specify the amount of information being outputted to the terminal.

*Object specific variables*
---------------------------
Every object has to be a part of an array, which has to be returned with the first
value of a function named *spawn_objects*. Second returned value should specify
the number of elements inside the array.

``posx``, ``posy``, ``posz`` - *Double*
    Used to position an object.
``velx``, ``vely``, ``velz`` - *Double*
    Initial velocity
``charge`` - *Double*
    Charge, if the object should have one.
``mass`` - *Double*
    Mass. Reqired to be non-zero for every object, as otherwise the program will crash.
``radius`` - *Float*
    Radius. Optional, to be used in collisions.
``atom`` - *String*
    If the object should represent an atom. Use Short Standard Periodic table notation.
``import`` - *String*
    Will import from a file. Currently, Waveform 3D *Obj*, *XYZ* and *PDB* files are supported.
``ignore`` - *Bool*
    Set this flag to prevent the object from being moved. Will still affect others.

FILE IMPORTING
==============
Work in progress, support for more files will be added in the future. In any case, 
the user can themselves write any reader in the Lua config file if needed.

- Waveform 3D Obj files is supported. Only vertices will be imported.

- XYZ importing is also functional and will correctly import atom information as well.

- PDB file importing has been partially implemented, with several ATOM variables used.

ENVIRONMENT VARIABLES
=====================
Some libraries used in this program can be controlled using environmental variables:

``SDL_VIDEODRIVER``
    Sets which video driver to use.
``SDL_VIDEO_X11_MOUSEACCEL``
    Sets mouse sensitivity for the X11 video driver.
``SDL_DEBUG``
    Useful when debugging input problems.

For a more exaustive list consult the SDL2 library manual.

SIGNAL HANDLING
===============
The following signal functions have been implemented:

``SIGINT``
    Will stop the threads, close all files, free all memory and quit.
``SIGUSR1``
    Will report the current status of the simulation.

EXIT CODES
==========

Normally **physengine** returns *0* as exit code upon quitting manually. Special
cases are listed below:

    :1: General errors.
    :2: File not found.
    :3: Memory allocation errors/out of memory/memory limit reached.

EXAMPLES
========

*Loading a standard simulation:*
    ``physengine -f simconf.lua``

*Don't simulate anything, just display:*
    ``physengine -f simconf.lua -a none``

*Dummy load sim, will pretend to use n-body but won't actually move anything:*
    ``physengine -f simconf.lua -a null``

*Simulate using the n-body algorithm using 3 threads:*
    ``physengine -f simconf.lua -t 3 -a n-body``

*Use the Barnes-Hut algorithm with 4 cores and create a logfile:*
    ``physengine -f simconf.lua -t 4 -a barnes-hut -l phys.log``

CONTACT
=======

For contact:

*IRC*
-----
``#physengine`` on *Freenode*, look for atomnuker

*E-mail*
--------
``Rostislav Pehlivanov`` - *atomnuker@gmail.com*

AUTHORS
=======

physengine was written by Rostislav Pehlivanov.
This manpage was written by the author.
