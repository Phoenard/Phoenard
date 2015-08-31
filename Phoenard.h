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

/**
 * @file
 * @brief Main include file for the Phoenard library
 */

#ifndef Phoenard_h
#define Phoenard_h

// Compilation architecture check to prevent impossible to understand errors
#ifndef __AVR_ATmega2560__
  #error "The Phoenard library only supports the ATMega 2560 CPU architecture"
#endif
 
#if(ARDUINO < 104)
	// These array operators are included in hardware\arduino\avr\cores\arduino\new.h and new.cpp
	// in newer Arduino distributions (1.0.4 and above)
	#include <stdlib.h>
	void * operator new[](size_t size);
	void operator delete[](void * ptr);
	// Normally, these would go in the .cpp file:
	void * operator new[](size_t size) 
	{ 
		return malloc(size); 
	} 
	void operator delete[](void * ptr) 
	{ 
		free(ptr); 
	} 
#endif

#include "PHNSettings.h"
#include "PHNDisplayHardware.h"
#include "PHNDisplay.h"
#include "PHNSim.h"
#include "PHNBlueWiFi.h"
#include "PHNMidi.h"
#include "PHNSDMinimal.h"
#include "PHNSRAM.h"

// This includes <all> the widgets available in the Phoenard library
#include "PHNWidgetAll.h"

#endif