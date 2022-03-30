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
 * Calculates nearest frequency that corresponds to a note in the key
 * @param actualFreq The frequency of the signal (can be estimated using
 *     fundamental())
 * @param key The desired key in which to search for the closest acceptable note
 */
double getTargetFreq(double actualFreq, int key);

#endif
