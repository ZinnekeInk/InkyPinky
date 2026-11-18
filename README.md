Inky Pinky

Inky Pinky turns a laser engraver into a pen plotter, without touching the engraver’s firmware.

A 3D-printed head replaces the laser module and plugs into the same laser cable. Inside, an ESP32 listens to the laser’s power signal and drives a small servo that lifts and drops the pen. Laser on → pen down, laser off → pen up. Any laser software that can send a job (LaserGRBL, LightBurn, plain G-code) can now draw with a pen.

The head is powered from the laser cable itself, so there are no extra cables. The ESP32 creates its own WiFi hotspot with a small web page to set the pen’s up and down angles, test the pen, and update the firmware from your phone.

Built for the Two Trees TTS-55. Other engravers with a 3-pin laser connector (12 V, GND, PWM) should work with an adapted mount.

Included: Arduino firmware (ESP32), wiring diagram, parts list, printable STL files, and print and setup instructions.

Made by one tinkerer in spare evenings. If it helps you, a coffee is always welcome.
