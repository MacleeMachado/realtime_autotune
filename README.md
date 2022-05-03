# realtime_autotune

This project combines Soundstretch, a student made API, and uses Microsoft functions to handle mic input and output to be able to record through the computer microphone, auto tune that input to the closest frequency, and output that recording while the sound is being played continuously. 

The main function, RTS.CPP is where our code is largely located. In this file we open the computer microphone, continously poll input, get the frequency spectrum of that input, send that input to soundstretch to be able change the frequency to the closest matching frequency so that any "flat" input is automatically tuned to be the desired frequency "non flat" then send that output to the speakers while still polling input. 

This continues until a button is pressed on the keyboard.
