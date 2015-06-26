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

/** @file
@brief Holds the PHN_TextContainer class used to dynamically access text using various datatypes.
*/

#ifndef _PHN_TEXTCONTAINER_H_
#define _PHN_TEXTCONTAINER_H_

#include <Arduino.h>

/**
@brief Abstraction for a widget that can store and update text

Makes it easier to set the text of any text-based widget to, for example, a number
or an Arduino-type String.
*/
class PHN_TextContainer {
 public:
   /// Sets the new text to display using an Arduino type String
   void setText(String textString);
   /// Sets the new text to display using an integer value
   void setText(int valueText) { setText((long) valueText); }
   /// Sets the new text to display using a long value
   void setText(long valueText);
   /// Sets the new text to display using a float value
   void setText(double valueText);
   /// Sets the new text to display using a null-terminated String
   void setText(const char* text);

   /// Sets the new text to display using a buffer and length
   virtual void setTextRaw(const char* text, int textLen);

   /// Gets the text displayed
   virtual const char* text();

   /// Gets the length of the text displayed
   virtual int textLength();
};

#endif