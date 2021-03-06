// -------------------------------------------------
// Name: Mitchell Adam, Ryan Shukla
// ID: 1528592, 1537980
// CMPUT 275, Winter 2018
//
// Final Project
//
// Modified for the RTS Semester Project Spring 2022
//
// -------------------------------------------------

#include <cmath>

#include "targetfreq.h"

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
