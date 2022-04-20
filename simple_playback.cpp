#pragma comment(lib, "winmm.lib")
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
//#define Samplerate 44100
#define Samplerate 22050

static HWAVEIN hWaveIn;
static HWAVEOUT hWaveOut;

enum { NUM_BUF = 3 };
WAVEHDR _header[NUM_BUF];

static void CALLBACK myWaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    static int _iBuf;
    waveOutWrite(hWaveOut, &_header[_iBuf], sizeof(WAVEHDR));   // play audio
    //cout << "callback" << endl;
    ++_iBuf;
    if (_iBuf == NUM_BUF)   _iBuf = 0;
    waveInAddBuffer(hWaveIn, &_header[_iBuf], sizeof(WAVEHDR));
}

int main(int argc, CHAR* argv[])
{
    WAVEFORMATEX pFormat;
    pFormat.wFormatTag = WAVE_FORMAT_PCM; // simple, uncompressed format
    pFormat.nChannels = 2; // 1=mono, 2=stereo
    pFormat.nSamplesPerSec = Samplerate;
    pFormat.wBitsPerSample = 16; // 16 for high quality, 8 for telephone-grade
    pFormat.nBlockAlign = pFormat.nChannels * pFormat.wBitsPerSample / 8;
    pFormat.nAvgBytesPerSec = (pFormat.nSamplesPerSec) * (pFormat.nChannels) * (pFormat.wBitsPerSample) / 8;
    pFormat.cbSize = 0;


    short int* _pBuf;
    size_t bpbuff = (pFormat.nSamplesPerSec) * (pFormat.nChannels) * (pFormat.wBitsPerSample)/8;
    _pBuf = new short int[bpbuff * NUM_BUF];

    waveInOpen(&hWaveIn, WAVE_MAPPER, &pFormat, (DWORD_PTR)myWaveInProc, 0L, CALLBACK_FUNCTION);
    MMRESULT res =  waveOutOpen(&hWaveOut, WAVE_MAPPER, &pFormat, NULL, 0, CALLBACK_NULL);
    if (res == MMSYSERR_NOERROR) {
        //cout << "WAVEOUT OPENED WITHOUT ERROR" << endl;
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

    // initialize all headers in the queue
    for (int i = 0; i < NUM_BUF; i++)
    {
        _header[i].lpData = (LPSTR)&_pBuf[i * bpbuff];
        _header[i].dwBufferLength = bpbuff * sizeof(*_pBuf);
        _header[i].dwFlags = 0L;
        _header[i].dwLoops = 0L;
        waveInPrepareHeader(hWaveIn, &_header[i], sizeof(WAVEHDR));
    }
    waveInAddBuffer(hWaveIn, &_header[0], sizeof(WAVEHDR));

    waveInStart(hWaveIn);

    getchar();
    waveInClose(hWaveIn);
    waveOutClose(hWaveOut);
    delete _pBuf;

    return 0;
}
