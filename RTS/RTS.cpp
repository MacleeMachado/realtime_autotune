// RTS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//    // Microsoft libmm waveio based passthru example
//

#include <iostream>
#include <stdio.h>
#include <wtypes.h>
#define NUM_FRAMES 1000

int main()
{
   HWAVEIN   inStream;
   HWAVEOUT outStream;
   WAVEFORMATEX waveFormat;
   WAVEHDR buffer[4];                             // pingpong buffers

   waveFormat.wFormatTag      = WAVE_FORMAT_PCM;  // PCM audio
   waveFormat.nSamplesPerSec  =           22050;  // really 22050 frames/sec
   waveFormat.nChannels       =               2;  // stereo
   waveFormat.wBitsPerSample  =              16;  // 16bits per sample
   waveFormat.cbSize          =               0;  // no extra data
   waveFormat.nBlockAlign     =
      waveFormat.nChannels * waveFormat.wBitsPerSample / 2;
   waveFormat.nAvgBytesPerSec =
      waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;

   // Event: default security descriptor, Manual Reset, initial non-signaled
   HANDLE event = CreateEvent(NULL, TRUE, FALSE, "waveout event");

   waveInOpen(  &inStream, WAVE_MAPPER, &waveFormat, (unsigned long)event, 
      0, CALLBACK_EVENT);
   waveOutOpen(&outStream, WAVE_MAPPER, &waveFormat, (unsigned long)event, 
      0, CALLBACK_EVENT);

   // initialize the input and output PingPong buffers
   int index;
   for(index = 0; index < 4; index++) {
      buffer[index].dwBufferLength = NUM_FRAMES * waveFormat.nBlockAlign;
      buffer[index].lpData         = 
	 (void *)malloc(NUM_FRAMES * waveFormat.nBlockAlign);

      buffer[index].dwFlags        = 0;
      waveInPrepareHeader(  inStream, &buffer[index], sizeof(WAVEHDR));
   }

   ResetEvent(event);
   for(index= 0; index < 4; index++) // queue all buffers for input
      waveInAddBuffer(inStream, &buffer[index], sizeof(WAVEHDR));
   waveInStart(inStream);
    
   while(!( buffer[1].dwFlags & WHDR_DONE)); // poll(!) for 2 full input buffers

   // move the two full buffers to output
   waveOutWrite(outStream, &buffer[0], sizeof(WAVEHDR));
   waveOutWrite(outStream, &buffer[1], sizeof(WAVEHDR));

   int inIndex = 2, outIndex = 0; // the next input and output to watch
   while(1) {  // poll for completed input and output buffers
      if(buffer[inIndex].dwFlags & WHDR_DONE) { // input buffer complete?
	 waveInAddBuffer(  inStream, &buffer[inIndex],  sizeof(WAVEHDR));
	 inIndex = (inIndex+1)%4;   // next buffer to watch for full
      }

      if(buffer[outIndex].dwFlags & WHDR_DONE) { // output buffer complete?
	 waveOutWrite(    outStream, &buffer[outIndex], sizeof(WAVEHDR));
	 outIndex = (outIndex+1)%4;   // next buffer to watch for empty
      }
   }
}
}

