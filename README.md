# MaCAN
Message Authenticated CAN library &amp; tools written in C.  

This library implements MaCAN protocol to improve security of messages on CAN bus. For more details about the protocol, see paper [1]. Supported platforms currently include **GNU/Linux** and **Infineon Tricore TC1798** (for nodes only).  

Project is divided into several, relatively independent parts:

### MaCAN library
The library is compiled into `libmacan.a` file and is used in any node (incl. keyserver and timeserver) participating in MaCAN communication. Library's API is defined in `macan.h` file. Other functions are internal to the library. This API is documented in Doxygen docs located in `docs/` directory. For actual code examples on how to use this library, see Demos section below.

### Keyserver
Provides session keys to other nodes (GNU/Linux only).

* -c *configuration.so*  
Path to the shared object with configuration.

* -k *allkeys.so*  
Path to the shared object with long term keys of every node.

### Timeserver
Provides plain and authenticated time to nodes (GNU/Linux only).

* -c *configuration.so*  
Path to the shared object with configuration.

* -k *key.so*  
Path to the shared object with timeserver's long term key.

### MaCAN Monitor
Dumps traffic on CAN bus and interprets MaCAN frames, useful for debbuging (GNU/Linux only).

* -c *configuration.so*  
Path to the shared object with configuration.

## Configuration

MaCAN configuration is done statically by filling following (defined in `macan.h`):

* `struct macan_sig_spec` - signal configuration
* `ecu2canid_map[]` - ECU-ID to CAN-ID mapping
* `struct macan_config` - main MaCAN configuration 

This configuration is then compiled into shared object file and used by all participating nodes.

For security reasons, Long Term Keys (LTK) are compiled separately into: 

1. shared object files (one per key) and linked to nodes at runtine.
2. single shared object file (containing all keys), which is used by keyserver only.

## Demos

Several example projects are located in `demos/` directory. Each demo contains helper scripts written in BASH, which wrap commands and their options, so you don't need to type them by hand. Scripts are located in `demos/demoXX/test` directory. Each demo typically includes following scripts:

* `init-vcan.sh` - initialize virtual CAN interfaces (should be invoked first)
* `ks.sh` - launch keyserver
* `ts.sh` - launch timeserver
* `nodeX.sh` - launch node `X` (where `X` is ECU-ID of node)
* `macanmon.sh` - launch MaCAN monitor (optional)


## Compiling on GNU/Linux

MaCAN library requires Low level crypto library [Nettle](http://www.lysator.liu.se/~nisse/nettle/).
To compile entire project (incl. library, keyserver, timeserver and demos), invoke:
```
make
```
Compiled binaries are located in `_compiled` directory.

## Compiling for Infienon Tricore TC1798

There is a project file for Altium TASKING IDE configured to compile node from demo projects, which runs on TC1798 CPU. Only one demo can be compiled at a time, others need to be excluded from build.

## References

[1] Oliver Hartkopp, Cornel Reuber and Roland Schilling - *MaCAN - Message Authenticated CAN*
