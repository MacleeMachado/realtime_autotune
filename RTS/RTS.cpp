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

#include "fourier.h"
#include "fundamental.h"
#include "targetfreq.h"
#include <string>

#include <thread>

#pragma comment (lib, "winmm.lib")

using namespace soundtouch;
using namespace std;

vector<clock_t> cs;
vector<clock_t> ce;

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

enum errors {
    SUCCESS = 0,
    ALLOCATED = -1,
    BADDEVICEID = -2,
    NODRIVER = -3,
    NOMEM = -4,
    BADFORMAT = -5,
    UNKNOWN_WAV_IN_OPEN_ERROR = -6,
    UNKNOWN_WAV_OUT_OPEN_ERROR = -7,
    INVALHANDLE = -8,
    UNKNOWN_WAV_IN_PREPARE_HEADER_ERROR = -9
};

/**
* setup: Sets the 'SoundTouch' object up according to command line parameters
* @param pSoundTouch pointer to the 'SoundTouch' object being setup
* @param params pointer to the command line parameters
*/
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
        // Use settings for speech processing
        pSoundTouch->setSetting(SETTING_SEQUENCE_MS, 40);
        pSoundTouch->setSetting(SETTING_SEEKWINDOW_MS, 15);
        pSoundTouch->setSetting(SETTING_OVERLAP_MS, 8);
        fprintf(stderr, "Tune processing parameters for speech processing.\n");
    }
    fflush(stderr);
}

// Processes the sound buffer
/**
* process: Processes the specified number of samples from the provided sample
* buffer using the 'SoundTouch' object.
* @param pSoundTouch pointer to the 'SoundTouch' object being processed
* @param sampleBuffer pointer to the buffer of audio samples
* @param nSamples the number of samples being processed
*/
static void process(SoundTouch* pSoundTouch, short* sampleBuffer, DWORD nSamples)
{
    int nChannels;
    int buffSizeSamples;

    nChannels = NUM_CHANNELS;
    assert(nChannels > 0);
    buffSizeSamples = BUFF_SIZE / nChannels;

    // Feed the samples into SoundTouch processor
    pSoundTouch->putSamples(sampleBuffer, nSamples);

    // Read ready samples from SoundTouch processor.
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

    // Now the input buffer is processed, yet 'flush' few last samples that are
    // hiding in the SoundTouch's internal processing pipeline.
    pSoundTouch->flush();
    do
    {
        nSamples = pSoundTouch->receiveSamples(sampleBuffer, buffSizeSamples);
    } while (nSamples != 0);
}

/**
* myWaveInProc: A callback function to process and output input sound buffers when they are full.
* @param hwi
* @param uMsg
* @param dwInstance
* @param dwParam1
* @param dwParam2
* All parameters are unused, but are necessary to pattern match the declaration of a CALLBACK function
* for the Windows API.
*/
static void CALLBACK myWaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    // This logic skips over the first callback, which occurs when the callback function is established.
    if (!callbackValid) {
        return;
    }
    
    static int _iBuf;
    // The _iBufTemp allows us to listen for input on the next sound buffer
    // while processing and outputting the sound buffer that triggered 
    // the callback function.
    int _iBufTemp = _iBuf + 1;

    if (_iBufTemp == NUM_BUF)   _iBufTemp = 0;

    // Here a start clock (cs) is added to the vector of start clocks.
    // This is done here because it is right before an input buffer is
    // released to be filled.
    cs.push_back(clock());
    waveInAddBuffer(inStream, &buffer[_iBufTemp], sizeof(WAVEHDR));

    // The 4 here is due to the 2 byte samples across 2 channels.
    int numSamples = buffer[_iBuf].dwBytesRecorded / 4;
    // This pointer conversion is necessary to properly consider 16 bit samples
    // (instead of the default 8 bit samples inferred by the char * data type)
    short* sampleBuffer = (short*)(buffer[_iBuf].lpData);

    // Pitch Detection Logic adapted from Pitcher
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
    // Calculate the target frequency (how much pitch correction needs to occur)
    // The term cent is used here because there are a 100 cents per semitone.
    // Here we are modifying the pitch by cents to reach the nearest semitone.
    double cent_diff = getTargetFreq(fund);

    soundTouch.setPitchSemiTones(cent_diff);

    // Process the filled sound buffer
    process(&soundTouch, sampleBuffer, numSamples);

    // Output the processed sound buffer
    waveOutWrite(outStream, &buffer[_iBuf], sizeof(WAVEHDR));
    // Here an end clock (ce) is added to the vector of end clocks.
    // This is done here because it is right after an input buffer is
    // released to be output.
    ce.push_back(clock());
    
    ++_iBuf;
    if (_iBuf == NUM_BUF)   _iBuf = 0;
}

/**
* This is the main function, where the functions defined above
* are called to provide Real-Time Audio Processing functionality.
*/
int main(const int nParams, const char* const paramStr[])
{
    RunParameters* params;
    WAVEFORMATEX waveFormat;
    MMRESULT res;

    // Used for timing performance
    runSum = 0;
    runCount = 0;

    for (int i = 0; i < NUM_BUF; i++) {
        bufferVector[i].resize(NUM_FRAMES);
    }

    /**********************  SoundStretch SetUp  **********************/
    
    // The "null" arguments are for the input and output files, which
    // we do not use.
    const char* const paramS[] = { "RTS.exe", "null", "null" };

    // Parse command line parameters
    params = new RunParameters(3, paramS);

    // Setup the 'SoundTouch' object for processing the sound
    setup(&soundTouch, params);

    /**********************  Windows SetUp  **********************/

    waveFormat.wFormatTag = WAVE_FORMAT_PCM;  // PCM audio
    waveFormat.nSamplesPerSec = SAMPLE_RATE;  // really 22050 frames/sec
    waveFormat.nChannels = 2;  // stereo
    waveFormat.wBitsPerSample = 16;  // 16bits per sample
    waveFormat.cbSize = 0;  // no extra data
    // Given by usage information in the Windows API documentation
    waveFormat.nBlockAlign =
        waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec =
        waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;

    // Opens the input wave stream, associating it with the myWaveInProc callback function.
    res = waveInOpen(&inStream, WAVE_MAPPER, &waveFormat, (DWORD_PTR)myWaveInProc, 0L, CALLBACK_FUNCTION);
    if (res == MMSYSERR_NOERROR) {
        cout << "WAVEIN OPENED WITHOUT ERROR" << endl;
    }
    else {
        if (res == MMSYSERR_ALLOCATED) {
            return ALLOCATED;
        }
        else if (res == MMSYSERR_BADDEVICEID) {
            return BADDEVICEID;
        }
        else if (res == MMSYSERR_NODRIVER) {
            return NODRIVER;
        }
        else if (res == MMSYSERR_NOMEM) {
            return NOMEM;
        }
        else if (res == WAVERR_BADFORMAT) {
            return BADFORMAT;
        }
        else {
            return UNKNOWN_WAV_IN_OPEN_ERROR;
        }
    }

    // Opens the output wave stream
    res = waveOutOpen(&outStream, WAVE_MAPPER, &waveFormat, NULL, 0, CALLBACK_NULL);
    if (res == MMSYSERR_NOERROR) {
        cout << "WAVEOUT OPENED WITHOUT ERROR" << endl;
    }
    else {
        if (res == MMSYSERR_ALLOCATED) {
            return ALLOCATED;
        }
        else if (res == MMSYSERR_BADDEVICEID) {
            return BADDEVICEID;
        }
        else if (res == MMSYSERR_NODRIVER) {
            return NODRIVER;
        }
        else if (res == MMSYSERR_NOMEM) {
            return NOMEM;
        }
        else if (res == WAVERR_BADFORMAT) {
            return BADFORMAT;
        }
        else {
            return UNKOWN_WAV_OUT_OPEN_ERROR;
        }
    }
    
    short int* _pBuf;
    size_t bpbuff = (waveFormat.nSamplesPerSec) * (waveFormat.nChannels) * (waveFormat.wBitsPerSample) / 8;
    _pBuf = new short int[bpbuff * NUM_BUF];

    // Initialize all headers in the queue
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
                return INVALHANDLE;
            }
            else if (res == MMSYSERR_NODRIVER) {
                return NODRIVER;
            }
            else if (res == MMSYSERR_NOMEM) {
                return NOMEM;
            }
            else {
                return UNKOWN_WAV_IN_PREPARE_HEADER_ERROR;
            }
        }
    }

    // Here a start clock (cs) is added to the vector of start clocks.
    // This is done here because it is right before an input buffer is
    // released to be filled.
    cs.push_back(clock());
    waveInAddBuffer(inStream, &buffer[0], sizeof(WAVEHDR));

    // Now that all the buffers are properly intialized and the first
    // buffer is being filled, the callback function is now valid.
    callbackValid = 1;

    waveInStart(inStream);

    // This keeps the function from exiting, allowing for continuous
    // input and output until the program is killed or the timer
    // expires.
    Sleep(100000);

    // This logic calculates the average processing time across
    // the various runs.
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

    delete params;
    delete _pBuf;

    return SUCCESS;
}
