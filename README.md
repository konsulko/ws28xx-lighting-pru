AM335x remoteproc firmware with command interface
=================================================

WS2812 interface
----------------

From https://github.com/mranostay/ws28xx-lighting-pru

WS2812 Datasheet: http://www.adafruit.com/datasheets/WS2812.pdf

Universe Pin Mappings on Beagebone Black/White:

* 0  -> P8\_45 (PRU1 R30\_0)
* 1  -> P8\_46 (PRU1 R30\_1)
* 2  -> P8\_43 (PRU1 R30\_2)
* 3  -> P8\_44 (PRU1 R30\_3)
* 4  -> P8\_41 (PRU1 R30\_4)
* 5  -> P8\_42 (PRU1 R30\_5)
* 6  -> P8\_39 (PRU1 R30\_6)
* 7  -> P8\_40 (PRU1 R30\_7)
* 8  -> P8\_27 (PRU1 R30\_8)
* 9  -> P8\_29 (PRU1 R30\_9)
* 10 -> P8\_28 (PRU1 R30\_10)
* 11 -> P8\_30 (PRU1 R30\_11)

*black sheep bits - disabled in overlay by default to allow eMMC usage*

* 12 -> P8\_21 (PRU1 R30\_12)
* 13 -> P8\_20 (PRU1 R30\_13)


Example usage *(low speed virtio serial usage)*:

		 $ echo BB-BONE-PRU-05 > /sys/devices/bone\_capemgr.\*/slots 
		 $ minicom -D /dev/vport0p0

		 PRU#0> ?
		 Help
		  s <universe>              select universe 0-13
		  b 	                    blanks slots 1-170
		  m <val>                   max number of slots per universe 0-169
		  w <num> <v1>.<v2>.<v3>    write 24-bit GRB value to slot number
		  l                         latch data out the PRU1

		 PRU#0> m 30
		 PRU#0> b
		 PRU#0> w 0 ff.00.00
		 PRU#0> w 1 00.ff.00
		 PRU#0> w 2 00.00.ff
		 ...
		 PRU#0> s 1
		 PRU#1> b
		 PRU#1> w 0 ff.ff.00
		 PRU#1> w 1 00.ff.ff
		 PRU#1> w 2 ff.ff.ff
		 ...
		 PRU#1> l

	         ** Blinky Lights! **

Examples usage *(HIGH speed ioctl/spidev usage)*:

		$ cat /proc/misc | grep pru_leds
		 59 pru_leds
		$ mknod /dev/pruleds0.0 c 10 59
		$ echo "m 30" > /dev/pruleds0.0

		** Install OLA and use the examples/spidev-pru.conf **


Important Notes:

* Disable HDMI out on BeagleBone Black to free up PRU pins
 * "optargs=capemgr.disable\_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN" in /boot/uEnv.txt
* Blanking only has to be called once per universe unless you are changing slot count
* Disabling unused slots will increase FPS and reduce the data written
* After latching data updating values is locked till the transaction completes
 * To avoid double buffering we have to be sure all data is written
 * Accessing the same PRU shared memory will stall one or both PRUs


duino interface
----------------

From https://github.com/ecto/duino

A framework for working with Arduinos in node.js

![arduino](http://i.imgur.com/eFq84.jpg)

# install

    npm install duino

# usage

````javascript
var arduino = require('duino'),
    board = new arduino.Board();

var led = new arduino.Led({
  board: board,
  pin: 13
});

led.blink();
````

# what ಠ_ಠ

The way this works is simple (in theory, not in practice). The Arduino listens for low-level signals over a serial port, while we abstract all of the logic on the Node side.

1.  Plug in your Arduino
2.  Upload the C code at `./src/du.ino` to it
3.  Write a simple **duino** script
4.  ?????
5.  Profit!

# libraries

##board

````javascript
var board = new arduino.Board({
  device: "ACM"
});
````
The board library will attempt to autodiscover the Arduino.
The `device` option can be used to set a regex filter that will help the library when scanning for matching devices.
**Note**: the value of this parameter will be used as argument of the grep command

If this parameter is not provided the board library will attempt to autodiscover the Arduino by quering every device containing 'usb' in its name.

````javascript
var board = new arduino.Board({
  debug: true
});
````

Debug mode is off by default. Turning it on will enable verbose logging in your terminal, and tell the Arduino board to echo everthing back to you. You will get something like this:

![debug](http://i.imgur.com/gBYZA.png)

The **board** object is an EventEmitter. You can listen for the following events:

* `data` messages from the serial port, delimited by newlines
* `connected` when the serial port has connected
* `ready` when all internal post-connection logic has finished and the board is ready to use

````javascript
board.on('ready', function(){
  // do stuff
});

board.on('data', function(m){
  console.log(m);
}
````

###board.serial

Low-level access to the serial connection to the board

###board.write(msg)

Write a message to the board, wrapped in predefined delimiters (! and .)

###board.pinMode(pin, mode)

Set the mode for a pin. `mode` is either `'in'` or `'out'`

###board.digitalWrite(pin, val)

Write one of the following to a pin:

####board.HIGH and board.LOW

Constants for use in low-level digital writes

###board.analogWrite(pin,val)

Write a value between 0-255 to a pin

##led

````javascript
var led = new arduino.Led({
  board: board,
  pin: 13
});
````

Pin will default to 13.

###led.on()

Turn the LED on

###led.off()

Turn the LED off

###led.blink(interval)

Blink the LED at `interval` ms. Defaults to 1000

###led.fade(interval)

Fade the to full brightness then back to minimal brightness in `interval` ms. Defaults to 2000

###led.bright

Current brightness of the LED

##lcd

This is a port of the [LiquidCrystal library](http://arduino.cc/en/Reference/LiquidCrystal) into JavaScript. Note that communicating with the LCD requires use of the synchronous `board.delay()` busy loop which will block other node.js events from being processed for several milliseconds at a time. (This could be converted to pause a board-level buffered message queue instead.)

````javascript
var lcd = new d.LCD({
  board: board,
  pins: {rs:12, rw:11, e:10, data:[5, 4, 3, 2]}
});
lcd.begin(16, 2);
lcd.print("Hello Internet.");
````

In `options`, the "pins" field can either be an array matching a call to any of the [LiquidCrystal constructors](http://arduino.cc/en/Reference/LiquidCrystalConstructor) or an object with "rs", "rw" (optional), "e" and a 4- or 8-long array of "data" pins. Pins will default to `[12, 11, 5, 4, 3, 2]` if not provided.

###lcd.begin(), lcd.clear(), lcd.home(), lcd.setCursor(), lcd.scrollDisplayLeft(), lcd.scrollDisplayRight()

These should behave the same as their counterparts in the [LiquidCrystal library](http://arduino.cc/en/Reference/LiquidCrystal).

###lcd.display(on), lcd.cursor(on), lcd.blink(on), lcd.autoscroll(on)

These are similar to the methods in the [LiquidCrystal library](http://arduino.cc/en/Reference/LiquidCrystal), however they can take an optional boolean parameter. If true or not provided, the setting is enabled. If false, the setting is disabled. For compatibility `.noDisplay()`, `.noCursor()`, `.noBlink()` and `.noAutoscroll()` methods are provided as well.

###lcd.write(val), lcd.print(val)

These take a buffer, string or integer and send it to the display. The `.write` and `print` methods are equivalent, aliases to the same function.

###lcd.createChar(location, charmap)

Configures a custom character for code `location` (numbers 0–7). `charmap` can be a 40-byte buffer as in [the C++ method](http://arduino.cc/en/Reference/LiquidCrystalCreateChar), or an array of 5-bit binary strings, or a 40-character string with pixels denoted by any non-space (`' '`) character. These bits determine the 5x8 pixel pattern of the custom character.

````javascript
var square = new Buffer("1f1f1f1f1f1f1f1f", 'hex');

var smiley = [
  '00000',
  '10001',
  '00000',
  '00000',
  '10001',
  '01110',
  '00000'
];

var random =
  ".  .." +
  " . . " +
  ". . ." +
  " . . " +
  " ..  " +
  ".  . " +
  " .  ." +
  ".. .." ;

lcd.createChar(0, square);
lcd.createChar(1, smiley);
lcd.createChar(2, random);
lcd.setCursor(5,2);
lcd.print(new Buffer("\0\1\2\1\0"));    // NOTE: when `.print`ing a string, 'ascii' turns \0 into a space
````

##piezo

````javascript
var led = new arduino.Piezo({
  board: board,
  pin: 13
});
````
Pin will default to 13.

###piezo.note(note, duration)

Play a pre-calculated note for a given duration (in milliseconds).

`note` must be a string, one of `d`, `e`, `f`, `g`, `a`, `b`, or `c` (must be lowercase)

###piezo.tone(tone, duration)

Write a square wave to the piezo element.

`tone` and `duration` must be integers. See code comments for math on `tone` generation.

##button

````javascript
var button = new arduino.Button({
  board: board,
  pin: 13
});
````
Pin will default to 13.

Buttons are simply EventEmitters. They will emit the events `up` and `down`. You may also access their `down` property.

````javascript
button.on('down', function(){
  // delete the database!
  console.log('BOOM');
});

setInterval(function(){
  console.log(button.down);
}, 1000);
````

##ping

See: <http://arduino.cc/en/Tutorial/Ping>

````javascript
var range = new arduino.Ping({
  board: board
});

range.on('read', function () {
  console.log("Distance to target (cm)", range.centimeters);
});
````

##servo

````javascript
var servo = new arduino.Servo({
  board: board
});

servo.write(0);
servo.write(180);
````
Pin will default to 9. (Arduino PWM default)

###servo.sweep()

Increment position from 0 to 180.

###servo.write(pos)

Instruct the servo to immediately go to a position from 0 to 180.

##motor

##potentiometer

# protocol

Each message sent to the Arduino board by the **board** class has 8 bytes.

A full message looks like this:

    !0113001.

`!` Start
`01` Command (digitalWrite)
`13` Pin number
`001` Value (high)
`.` Stop

I was drunk. It works.

##command

What is implemented right now:

*  `00` pinMode
*  `01` digitalWrite
*  `02` digitalRead
*  `03` analogWrite
*  `04` analogRead
*  `97` ping
*  `98` servo
*  `99` debug

##pin

Pins can be sent as an integer or a string(`1`, `2`, `"3"`, `"A0"`)

##value

*  `board.LOW`(`0`)
*  `board.HIGH`(`255`)
*  integer/string from `0`-`255` for PWM pins

# license

Copyright (c) 2011 Cam Pedersen <cam@onswipe.com>
Copyright (c) 2013 Matt Ranostay
Copyright (c) 2013 Jason Kridner, Texas Instruments, Inc.

See COPYING
