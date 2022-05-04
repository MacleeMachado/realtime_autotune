// -------------------------------------------------
// Name: Mitchell Adam, Ryan Shukla
// ID: 1528592, 1537980
// CMPUT 275, Winter 2018
//
// Final Project
// -------------------------------------------------

#include <cmath>

#include "targetfreq.h"
/*
* getTargetFreq takes in the actual frequency of the sound
* sample in Hz and converts this frequency into a number
* of semitones away from C0. This value is then rounded,
* with the difference between the rounded and actual
* values being returned in order to perform pitch correction.
*/
double getTargetFreq(double actualFreq) {
  // Frequency in Hz of A4
  double A4 = 440;
  // Frequency in Hz of C0
  double C0 = A4 * pow(2, -4.75);

  // Calculate semitones away from C0 using a logarithmic scale
  double semitonesFromC0 = 12 * log2(actualFreq / C0);
  // Round here
  double semitonesRounded = std::round(semitonesFromC0);
  // Take difference
  double difference = semitonesRounded - semitonesFromC0;
  return difference;
}
