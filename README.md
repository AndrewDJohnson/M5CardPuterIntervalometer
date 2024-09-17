// Based on a sketch will send out a Nikon D50 trigger signal (probably works with most Nikons)
// See the full tutorial at https://learn.adafruit.com/ir-sensor/making-an-intervalometer
// Adapted for M5-stack cardputer by A.D. Johnson in Sept 2024
//
// Can now be used as a manual remote control (single shot) or as an Intervalometer, with settings entered on the Cardputer's keyboard!
// Countown and Photo Counter are displayed on the Cardputer's small screen!
// The idea is then that multiple pictures are taken and can be made into a time-lapse video!
// Code has been included to trigger Canon, Nikon, Sony, Olympus, Pentax and Minolta cameras

A forum post I read noted this problem, and a response to the Original Poster suggested a solution was to use an Arduino device and circuit to send an IR pulse to the camera, mimicking the camera’s IR remote, to take photos. The Arduino device mentioned was an ESP32 microcontroller  - and the circuit was shown constructed on a bread board. This seemed rather unwieldy, to say the least. At this point I’d realised that a recent purchase – about which I’ve been meaning the write a blog entry – not only had an ESP32 microcontroller, it also had a built-in IR LED – it was the excellent and versatile M5Cardputer! 

I thought that I should be able to directly use the Arduino Sketch from the circuit example, with only one modification. All I needed to do was change the Pin number of the IR LED. That is, in the M5Cardputer, the IR LED is connected to pin 44 of the ESP32.

I loaded the code into the Arduino IDE and compiled it and downloaded it to the Cardputer… it worked straight away!

The Cardputer is well-suited to the role of IR Camera remote control – being battery powered and pocketable! All that needed to be added was some code to time the repetition of pulses and then offer some options to the user.

Of course, one could view IR remote control as “old hat” now – as newer cameras can be controlled by Apps and either a BlueTooth or WiFi connection, providing easier functionality and, for example, video/image previews on the remote device (i.e. Smart Phone or Tablet). The Carpduter is also equipped with WiFi and Bluetooth, so could, in theory, be used to control a camera with these methods. One such project has been built for a Canon Camera, using an Arduino Board.

Support for Different Makes of Camera - Code was copied from MD_multiCameraIrControl to trigger the popular makes of camera....

