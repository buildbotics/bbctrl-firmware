TinyG is a 6 axis motion control system designed for high-performance on small to mid-sized machines.  This fork contains a modified version of the TinyG firmware.  See the [original TinyG](https://github.com/synthetos/TinyG) for more info.

# Features
* 6 axis motion (XYZABC axes)
* jerk controlled motion for acceleration planning (3rd order motion planning)
* status displays ('?' character)
* XON/XOFF and RTS/CTS protocol over USB serial
* RESTful interface using JSON

# Build Instructions
To build in Linux run:

    make

Other make commands are:

 * **size** - Display program and data sizes
 * **program** - program using AVR dude and an avrispmkII
 * **erase** - Erase chip
 * **fuses** - Write AVR fuses bytes
 * **read_fuses** - Read and pring AVR fuse bytes
 * **clean** - Remove build files
 * **tidy** - Remove backup files

# Links
* [TinyG Wiki](https://github.com/synthetos/TinyG/wiki)
* [TinyG Support Forum](https://www.synthetos.com/forum/tinyg/)
* [TinyG Github](https://github.com/synthetos/TinyG)
