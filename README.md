# sonos-esp

Basic Arduino code for connecting ESP8266 to Sonos systems.

Right now there is one sketch which provides a web interface for connecting the ESP8266 to a wifi network, and one sketch that just reads the current volume of a Sonos divice. 

The two sketches will be integrated into one standalone sonos control device. 

To connect to an accesspoint:

1. Connect the ESP8266 to power, and connect a pushbutton between ground and input pin D6. Internal pullup is used, no external resistor needed
2. The ESP tries to connect to last known network. If it works, the built in led will turn off, but give a short blink every 15th second to indicate it is alive
3. If it does not succeed in connecting to a network, the built in led will turn on, but here too give a short blink every  15th second
4. Press the button to put the ESP in configure state. The built in led will start blinking 5 blinks per soecond and the ESP is now a wifi access point with SSID=configure
5. Connect a computer to the wifi network "configure"
6. Open a browser and go to http://192.168.4.1
7. Enter ssid and password in webform and connect. The led blinks a  little slower
8. A messege should appear if you succeeded or not.
9. If you succeeded, the ESP leaves configure state and is now connected
10. If you did not succeed you can try again
11. If you pressed the button accidentaly, you can press it again to leave configure mode

## Todo

- Test the sonsos discover part with several sonos devices
- Merge sonoscontrol and connecttoaccesspoint into one file
- Add rotational encoder for volume setting

## Thanks

Much help and inspiration found here:

- https://gist.github.com/dogrocker/f998dde4dbac923c47c1 - very basic connection manager that lacks some vital parts  
- https://github.com/tzapu/WiFiManager ESP8266 WiFi Connection manager with web captive portal. Works well but I didn't want the captive portal
- http://jpmens.net/2010/03/16/sonos-pause-switch/ A simple arduino sonos pause swich that showed me the basics
- https://github.com/DjMomo/sonos/blob/master/sonos.class.php a nice sonos php library that I based my arduino functions on
