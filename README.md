# Smart-Switch-with-Servo

This is a piece of code for a smart on-off flip switch. I use it to turn my dumb bathroom switch into a smart one.

The hardware is based on this project https://www.thingiverse.com/thing:2848069 except for a more powerful MG996R servo. The later required a separate power source.

## HARDWARE

- NODEMCU v1.0 board
- MG996R servo
- 3D printed frame
- 5.5mm power plug, wires and screws

## SOFTWARE

Once powered on, the gadget connects to WiFi and gets current time from pool.ntp.org. Based on the time, it starts an ON-OFF loop for servo with either ON or OFF. The times for turning the switch ON and OFF are specified in project, along with wifi credentials.
