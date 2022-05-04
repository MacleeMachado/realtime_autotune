// -------------------------------------------------
// Name: Mitchell Adam, Ryan Shukla
// ID: 1528592, 1537980
// CMPUT 275, Winter 2018
//
// Final Project
// -------------------------------------------------

#ifndef _TARGET_FREQ_H_
#define _TARGET_FREQ_H_

/**
* getTargetFreq takes in the actual frequency of the sound
* sample in Hz and converts this frequency into a number
* of semitones away from C0. This value is then rounded,
* with the difference between the rounded and actual
* values being returned in order to perform pitch correction.
*
* @param actualFreq The frequency of the signal (can be estimated
*      fundamental())
*/
double getTargetFreq(double actualFreq);

#endif
