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
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <vector>

#include "SoundTouch.h"
#include "RunParameters.h"
#include "WavFile.h"
#include "BPMDetect.h"

//#include "processbuffer.h"
#include "fourier.h"
#include "fundamental.h"
#include "targetfreq.h"
//#include "targetfreq.h"
#include <string>

#include <thread>

#pragma comment (lib, "winmm.lib")

using namespace soundtouch;
using namespace std;

vector<clock_t> cs;
vector<clock_t> ce;

//67200 and 268800

#define NUM_FRAMES 1680 // BUFF_SIZE / 4
#define NUM_CHANNELS 2
#define SAMPLE_RATE 22050
#define NUM_BITS 16
// Processing chunk size (size chosen to be divisible by 2, 4, 6, 8, 10, 12, 14, 16 channels ...)
#define BUFF_SIZE           6720




#define SET_STREAM_TO_BIN_MODE(f) (_setmode(_fileno(f), _O_BINARY))

#define NUM_BUF 4

static HWAVEIN inStream;
static HWAVEOUT outStream;
static CVector bufferVector[NUM_BUF];
static SoundTouch soundTouch;
static int callbackValid;

double runSum;
int runCount;

WAVEHDR buffer[NUM_BUF];

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
    fflush(stderr);
}

// Processes the sound
static void process(SoundTouch* pSoundTouch, short* sampleBuffer, DWORD nSamples)
{
    int nChannels;
    int buffSizeSamples;

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
    } while (nSamples != 0);

    // Now the input file is processed, yet 'flush' few last samples that are
    // hiding in the SoundTouch's internal processing pipeline.
    pSoundTouch->flush();
    do
    {
        nSamples = pSoundTouch->receiveSamples(sampleBuffer, buffSizeSamples);
    } while (nSamples != 0);
}

static void CALLBACK myWaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (!callbackValid) {
        return;
    }
    
    static int _iBuf;
    int _iBufTemp = _iBuf + 1;

    if (_iBufTemp == NUM_BUF)   _iBufTemp = 0;

    
    cs.push_back(clock());
    waveInAddBuffer(inStream, &buffer[_iBufTemp], sizeof(WAVEHDR));

    int numSamples = buffer[_iBuf].dwBytesRecorded / 4;
    short* sampleBuffer = (short*)(buffer[_iBuf].lpData);

    // Pitch Detection from Pitcher
    int k, chan;

    // Construct a valarray of complex numbers from the input buffer
    bufferVector[_iBuf].resize(numSamples);

    for (chan = 0; chan < NUM_CHANNELS; chan++) {
        for (k = chan; k < numSamples; k += NUM_CHANNELS) {
            // Convert each value in the buffer into a complex number
            bufferVector[_iBuf][k] = CNum(sampleBuffer[k], 0);
        }
    }

    // Calculate the fundamental frequency
    double fund = fundamental(bufferVector[_iBuf], SAMPLE_RATE);
    // Calculate the target freqency and scale factor
    double cent_diff = getTargetFreq(fund);
    //cout << "****" << endl;
    //cout << cent_diff << endl;

    soundTouch.setPitchSemiTones(cent_diff);


    //clock_t cs = clock();    // for benchmarking processing duration
    // Process the sound
    process(&soundTouch, sampleBuffer, numSamples);
    //clock_t ce = clock();    // for benchmarking processing duration
    // printf("duration: %lf\n", (double)(ce-cs)/CLOCKS_PER_SEC);

    waveOutWrite(outStream, &buffer[_iBuf], sizeof(WAVEHDR));   // play audio
    ce.push_back(clock());
    //cout << "callback" << endl;
    ++_iBuf;
    if (_iBuf == NUM_BUF)   _iBuf = 0;
}

int main(const int nParams, const char* const paramStr[])
{
    RunParameters* params;
    WAVEFORMATEX waveFormat;
    MMRESULT res;

    runSum = 0;
    runCount = 0;

    for (int i = 0; i < NUM_BUF; i++) {
        bufferVector[i].resize(NUM_FRAMES);
    }

    const char* const paramS[] = { "RTS.exe", "null", "null" };

    // SoundStretch SetUp

    // Parse command line parameters
    params = new RunParameters(3, paramS);

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

    res = waveInOpen(&inStream, WAVE_MAPPER, &waveFormat, (DWORD_PTR)myWaveInProc, 0L, CALLBACK_FUNCTION);
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

    res = waveOutOpen(&outStream, WAVE_MAPPER, &waveFormat, NULL, 0, CALLBACK_NULL);
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
    
    short int* _pBuf;
    size_t bpbuff = (waveFormat.nSamplesPerSec) * (waveFormat.nChannels) * (waveFormat.wBitsPerSample) / 8;
    _pBuf = new short int[bpbuff * NUM_BUF];


    // initialize all headers in the queue
    for (int i = 0; i < NUM_BUF; i++)
    {
        buffer[i].lpData = (LPSTR)&_pBuf[i * bpbuff];
        buffer[i].dwBufferLength = bpbuff * sizeof(*_pBuf);
        buffer[i].dwFlags = 0L;
        buffer[i].dwLoops = 0L;
        res = waveInPrepareHeader(inStream, &buffer[i], sizeof(WAVEHDR));
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
    }

    cs.push_back(clock());
    waveInAddBuffer(inStream, &buffer[0], sizeof(WAVEHDR));

    callbackValid = 1;

    waveInStart(inStream);

    //getchar();
    Sleep(100000);


    //cout << "ce nums: " << ce[0] << cs[0] << endl;
    for (int i = 0; i < ce.size(); i++) {
        runSum += (double)(ce[i] - cs[i]) / (double)CLOCKS_PER_SEC;
    }
    double runAvg = runSum / ce.size();
    cout << "runAvg: " << runAvg << " secs" << endl;



    // Clean Up Work
    for (int i = 0; i < NUM_BUF; i++) {
        waveOutUnprepareHeader(outStream, &buffer[i], sizeof(WAVEHDR));
        waveInUnprepareHeader(inStream, &buffer[i], sizeof(WAVEHDR));
        buffer[i].lpData = NULL;
    }
    waveInClose(inStream);
    waveOutClose(outStream);
    // Close WAV file handles & dispose of the objects

    delete params;
    delete _pBuf;

    return 0;
}
