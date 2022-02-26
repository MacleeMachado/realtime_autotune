// RTS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//    // Microsoft libmm waveio based passthru example
//

#include <iostream>
#include <stdio.h>
#include <wtypes.h>
#include <string>
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>
#include <mmeapi.h>

#pragma comment (lib, "winmm.lib")


#define NUM_FRAMES 1000000

using namespace std;

int main()
{
   HWAVEIN   inStream;
   HWAVEOUT outStream;
   WAVEFORMATEX waveFormat;
   WAVEHDR buffer[1]; 

   MMRESULT res;

   waveFormat.wFormatTag      = WAVE_FORMAT_PCM;  // PCM audio
   waveFormat.nSamplesPerSec  =           22050;  // really 22050 frames/sec
   waveFormat.nChannels       =               2;  // stereo
   waveFormat.wBitsPerSample  =              16;  // 16bits per sample
   waveFormat.cbSize          =               0;  // no extra data
   waveFormat.nBlockAlign     =
      waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
   waveFormat.nAvgBytesPerSec =
      waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;

   string str = "waveout event";
   wstring temp = wstring(str.begin(), str.end());
   LPCWSTR widestring = temp.c_str();

   // Event: default security descriptor, Manual Reset, initial non-signaled
   HANDLE event = CreateEvent(NULL, TRUE, FALSE, widestring);
   //buffer[0].dwBytesRecorded = 0;
   cout << "First " << buffer[0].dwBytesRecorded << endl;

   cout << "waveinopen" << endl;
   res = waveInOpen(  &inStream, WAVE_MAPPER, &waveFormat, (unsigned long)event, 
      0, CALLBACK_EVENT);
   if (res == MMSYSERR_NOERROR) {
       cout << "WAVEIN OPENED WITHOUT ERROR" << endl;
   }
   else {
       if (res == MMSYSERR_ALLOCATED) {
           return -1;
       }
       else if (res == MMSYSERR_BADDEVICEID) {
           return -2;
       }
       else if (res == MMSYSERR_NODRIVER) {
           return -3;
       }
       else if (res == MMSYSERR_NOMEM) {
           return -4;
       }
       else if (res == WAVERR_BADFORMAT) {
           return -5;
       }
       else {
           return -6;
       }
   }

   cout << "waveoutopen" << endl;
   res = waveOutOpen(&outStream, WAVE_MAPPER, &waveFormat, (unsigned long)event, 
      0, CALLBACK_EVENT);
   if (res == MMSYSERR_NOERROR) {
       cout << "WAVEOUT OPENED WITHOUT ERROR" << endl;
   }
   else {
       if (res == MMSYSERR_ALLOCATED) {
           return -1;
       }
       else if (res == MMSYSERR_BADDEVICEID) {
           return -2;
       }
       else if (res == MMSYSERR_NODRIVER) {
           return -3;
       }
       else if (res == MMSYSERR_NOMEM) {
           return -4;
       }
       else if (res == WAVERR_BADFORMAT) {
           return -5;
       }
       else {
           return -6;
       }
   }

   

   // initialize the input and output PingPong buffers
   buffer[0].dwBufferLength = NUM_FRAMES * waveFormat.nBlockAlign;
   buffer[0].lpData = (LPSTR) malloc(NUM_FRAMES * (waveFormat.nBlockAlign));
   buffer[0].dwFlags = 0;

   cout << "WAVE IN PREPARE" << endl;
   res = waveInPrepareHeader(  inStream, &buffer[0], sizeof(WAVEHDR));
   if (res == MMSYSERR_NOERROR) {
       cout << "PREPARED WITHOUT ERROR" << endl;
   }
   else {
       if (res == MMSYSERR_INVALHANDLE) {
           return -7;
       }
       else if (res == MMSYSERR_NODRIVER) {
           return -3;
       }
       else if (res == MMSYSERR_NOMEM) {
           return -4;
       }
       else {
           return -10;
       }
   }
   res = waveOutPrepareHeader(outStream, &buffer[0], sizeof(WAVEHDR));
   if (res == MMSYSERR_NOERROR) {
       cout << "PREPARED WITHOUT ERROR" << endl;
   }
   else {
       if (res == MMSYSERR_INVALHANDLE) {
           return -7;
       }
       else if (res == MMSYSERR_NODRIVER) {
           return -3;
       }
       else if (res == MMSYSERR_NOMEM) {
           return -4;
       }
       else {
           return -10;
       }
   }

   cout << "Flags: " << buffer[0].dwFlags << endl;

   if (buffer[0].dwFlags & WHDR_PREPARED) {
       cout << "PREPARED" << endl;
   }
   
   cout << "reset event" << endl;
   ResetEvent(event);
   waveInAddBuffer(inStream, &buffer[0], sizeof(WAVEHDR));
   waveInStart(inStream);
    
   cout << "while" << endl;
   cout << "Wanted Length: " << buffer[0].dwBufferLength<< endl;
   cout << "First Byte Rec: " << buffer[0].dwBytesRecorded << endl;
   while (!(buffer[0].dwFlags & WHDR_DONE)) {
       cout << "flag: " << buffer[0].dwFlags << endl;
       cout << "Bytes Rec: " << buffer[0].dwBytesRecorded << endl;
   }// poll until buffer is full

   // move the buffer to output
   cout << "WaveOut" << endl;

   res = waveOutWrite(outStream, &buffer[0], sizeof(WAVEHDR));
   if (res == MMSYSERR_NOERROR) {
       cout << "WAVEOUT WITHOUT ERROR" << endl;
   }
   else {
       if (res == MMSYSERR_INVALHANDLE) {
           return -7;
       }
       else if (res == MMSYSERR_NODRIVER) {
           return -3;
       }
       else if (res == MMSYSERR_NOMEM) {
           return -4;
       }
       else if (res == WAVERR_UNPREPARED) {
           return -21;
       }
       else {
           return -10;
       }
   }

   Sleep(300000);

   waveOutUnprepareHeader(outStream, &buffer[0], sizeof(WAVEHDR));
   waveInUnprepareHeader(inStream, &buffer[0], sizeof(WAVEHDR));
   waveInClose(inStream);
   waveOutClose(outStream);


   buffer[0].lpData = NULL;
}


