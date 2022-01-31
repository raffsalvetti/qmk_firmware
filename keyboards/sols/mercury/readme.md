# mercury

![mercury](imgur.com image replace me!)

*A short description of the keyboard/project*

* Keyboard Maintainer: [raffsalvetti](https://github.com/yourusername)
* Hardware Supported: *The PCBs, controllers supported*
* Hardware Availability: *Links to where you can find this hardware*

Make example for this keyboard (after setting up your build environment):

    make mercury:default

Flashing example for this keyboard:

    make mercury:default:flash

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

build: qmk compile -kb sols/mercury -km default
fuse 16MHz external christal:  avrdude -c usbasp -p m32 -U lfuse:w:0xbf:m -U hfuse:w:0x99:m
flash: avrdude -c usbasp -p m32 -U flash:w:sols_mercury_default.hex

