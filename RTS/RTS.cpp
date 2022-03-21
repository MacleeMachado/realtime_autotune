// RTS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//    // Microsoft libmm waveio based passthru example
//

#include <iostream>
#include <stdio.h>
#include <wtypes.h>
#include <string>
#include <Windows.h>
#include <mmsystem.h>
#include <mmeapi.h>
#include <stdexcept>
#include <string.h>
#include <time.h>
#include <io.h>

#include "SoundTouch.h"
#include "RunParameters.h"
#include "WavFile.h"
#include "BPMDetect.h"

#pragma comment (lib, "winmm.lib")

#define NUM_FRAMES 1680 // BUFF_SIZE / 4
#define NUM_CHANNELS 2
#define SAMPLE_RATE 22050
#define NUM_BITS 16
// Processing chunk size (size chosen to be divisible by 2, 4, 6, 8, 10, 12, 14, 16 channels ...)
#define BUFF_SIZE           6720

using namespace soundtouch;
using namespace std;

#if _WIN32
    #include <io.h>
    #include <fcntl.h>

    // Macro for Win32 standard input/output stream support: Sets a file stream into binary mode
    #define SET_STREAM_TO_BIN_MODE(f) (_setmode(_fileno(f), _O_BINARY))
#else
    // Not needed for GNU environment... 
    #define SET_STREAM_TO_BIN_MODE(f) {}
#endif


static void openOutputFile(WavOutFile** outFile, const RunParameters* params)
{

    if (params->outFileName)
    {
        if (strcmp(params->outFileName, "stdout") == 0)
        {
            SET_STREAM_TO_BIN_MODE(stdout);
            *outFile = new WavOutFile(stdout, SAMPLE_RATE, NUM_BITS, NUM_CHANNELS);
        }
        else
        {
            *outFile = new WavOutFile(params->outFileName, SAMPLE_RATE, NUM_BITS, NUM_CHANNELS);
        }
    }
    else
    {
        *outFile = NULL;
    }
}



// Sets the 'SoundTouch' object up according to input file sound format & 
// command line parameters
static void setup(SoundTouch* pSoundTouch, const RunParameters* params)
{

    pSoundTouch->setSampleRate(SAMPLE_RATE);
    pSoundTouch->setChannels(NUM_CHANNELS);

    pSoundTouch->setTempoChange(params->tempoDelta);
    pSoundTouch->setPitchSemiTones(params->pitchDelta);
    pSoundTouch->setRateChange(params->rateDelta);

    pSoundTouch->setSetting(SETTING_USE_QUICKSEEK, params->quick);
    pSoundTouch->setSetting(SETTING_USE_AA_FILTER, !(params->noAntiAlias));

    if (params->speech)
    {
        // use settings for speech processing
        pSoundTouch->setSetting(SETTING_SEQUENCE_MS, 40);
        pSoundTouch->setSetting(SETTING_SEEKWINDOW_MS, 15);
        pSoundTouch->setSetting(SETTING_OVERLAP_MS, 8);
        fprintf(stderr, "Tune processing parameters for speech processing.\n");
    }

    // print processing information
    if (params->outFileName)
    {
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        fprintf(stderr, "Uses 16bit integer sample type in processing.\n\n");
#else
#ifndef SOUNDTOUCH_FLOAT_SAMPLES
#error "Sampletype not defined"
#endif
        fprintf(stderr, "Uses 32bit floating point sample type in processing.\n\n");
#endif
        // print processing information only if outFileName given i.e. some processing will happen
        fprintf(stderr, "Processing the file with the following changes:\n");
        fprintf(stderr, "  tempo change = %+g %%\n", params->tempoDelta);
        fprintf(stderr, "  pitch change = %+g semitones\n", params->pitchDelta);
        fprintf(stderr, "  rate change  = %+g %%\n\n", params->rateDelta);
        fprintf(stderr, "Working...");
    }
    else
    {
        // outFileName not given
        fprintf(stderr, "Warning: output file name missing, won't output anything.\n\n");
    }

    fflush(stderr);
}

// Processes the sound
static void process(SoundTouch* pSoundTouch, LPSTR sampleBuffer, WavOutFile* outFile)
{
    int nSamples;
    int nChannels;
    int buffSizeSamples;

    if (outFile == NULL) return;  // nothing to do.

    nChannels = NUM_CHANNELS;
    assert(nChannels > 0);
    buffSizeSamples = BUFF_SIZE / nChannels;


    // Feed the samples into SoundTouch processor
    pSoundTouch->putSamples(sampleBuffer, nSamples);

    // Read ready samples from SoundTouch processor & write them output file.
    // NOTES:
    // - 'receiveSamples' doesn't necessarily return any samples at all
    //   during some rounds!
    // - On the other hand, during some round 'receiveSamples' may have more
    //   ready samples than would fit into 'sampleBuffer', and for this reason 
    //   the 'receiveSamples' call is iterated for as many times as it
    //   outputs samples.
    do
    {
        nSamples = pSoundTouch->receiveSamples(sampleBuffer, buffSizeSamples);
        outFile->write(sampleBuffer, nSamples * nChannels);
    } while (nSamples != 0);

    // Now the input file is processed, yet 'flush' few last samples that are
    // hiding in the SoundTouch's internal processing pipeline.
    pSoundTouch->flush();
    do
    {
        nSamples = pSoundTouch->receiveSamples(sampleBuffer, buffSizeSamples);
        outFile->write(sampleBuffer, nSamples * nChannels);
    } while (nSamples != 0);
}


int main(const int nParams, const char* const paramStr[])
{
    WavOutFile* outFile;
    RunParameters* params;
    SoundTouch soundTouch;
    HWAVEIN   inStream;
    HWAVEOUT outStream;
    WAVEFORMATEX waveFormat;
    WAVEHDR buffer[1];
    MMRESULT res;

    // SoundStretch SetUp

    fprintf(stderr, _helloText, SoundTouch::getVersionString());

    // Parse command line parameters
    params = new RunParameters(nParams, paramStr);

    // Open output file
    openOutputFile(&outFile, params);

    // Setup the 'SoundTouch' object for processing the sound
    setup(&soundTouch, params);

    // Windows SetUp

    waveFormat.wFormatTag = WAVE_FORMAT_PCM;  // PCM audio
    waveFormat.nSamplesPerSec = SAMPLE_RATE;  // really 22050 frames/sec
    waveFormat.nChannels = 2;  // stereo
    waveFormat.wBitsPerSample = 16;  // 16bits per sample
    waveFormat.cbSize = 0;  // no extra data
    waveFormat.nBlockAlign =
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
    res = waveInOpen(&inStream, WAVE_MAPPER, &waveFormat, (unsigned long)event,
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
    buffer[0].lpData = (LPSTR)malloc(NUM_FRAMES * (waveFormat.nBlockAlign));
    buffer[0].dwFlags = 0;

    cout << "WAVE IN PREPARE" << endl;
    res = waveInPrepareHeader(inStream, &buffer[0], sizeof(WAVEHDR));
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

    // Processing While Loop (!) Need to decide how much read setup must be included.
    while (1) {
        cout << "reset event" << endl;
        ResetEvent(event);
        waveInAddBuffer(inStream, &buffer[0], sizeof(WAVEHDR));
        waveInStart(inStream);

        cout << "while" << endl;
        cout << "Wanted Length: " << buffer[0].dwBufferLength << endl;
        cout << "First Byte Rec: " << buffer[0].dwBytesRecorded << endl;
        while (!(buffer[0].dwFlags & WHDR_DONE)) {
            cout << "flag: " << buffer[0].dwFlags << endl;
            cout << "Bytes Rec: " << buffer[0].dwBytesRecorded << endl;
        }// poll until buffer is full

        // clock_t cs = clock();    // for benchmarking processing duration
        // Process the sound
        process(&soundTouch, buffer[0].lpData, outFile);
        // clock_t ce = clock();    // for benchmarking processing duration
        // printf("duration: %lf\n", (double)(ce-cs)/CLOCKS_PER_SEC);
    }

    // move the buffer to output
    cout << "WaveOut" << endl;
    /* Focusing on writing to output file instead
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
    */

    // Clean Up Work
    waveOutUnprepareHeader(outStream, &buffer[0], sizeof(WAVEHDR));
    waveInUnprepareHeader(inStream, &buffer[0], sizeof(WAVEHDR));
    waveInClose(inStream);
    waveOutClose(outStream);
    // Close WAV file handles & dispose of the objects
    delete outFile;
    delete params;

    buffer[0].lpData = NULL;
}
