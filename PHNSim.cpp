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

#include "PHNSim.h"
#include<string.h>

// Initialize sim here
PHN_Sim sim;

void PHN_Sim::init() {
  if (this->initialized) {
    return;
  }
  // Initialize the SIM fields
  Serial1.begin(SIM_BAUDRATE);
  pinMode(SIM_PWRKEY_PIN, OUTPUT);
  this->callStatus = SIM_CALL_STATUS_NONE;
  this->latestInbox = -1;
  this->initialized = true;
}

void PHN_Sim::begin() {
  // Probe the SIM module, turn it on if needed
  // The on check already initializes everything
  if (!isOn()) {
    togglePower();

    // Wait until SIM responds
    while (!writeATCommand("AT"));
  }
}

void PHN_Sim::end() {
  // Turn the SIM off of it is currently on
  if (isOn()) {
    togglePower();
    // Wait until the 'power down' message
    readToken("NORMAL POWER DOWN", 1000);
  }
}

bool PHN_Sim::isOn() {
  return digitalRead(SIM_DTRS_PIN);
}

void PHN_Sim::togglePower() {
  init();
  digitalWrite(SIM_PWRKEY_PIN,HIGH);
  delay(SIM_PWR_DELAY);
  digitalWrite(SIM_PWRKEY_PIN,LOW);
}

void PHN_Sim::update() {
  init();
  if (Serial1.available()) {
    const int INPUT_BUFF_LEN = 100;
    char inputBuffer[INPUT_BUFF_LEN+1];
    int inputIndex = 0;
    while (inputIndex < INPUT_BUFF_LEN && waitRead()) {
      inputBuffer[inputIndex++] = Serial1.read();
    }
    inputBuffer[inputIndex] = 0;
    Serial.println(inputBuffer);

    // Find any commands in the received message
    char* inputText;
    for (int i = 0; i < inputIndex; i++) {
      inputText = inputBuffer+i;
      if (strstr(inputText, "NO CARRIER") == inputText) {
        // No carrier call status update
        callStatus = SIM_CALL_STATUS_NOCARRIER;
        i += 9;
      } else if (strstr(inputText, "BUSY") == inputText) {
        // Busy call status update
        callStatus = SIM_CALL_STATUS_BUSY;
        i += 3;
      } else if (strstr(inputText, "+CLIP: ") == inputText) {
        // RING message received. Voice call?
        char* args[1];
        if (getSimTextArgs(inputText+8, args, 1)) {
          // Voice call
          strcpy(incomingNumber, args[0]);
          callStatus = SIM_CALL_STATUS_CALLED;
        }
      } else if (strstr(inputText, "+CMTI: ") == inputText) {
        // Text message received
        // Read arguments
        char* args[2];
        if (getSimTextArgs(inputText+7, args, 2)) {
          // Receiving a new text message
          if (!strcmp(args[0], "SM")) {
            latestInbox = atoi(args[1]);
          }
        }
      }
    }
  }
}

inline bool PHN_Sim::waitRead() {
  return waitAvailable(Serial1, SIM_ATCOMMAND_TIMEOUT);
}

bool PHN_Sim::readToken(const char *token, unsigned long timeoutMS) {
  uint16_t i = 0;
  unsigned long startTime = millis();
  while (token[i]) {
    if ((millis() - startTime) >= timeoutMS) {
      // Timeout
      return false;
    }
    if (Serial1.available() && Serial1.read() != token[i++]) {
      i = 0;
    }
  }
  return true;
}

bool PHN_Sim::enterPin(const char* pin) {
  // Build the enter pin command
  char command[13];
  memcpy(command, "AT+CPIN=", 8);
  strncpy(command+8, pin, 4);
  command[12] = 0;
  
  if (sendATCommand(command)) {
    // Wait for the 'Call Ready' Response
    return readToken("Call Ready", 10000);
  }
  return false;
}

int PHN_Sim::getPinStatus() {
  char resp[30];
  if (sendATCommand("AT+CPIN?", resp, 30)) {
    // Read status
    if (!strcmp(resp, "READY")) {
      return SIM_PIN_STATUS_READY;
    } else if (!strcmp(resp, "SIM PIN")) {
      return SIM_PIN_STATUS_NEEDPIN;
    } else if (!strcmp(resp, "SIM PUK")) {
      return SIM_PIN_STATUS_NEEDPUK;
    }
  }
  return SIM_PIN_STATUS_ERROR;
}

Date PHN_Sim::readDate() {
  Date date;
  char resp[40];
  char* args[1];
  if (sendATCommand("AT+CCLK?", resp, 40) && getSimTextArgs(resp, args, 1) == 1)
    date = readDate(args[0]);
  return date;
}

bool PHN_Sim::readProvider(char* buffer, int bufferLength) {
  char resp[50];
  char *provArgs[3];
  if (sendATCommand("AT+COPS?", resp, 50) && getSimTextArgs(resp, provArgs, 3) == 3) {
    int len = min((int) (strlen(provArgs[2])+1), bufferLength);
    memcpy(buffer, provArgs[2], len * sizeof(char));
    buffer[len] = 0;
    return true;
  } else {
    int len = min(bufferLength-1, 6);
    memcpy(buffer, "No Sim", len);
    buffer[len] = 0;
  }
  return false;
}

float PHN_Sim::readBatteryLevel() {
  char resp[20];
  char *args[3];
  if (sendATCommand("AT+CBC", resp, 20) && getSimTextArgs(resp, args, 3) == 3) {
    return 0.01 * atoi(args[1]);
  }
  return 0.0;
}

int PHN_Sim::readSignalLevel() {
  char resp[20];
  char *args[2];
  uint8_t raw = 99;
  if (sendATCommand("AT+CSQ", resp, 20) && getSimTextArgs(resp, args, 2) == 2) {
    raw = atoi(args[0]);
  }
  if (raw == 99) {
    return 0;
  } else if (raw == 0) {
    return -115;
  } else if (raw == 1) {
    return -111;
  } else if (raw == 31) {
    return -52;
  } else {
    return map(raw, 2, 30, -110, -54);
  }
}

void PHN_Sim::call(const char* address) {
  // Set up the command to execute
  int address_len = strlen(address);
  char cmd[30] = "ATD";
  memcpy(cmd+3, address, min(address_len, 30-5));
  cmd[address_len+3] = ';';
  cmd[address_len+4] = 0;
  
  if (sendATCommand(cmd, 0, 0))
    callStatus = SIM_CALL_STATUS_CALLING;
}

void PHN_Sim::endCall() {
  sendATCommand("ATH", 0, 0);
  callStatus = SIM_CALL_STATUS_NONE;
}

void PHN_Sim::acceptCall() {
  sendATCommand("ATA", 0, 0);
  callStatus = SIM_CALL_STATUS_CALLING;
}

int PHN_Sim::getCallStatus() {
  update();
  return callStatus;
}

String PHN_Sim::getIncomingNumber() {
  return String(incomingNumber);
}

bool PHN_Sim::hasNewMessage() {
  update();
  return latestInbox != -1;
}

SimMessage PHN_Sim::readNewMessage() {
  SimMessage message = readMessage(latestInbox);
  latestInbox = -1;
  return message;
}

void PHN_Sim::deleteMessage(uint8_t messageIndex) {
  char command[15] = "AT+CMGD=";
  itoa(messageIndex, command+8, 10);
  int offset = (messageIndex < 10) ? 1 : 2;
  command[8+offset] = ',';
  command[8+offset+1] = '0';
  command[8+offset+2] = 0;
  sendATCommand(command, 0, 0);
}

SimMessage PHN_Sim::readMessage(uint8_t messageIndex) {
  // Put into text mode
  sendATCommand("AT+CMGF=1", 0, 0);

  // Set up a command to read the message
  char command[13] = "AT+CMGR=";
  char resp[400];
  char *args[5];
  itoa(messageIndex, command+8, 10);
  sendATCommand(command, resp, 400);

  // Allocate the message to return
  SimMessage message;
  
  // Split the response arguments
  // If this fails, message will be left all-0
  message.valid = getSimTextArgs(resp, args, 5) >= 5;
  if (message.valid) {
    // Parse the text arguments
    message.index = messageIndex;
    message.read = !strcmp(args[0], "REC READ");
    strcpy(message.sender.address, args[1]);
    strcpy(message.sender.name, args[2]);
    strcpy(message.text, args[4]);
    message.date = readDate(args[3]);
  }
  return message;
}

SimContact PHN_Sim::readContact(uint8_t contactIndex) {
  char command[13] = "AT+CPBR=";
  char resp[200];
  char *args[4];
  itoa(contactIndex, command+8, 10);
  sendATCommand(command, resp, 400);
  
  SimContact contact;
  contact.valid = getSimTextArgs(resp, args, 4) >= 4;
  if (contact.valid) {
    contact.index = atoi(args[0]);
    strcpy(contact.address, args[1]);
    strcpy(contact.name, args[3]);
  }
  return contact;
}

bool PHN_Sim::sendMessage(char* receiverAddress, char* messageText) {
  char command[200];
  int index;

  // Set to text message mode
  writeATCommand("AT+CMGF=1");

  // Write the message start command containing the dest. address
  index = 0;
  index += strcpy_count(command+index, "AT+CMGS=");
  command[index++] = '"';
  index += strcpy_count(command+index, receiverAddress);
  command[index++] = '"';
  command[index++] = '\r';
  command[index++] = '\n';
  command[index++] = 0;
  writeATCommand(command);

  // Write the text message, with end token
  index = 0;
  index += strcpy_count(command+index, messageText);
  command[index++] = 0x1A;
  command[index++] = 0;
  return sendATCommand(command, 0, 0, SIM_ATCOMMAND_SENDTEXT_TIMEOUT);
}

bool PHN_Sim::writeATCommand(const char* command) {
  // Before executing anything, flush the serial with an update
  update();
  
  // Execute the command, retry as needed
  uint8_t retryIdx, readIdx;
  for (retryIdx = 0; retryIdx < SIM_ATCOMMAND_TRYCNT; retryIdx++) {
    // Flush the incoming data
    flushRead(Serial1);

    // Write the command, wait for a response
    Serial1.println(command);
    if (!waitRead()) {
      continue;
    }
  
    // Try reading back the echo from the SIM, and validate
    readIdx = 0;
    while (command[readIdx] && waitRead()) {
      if (Serial1.read() != command[readIdx++]) {
        readIdx = 0;
      }
    }
    if (command[readIdx]) {
      continue;
    }
  
    // Read the two newline characters (\r\n) as well
    if (!waitRead() || Serial1.read() != '\r')
      continue;
    if (!waitRead() || Serial1.read() != '\n')
      continue;
  
    // Success!
    return true;
  }
  return false;
}

bool PHN_Sim::sendATCommand(const char* command) {
  return sendATCommand(command, 0, 0);
}

bool PHN_Sim::sendATCommand(const char* command, char* respBuffer, uint16_t respBufferLength) {
  return sendATCommand(command, respBuffer, respBufferLength, SIM_ATCOMMAND_TIMEOUT);
}

bool PHN_Sim::sendATCommand(const char* command, char* respBuffer, uint16_t respBufferLength, long timeout) {
  if (!writeATCommand(command)) {
    // Failure to communicate the command
    Serial.println("Command got no response!");
    return false;
  }

  // Read the response from the command
  const int statusBufferLen = 9;
  char statusBuffer[statusBufferLen+1];
  uint16_t length = 0;
  bool ok = false, error = false;
  while (waitAvailable(Serial1, timeout)) {
    // Shift data into the status buffer
    memmove(statusBuffer, statusBuffer+1, statusBufferLen-1);
    statusBuffer[statusBufferLen-1] = Serial1.read();

    // Shift data into the response buffer if possible
    if (length+1 < respBufferLength) {
      respBuffer[length] = statusBuffer[statusBufferLen-1];
    }
    length++;

    // Check whether an OK was received
    if (!strncmp(statusBuffer+statusBufferLen-6,"\r\nOK\r\n",6)) {
      length -= 6;
      ok = true;
      break;
    }
    // Check whether an ERROR was received
    if (!strncmp(statusBuffer+statusBufferLen-9,"\r\nERROR\r\n",9)) {
      length -= 9;
      error = true;
      break;
    }
  }

  // Handle post-reading response buffer operations
  if (respBufferLength) {
    // Limit the length by buffer size
    length = min(respBufferLength-1, length);
    // Trim starting \r\n from the response
    if (length >= 2 && !strncmp(respBuffer, "\r\n", 2)) {
      length -= 2;
      memmove(respBuffer, respBuffer+2, length);
    }
    // Trim everything before the first space from the response
    uint16_t i;
    for (i = 1; i < length; i++) {
      if (respBuffer[i-1] == ' ') {
        length -= i;
        memmove(respBuffer, respBuffer+i, length);
        break;
      }
    }
    // Trim ending \r\n from the response
    if (length >= 2 && !strncmp(respBuffer+length-2, "\r\n", 2)) {
      length -= 2;
    }
    
    // Delimit end of String with a NULL character
    respBuffer[length] = 0;
  }

  // Debug
  if (!error && !ok) {
    Serial.println("NO RESULTCODE!");
  }
  return ok;
}

int PHN_Sim::getSimTextArgs(char *text, char **args, int argCount) {
  int maxArgs = argCount;
  uint16_t textIndex = 0;
  argCount = 0;

  // Text format:
  // 12,35,"hello world",12:23:23.55\r\nPayload

  // Go by the characters, claiming arguments
  while (argCount < maxArgs && text[textIndex]) {
    // Find the start of the current argument
    if (text[textIndex] == '\n') {
      // Newline mode: everything after this is the payload data
      args[argCount++] = text+textIndex+1;
      // Exit the entire loop: we are done reading
      break;
    }
    if (text[textIndex] == '"') {
      args[argCount++] = text+(++textIndex);
      // Read argument up till next '"'
      do {
        if (text[textIndex] == '"') {
          text[textIndex++] = 0;
          break;
        }
      } while (text[++textIndex]);
    } else {
      args[argCount++] = text+(textIndex);
    }
    // Move to the end of the current argument (after ',' or '\r')
    do {
      if (text[textIndex] == ',' || text[textIndex] == '\r') {
        text[textIndex++] = 0;
        break;
      }
    } while (text[++textIndex]);
  }
  return argCount;
}

Date PHN_Sim::readDate(char *text) {
  Date date;

  // Parse the date (14/03/07 and 14:37:59+04)
  char *dateArgs[2];
  if (getSimTextArgs(text, dateArgs, 2) == 2) {
    // Yr/Mt/Dy - null terminate parts and parse
    dateArgs[0][2] = 0; dateArgs[0][5] = 0; dateArgs[0][8] = 0;
    date.year = atoi(dateArgs[0]);
    date.month = atoi(dateArgs[0]+3);
    date.day = atoi(dateArgs[0]+6);
    // HH:MM:SS+TZ - null terminate parts and parse
    dateArgs[1][2] = 0; dateArgs[1][5] = 0; dateArgs[1][8] = 0;
    date.hour = atoi(dateArgs[1]);
    date.minute = atoi(dateArgs[1]+3);
    date.second = atoi(dateArgs[1]+6);
  }
  return date;
}