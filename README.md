# Quilt Controller

Patchwork quilt's light installation is controlled with the **Arduino Lilypad** integrated inside the quilt. The Lilypad is connected to an **Xbee RF module**. When turned on, it continuously listens for incoming commands from Xbee. Lilypad connects to the Xbee using [**Software Serial**](https://www.arduino.cc/en/Reference/SoftwareSerial). The code, will insert the commands into the queue and execute them one after another. It'll signal the election of the command over radio frequency, so that the sender/server can keep track of the quilt's state.

The Arduino sees the quilt as an 8x8 LED module. I am using maxim's [**MAX7219**](https://www.maximintegrated.com/en/products/power/display-power-control/MAX7219.html) LED driver to control the lighting installation on the quilt. I am using Arduino's SPI pins to update MAX7219 states. That will give us the more than clock rate to smoothly update the screen. For graphics displayed on the quilt, I am using [**Adafruit's GFX Library**](https://github.com/adafruit/Adafruit-GFX-Library).



Patchwork is a workshop activity first exhibited In Siggraph 2015 by:

[**Katherine Moriwaki**](http://www.kakirine.com/) & [**Saman Tehrani**](http://samantehrani.com)