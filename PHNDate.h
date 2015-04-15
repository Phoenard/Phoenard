/*
The MIT License (MIT)

This file is part of the Phoenard Arduino library
Copyright (c) 2014 Phoenard

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/**@file
 * @brief Date-related functions
 */

#ifndef _PNN_DATE_H_
#define _PNN_DATE_H_

/// Enumeration for all 12 months of the year
enum Month {JANUARY, FEBRUARY, MARCH, APRIL, MAY, JUNE,
           JULY, AUGUST, SEPTEMBER, OCTOBER, NOVEMBER, DECEMBER};

/// Struct to hold years, months, days, hours, minutes and seconds for a given Date
typedef struct Date {
  uint8_t year, month, day, hour, minute, second;

  Date() : year(0), month(0), day(0), hour(12), minute(0), second(0) {
  }
} Date;

/// Adds a certain amount of seconds to a date
Date addDateSeconds(Date date, long totalSeconds);

#endif