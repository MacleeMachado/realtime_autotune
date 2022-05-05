# Realtime_Autotune

This project combines Soundstretch, a student made API (Pitcher), and uses Microsoft functions to handle mic input and output to be able to record through the computer microphone, auto tune that input to the closest frequency, and output that recording while the sound is being played continuously. 

The main function, RTS.CPP is where our code is largely located. In this file we open the computer microphone, continously poll input, get the frequency spectrum of that input, send that input to soundstretch to be able change the frequency to the closest matching frequency so that any "flat" input is automatically tuned to be the desired frequency "non flat" then send that output to the speakers while still polling input. 

This continues until a button is pressed on the keyboard.

# Install
To install this program, simpy clone this repository to your computer.

# How-To-Run
The easiest way to run this is on windows, using Visual Studio 2020.
If you are using Visual Studio 2020 you can simply pull this project into visual studio. 

Regardless of what you do to run the code set your speaker and microphone settings by going into their advanced setting to input and output at 24 bits, 48000 Hz. Be sure that you are also using 2 channel on the speakers, in the same settings.

Make sure to use a headset as if you do not there will be a feedback loop created from when the program outputs to what it inputs. Start the program by running the RTS.cpp file in whatever program you want (in visual studio just hit the green button) otherwise make sure all the files are linked appropriately, no command line arguments are needed. Once you start the program wait a second for it to load then continually play or put in whatever input you desired to be tuned and it will be output to the headphones you are using.

There is a slight delay between input and output, roughly 2.5 seconds on average but you will be able to input while hearing the output.
