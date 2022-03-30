// -------------------------------------------------
// Name: Mitchell Adam, Ryan Shukla
// ID: 1528592, 1537980
// CMPUT 275, Winter 2018
//
// Final Project
// -------------------------------------------------

#include "processbuffer.h"
//#include "processstft.h"
//#include "filter.h"
#include "targetfreq.h"

  int k, chan;

  // Construct a valarray of complex numbers from the input buffer
  CVector bufferVector;
  int sampleRate = 44100;
  bufferVector.resize(inputBufferLen);

  for (chan = 0; chan < channels; chan++) {
    for (k = chan; k < inputBufferLen; k += channels) {
      // Convert each value in the buffer into a complex number
      bufferVector[k] = CNum(inputBuffer[k], 0);
    }
  }

    // Calculate the fundamental frequency
    double fund = fundamental(bufferVector, sampleRate);
    // Calculate the target freqency and scale factor
    double target = getTargetFreq(fund);
