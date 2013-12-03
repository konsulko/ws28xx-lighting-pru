AM335x remoteproc firmeware for driving WS281x RGB LEDS
=======================================================

WS2812 Datasheet: http://www.adafruit.com/datasheets/WS2812.pdf

Universe Pin Mappings on Beagebone Black/White:

* 0  -> P8\_45 (PRU1 R30\_0)
* 1  -> P8\_46 (PRU1 R32\_1)
* 2  -> P8\_43 (PRU1 R32\_2)
* 3  -> P8\_44 (PRU1 R32\_3)
* 4  -> P8\_41 (PRU1 R32\_4)
* 5  -> P8\_42 (PRU1 R32\_5)
* 6  -> P8\_39 (PRU1 R32\_6)
* 7  -> P8\_40 (PRU1 R32\_7)
* 8  -> P8\_27 (PRU1 R32\_8)
* 9  -> P8\_29 (PRU1 R32\_9)
* 10 -> P8\_28 (PRU1 R32\_10)
* 11 -> P8\_30 (PRU1 R32\_11)


Example usage:

		 $ echo BB-BONE-LIGHTING-00 > /sys/devices/bone\_capemgr.\*/slots 
		 $ minicom -D /dev/vport0p0

		 PRU#0> ?
		 Help
		  s <universe>              select universe 0-11
		  b 	                    blanks slots 0-255
		  w <num> <v1>.<v2>.<v3>    write 24-bit GRB value to slot number
		  l                         latch data out the PRU1

		 PRU#0> s 0
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

Important Notes:

* Blanking only has to be called once per universe unless you are changing slot count
* After latching data updating values is locked till the transaction completes
 * To avoid double buffering we have to be sure all data is written
 * Accessing the same PRU shared memory will stall one or both PRUs
