# Minetest NMPR

This version of the initial network multiplayer release of Minetest has been
modified to compile on modern GNU/Linux distributions, with Irrlicht 1.7 or 1.8,
and with JThread 1.2 or 1.3.

As the NMPR is rather minimalistic and it also has some historical significance,
it is useful for educational purposes.

## How to use

Installing dependencies on Fedora:
```
$ dnf install irrlicht-devel jthread-devel
```

Installing dependencies on Debian or Ubuntu:
```
$ apt-get install libirrlicht-dev libjthread-dev
```

Building:
```
$ make
```

Running:
```
$ cd bin  # "bin" is required to be the working directory so that data is located at "../data"
$ ./test
```

