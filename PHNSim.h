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
 * @brief Contains PHN_Sim for accessing the SIM module for calling, texting and much more
 */
 
#include <Arduino.h>

#ifndef _PHNSIM_H_
#define _PHNSIM_H_

#include <utility/PHNUtils.h>
#include "PHNDate.h"
#include "PHNCore.h"

#define SIM_BAUDRATE 57600
#define SIM_PWR_DELAY 1500

// AT Command timeout in MS.
#define SIM_ATCOMMAND_TIMEOUT 500
// AT Send Text message timeout in MS
#define SIM_ATCOMMAND_SENDTEXT_TIMEOUT 8000
// AT Send DTFM message timeout in MS
#define SIM_ATCOMMAND_DTFM_TIMEOUT 1500
// Total amount of times an AT Command will be executed
// If set to 1, no retries are performed.
#define SIM_ATCOMMAND_TRYCNT 3

// Pin status constants
#define SIM_PIN_STATUS_READY 0
#define SIM_PIN_STATUS_NEEDPIN 1
#define SIM_PIN_STATUS_NEEDPUK 2
#define SIM_PIN_STATUS_ERROR 3

// Call status constants
#define SIM_CALL_STATUS_NONE 0
#define SIM_CALL_STATUS_CALLING 1
#define SIM_CALL_STATUS_CALLED 2
#define SIM_CALL_STATUS_BUSY 3

/// A single message contact
typedef struct SimContact {
  SimContact() : valid(false), type(129) {}
  
  bool valid;
  int type;
  char text[51];
  char number[21];
  uint8_t index;
  
  operator bool() const { 
    return valid;
  }
} SimContact;

/// A single text message
typedef struct SimMessage {
  SimMessage() : valid(false) {}

  bool valid;
  SimContact sender;
  Date date;
  bool read;
  char text[140*2+1];
  uint8_t index;

  operator bool() const { 
    return valid;
  }
} SimMessage;

/**
 * @brief Simplistic library to make use of the SIM908 (phone) controller
 *
 * Make or receive calls, send or receive text messages and generally operate
 * the SIM908. Some of the basic AT-Commands are implemented with an easy API.
 * You can make use of the AT-Command sending routines to extend the library.
 * This function automatically parses the response into an array of arguments,
 * with retries and basic text parsing taken care of.
 */
class PHN_Sim {
public:
  /// Ensures the SIM logic is initalized
  void init();
  /// Sets up the SIM for first use
  void begin();
  /// Shuts off the SIM
  void end();
  /// Resets all SIM AT parameters to the factory defaults
  void reset();
  /// Receives SIM status codes, call routinely to keep up to date
  void update();
  /// Checks if the SIM is currently turned on
  bool isOn();
  /// Toggles power on or off - for async toggling
  void togglePower();

  // Read SIM general information
  /// Reads the SIM network registration status
  int getRegStatus();
  /// Checks if the SIM has a connection with a network (is registered)
  bool isConnected();
  /// Checks whether a (valid) SIM card is inserted
  bool isSimCardInserted();
  /// Get the current date according to the SIM
  Date getDate();
  /// Sets a new date stored and updated by the SIM
  void setDate(Date newDate);
  /// Reads the SIM provider name
  bool readProvider(char* buffer, int bufferLength);
  /// Reads the battery level ranging 0.0 (empty) to 1.0 (full)
  float readBatteryLevel();
  /// Reads the signal level in dB, is 0 when there is no connection
  int readSignalLevel();

  // Reading/Entering of access codes
  /// Enters SIM Pin code
  bool enterPin(const char* pin);
  /// Enters SIM PUK and new Pin code
  bool enterPuk(const char* puk, const char* newPin);
  /// Gets the pin entering status
  int getPinStatus();

  // Calling
  /// Initiates a call
  void call(const char* address);
  /// Ends the current call, or cancels a call silently
  void endCall();
  /// Rejects an incoming call by stating you are busy
  void rejectCall();
  /// Accepts the call
  void acceptCall();
  /// Checks whether the SIM is being called, used getCallStatus()
  bool isCalled();
  /// Gets the calling status
  int getCallStatus();
  /// Gets the number of the person calling this SIM
  const char* getIncomingNumber();
  /// Sends a DTMF tone number, does nothing when character is NULL
  void sendDTMF(char character);

  // Text messages
  /// Checks whether a new message is available
  bool hasNewMessage();
  /// Reads the message received last
  SimMessage getNewMessage();
  /// Reads a message from the message inbox
  SimMessage getMessage(int messageIndex);
  /// Deletes a message from the inbox
  void deleteMessage(int messageIndex);
  /// Sends a text message
  bool sendMessage(const char* receiverAddress, const char* messageText);

  // Contacts
  /// Switches to a different phone book to access contacts of
  bool setContactBook(const char* bookName);
  /// Gets how many valid contacts are stored in the current good book
  int getContactCount();
  /// Gets how many contacts can possibly be stored in the current book
  int getContactLimit() const { return this->bookSize; }
  /// Reads the contact stored at the index in the current book
  SimContact getContact(int contactIndex);
  /// Adds new contact information to the current book
  bool addContact(SimContact contact);
  /// Writes new contact information at the index in the current book
  bool setContact(int contactIndex, SimContact contact);
  /// Deletes the contact information at the index in the current book
  bool deleteContact(int contactIndex);

  // AT Command handling routines
  // Return value indicates whether it was successful
  /// Sends a command, returns whether it was successful
  bool sendATCommand(const char* command);
  /// Sends a command and receives the response, returns whether it was successful
  bool sendATCommand(const char* command, char* respBuffer, uint16_t respBufferLength);
  /// Sends a command and receives the response, returns whether it was successful
  bool sendATCommand(const char* command, char* respBuffer, uint16_t respBufferLength, long timeout);
  /// Writes the command, handling non-responsiveness and retries, not reading the full response
  bool writeATCommand(const char* command);

  /** @brief Reads the arguments sent in a sim text response message
   * 
   * The command repeat in front has to be removed by the caller
   * The args array has to be pre-allocated with the min. required elements
   * Return value indicates the amount of arguments read
   * Warning: alters the input text argument!
   */
  unsigned char getSimTextArgs(char *text, char **args, unsigned char maxArgs);

  /// Reads a DATE argument from received response (note: alters text buffer)
  Date readDate(char *text);
  /// Writes a DATE argument to an output buffer
  void writeDate(char* buffer, Date date);
 private:
  int latestInbox;
  int callStatus;
  int bookOffset, bookSize;
  int bookNameLength, bookAddressLength;
  char incomingNumber[20];
  bool initialized;
  bool callReady, gpsReady; // Used when switching between call/gps mode

  /// Waits until reading is possible with the default timeout
  bool waitRead();
  /// Reads up until a String token is read
  bool readToken(const char *token, unsigned long timeoutMS);
};

extern PHN_Sim sim;

#endif