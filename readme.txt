What?
=====
* Infrared remote control to switch on/off 220V AC light or fan and also change the fan speed.
* Temperature is monitored and if it goes below set temperature, the fan is switched off.
* Settings/temperature are displayed in a TFT LCD screen.
* RTC module DS3231 is used for time display.

Why?
====
* The main impetus for this project is for situations where the temperature is warm enough in the night to need the ceiling fan, but becomes cold enough in the morning when it is no longer needed, typically during winters in India.

How?
====
Capacitors taken from a off the shelf fan regulator have been used to vary the speed of the fan.  Using zero detection and firing the triac in version 1 was causing noise in the fan as the current increases from zero immediately.   hence, capacitors are used.    

On the input side,  there are two relays - one for light and the other for fan.  If Arduino is powered off, then the NC ports of the relays are connected and hence wall outlet switches will work normally.  When the Arduinos is powered on, then these relays are active.  

From the light relay, the NO port is connected to a triac which can be trigged to switch on the light.  

The NO port of the fan relay is connected to three capacitors.  There are also four triacs, one connected directly to the output and other through the capacitors.  Based on how many triacs are triggered, the fan of the speed varies.  But if only the triac connected to the output directly is fired, then the fan rotates at max speed.   

There is a 128X128 SPI Color display to show temperature, fan/light status and fan speed. 

The temperature is periodically checked and if it goes below the set temperature, the fan is switched off.   