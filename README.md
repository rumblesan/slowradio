# Slow Radio

A slow radio station, that plays a constant stream of stretched audio, based on an implementation of the [Paul Stretch algorithm](http://hypermammut.sourceforge.net/paulstretch/).

All audio should be under the Creative Commons Attribution License.

You can listen to it at [slowradio.rumblesan.com](http://slowradio.rumblesan.com).

# Running

The only configuration Slow Radio needs is the cfg file that has settings for the file reader, stretcher, encoder and broadcaster. The *radio.cfg* file should give a good example.

Currently only Ogg is a supported input type, and only icecast as a broadcast target.

There are [docker images available](https://hub.docker.com/r/rumblesan/slowradio/) if you want to run your own stretched radio. To run it, just give it a folder containing the config and audio folders to mount as a volume and it should just work.
The docker images are currently pretty big as I've not put much effort into making them smaller.

# Building

Slow Radio is built in C, and uses CMake. It has dependencies on a number of third-party libraries as well as two of my own.

## Dependencies

Third-party libraries
* libshout
* libconfig
* libvorbis
* libvorbisfile
* libvorbisenc
* libogg

My own libraries
* [bclib](https://github.com/rumblesan/bclib)
* [libpstretch](https://github.com/rumblesan/libpstretch)

## Contact

Drop me an email at guy@rumblesan.com

## License

BSD License.
