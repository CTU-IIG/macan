MaCAN
=====

Message Authenticated CAN library & tools written in C.  

This library implements MaCAN protocol [1, [2]], which improves [Controller Area Network](https://en.wikipedia.org/wiki/CAN_bus) security. MaCAN network consists from a
keyserver, a timeserver and one or more nodes (a.k.a. ECUs). Supported
platforms currently include **GNU/Linux** and **Infineon Tricore
TC1798** and STM32.

The project is divided into several, relatively independent parts:

### MaCAN library

The MaCAN library is used in any node (incl. keyserver and timeserver)
participating in MaCAN communication. Library's API is defined in
`macan.h` file. Other functions are internal to the library. This API
is documented in Doxygen docs located in `docs/` directory. For actual
code examples on how to use this library, see Demos section below.

### Keyserver

Provides session keys to other nodes. When run on Linux, the following
command line parameters are supported:

* -c *configuration.so*  

  Path to the shared object with configuration.

* -k *allkeys.so*  

  Path to the shared object with long term keys of every node.

### Timeserver

Provides plain and authenticated time signals to nodes. When run on
Linux, the following command line parameters are supported:

* -c *configuration.so*  

  Path to the shared object with configuration.

* -k *key.so*  

  Path to the shared object with timeserver's long term key.

### MaCAN Monitor


Dumps traffic on CAN bus and interprets MaCAN frames, useful for
debugging (GNU/Linux only). The following command line parameters are
supported:

* -c *configuration.so*  

  Path to the shared object with configuration.

Configuration
-------------

MaCAN configuration is done statically by filling the following
information (as defined in `macan.h`):

* `struct macan_config` - main MaCAN configuration (number of nodes,
  timing, etc.)
* `struct macan_sig_spec` -- signal configuration
* `ecu2canid_map[]` -- ECU-ID to CAN-ID mapping

This configuration is then compiled into shared object file (on Linux)
or to a static library (on embedded platforms) and used by all
participating nodes.

For security reasons, long-term keys (LTK) are compiled separately
into:

1. object files (one per key) and linked to appropriate nodes (statically or, in case of time server, dynamically).
2. single shared object file (containing all keys), which is used by
   the keyserver only.

Demos
-----

Several example projects are located in the `demos/` directory. Each demo
contains helper shell scripts, which wrap commands and their
options, so you don't need to type them by hand. Scripts are located
in `demos/XYZ/test` directory. Each demo typically includes
following scripts:

* `init-vcan.sh` - initialize virtual CAN interfaces (should be
  invoked first)
* `ks.sh` - launch keyserver
* `ts.sh` - launch timeserver
* `nodeX.sh` - launch node `X` (where `X` is ECU-ID of node)
* `macanmon.sh` - launch MaCAN monitor (optional)


Compiling on GNU/Linux
----------------------

MaCAN library requires a low level cryptographic library
[Nettle](http://www.lysator.liu.se/~nisse/nettle/). To compile the
entire project (incl. library, keyserver, timeserver and demos),
invoke:

    make

in `build/linux`. Compiled binaries are located in the `_compiled`
subdirectory.

Compiling for Infineon TriCore TC1798
-------------------------------------

To compile MaCAN for TriCore, Infineon's proprietary AUTOSAR MCAL
drivers are needed. There is a project file for Altium TASKING IDE
configured to compile node from demo projects, which runs on TC1798
CPU. Only one demo can be compiled at a time, others need to be
excluded from build.

References
----------

[1] Oliver Hartkopp, Cornel Reuber and Roland Schilling, *MaCAN - Message Authenticated CAN*, [ESCAR 2012](https://www.escar.info/index.php?id=208).  
[2] [Ondřej Kulatý, *Message authentication for CAN bus and AUTOSAR software architecture*][2], master's thesis.
[2]: http://rtime.felk.cvut.cz/~sojka/students/Dp_2015_kulaty_ondrej.pdf
