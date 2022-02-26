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
   WAVEHDR buffer;                     

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
      buffer.dwBufferLength = NUM_FRAMES * waveFormat.nBlockAlign;
      buffer.lpData         = 
	 (void *)malloc(NUM_FRAMES * waveFormat.nBlockAlign);

      buffer.dwFlags        = 0;
      waveInPrepareHeader(  inStream, &buffer, sizeof(WAVEHDR));

   ResetEvent(event);
   waveInAddBuffer(inStream, &buffer, sizeof(WAVEHDR));
   waveInStart(inStream);
    
   while(!( buffer.dwFlags & WHDR_DONE)); // poll until buffer is full

   // move the buffer to output
   waveOutWrite(outStream, &buffer, sizeof(WAVEHDR));
}


