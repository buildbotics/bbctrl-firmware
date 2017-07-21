The Buildbotics firmware is a 4 axis motion control system designed for
high-performance on small to mid-sized machines.  It was originally
derived from the [TinyG firmware](https://github.com/synthetos/TinyG).

# Features
 * 4 axis motion
 * jerk controlled motion for acceleration planning (3rd order motion planning)

# Build Instructions
To build in Linux run:

    make

Other make commands are:

 * **size** - Display program and data sizes
 * **program** - program using AVR dude and an avrispmkII
 * **erase** - Erase chip
 * **fuses** - Write AVR fuses bytes
 * **read_fuses** - Read and print AVR fuse bytes
 * **clean** - Remove build files
 * **tidy** - Remove backup files
