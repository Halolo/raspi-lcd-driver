raspi-lcd-driver
================

*[ERM12864-2](http://hackerstore.nl/PDFs/ERM12864-2.pdf)* LCD graphic module control for *Raspberry Pi*.

Written for a [Raspberry Pi 1 model B](https://www.raspberrypi.org/products/model-b/) running [ArchLinux ARM](https://archlinuxarm.org/platforms/armv6/raspberry-pi).

Hardware setup
--------------

The *ERM12864-2* Display is interfaced with the *Raspberry Pi* using *GPIOs*

**The GPIOs are on TTL levels (+5V) while the LCD I/Os level is 3.3V => a voltage adaptation circuit is needed**

Mapping
-------

    RS   => GPIO 23  
    RW   => GPIO 24  
    E    => GPIO 11  
    RST  => GPIO 9  
    CS1  => GPIO 10  
    CS2  => GPIO 22  
    DB0  => GPIO 18  
    DB1  => GPIO 15  
    DB2  => GPIO 14  
    DB3  => GPIO 2  
    DB4  => GPIO 3  
    DB5  => GPIO 4  
    DB6  => GPIO 17  
    DB7  => GPIO 27  

Software componemts
-------------------

This repository provides *3 components*

###liblcd.so

Provides an *interface* to send commands to the LCD.

###lcd-controllerd

Designed to be run in *background* (e.g. with *start-stop-daemon*).
Interfaced with the LCD using the *liblcd.so*.
Open an *UNIX socket* and listen for messages.

###lcd-controllerd-cli

Used to send messages to the *lcd-controllerd*'s socket.

    lcd-controllerd-cli -h for help.

Use it for:
  - turn ON/OFF the LCD
  - Clear the display
  - Print a 128 × 64 Black and White *BMP*
  - Print text
  - Store the current screen in a *BMP* file
  - Stop the *lcd-controllerd* daemon

Exemples
--------

    lcd-controllerd-cli -p /usr/local/share/lcd-controllerd-cli/linux.bmp
    lcd-controllerd-cli -t "Uptime: $(uptime | sed -n 's/^.*up\s*\([0-9]*\)\s*days.*$/\1/p') days"


Installation
------------

**On target**

    ./make.sh -b
    cd build
    sudo make install

*liblcd.so => /usr/local/lib*  
*lcd-controllerd => /usr/local/bin*  
*lcd-controllerd-cli => /usr/local/bin*  
*linux.bmp => /usr/local/share/lcd-controllerd-cli*  

**Cross compilation**

    ./make.sh -h

for help
