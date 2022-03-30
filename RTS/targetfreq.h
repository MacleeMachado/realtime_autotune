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
 * Returns amount of cents needed for autotune correction.
 * @param actualFreq The frequency of the signal (can be estimated using
 *     fundamental())
 */
int getTargetFreq(double actualFreq);

#endif
