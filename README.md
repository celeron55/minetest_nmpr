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

## License

### Everything not listed separately

Creative Commons Public Domain Dedication (CC0 1.0 Universal (CC0 1.0)).

### /data/tf.jpg

Owned by someone called Carlos Ramirez and used originally in the NMPR with
permission whatsoever. Maybe having it here falls under fair use or something,
because it is what the NMPR contained and this repository attempts to be
historically accurate.

### /data/fontlucida.png

This is from Irrlicht's examples so maybe it falls under the zlib license or
something.

