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
  this->bookOffset = 1;
  this->bookSize = 10;
  this->initialized = true;
  this->callReady = false;
  this->gpsReady = false;
}

void PHN_Sim::begin() {
  // Probe the SIM module, turn it on if needed
  // The on check already initializes everything
  if (!isOn()) {
    togglePower();

    // Wait until the sim turns on
    while (!sim.isOn());

    // Wait until SIM responds
    while (!sendATCommand("AT"));
  }
}

void PHN_Sim::end() {
  // Turn the SIM off of it is currently on
  if (isOn()) {
    togglePower();
    // Wait until the sim is no longer on
    while (sim.isOn());
  }
}

void PHN_Sim::reset() {
  // Reset to factory defaults
  sendATCommand("AT&F");
  // Turn on extended +CRING information
  sendATCommand("AT+CRC=1");
  // Turn on the incoming call information readout
  sendATCommand("AT+CLIP=1");
  // Automatically receive messages when SMS is received
  sendATCommand("AT+CNMI=3,1");
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

    // Debugging: log all incoming data
    Serial.print("Update input: ");
    Serial.println(inputBuffer);

    // Find any commands in the received message
    char* inputText;
    for (int i = 0; i < inputIndex; i++) {
      inputText = inputBuffer+i;
      if (strstr(inputText, "NO CARRIER") == inputText) {
        // No carrier call status update
        callStatus = SIM_CALL_STATUS_NONE;
        i += 9;
      } else if (strstr(inputText, "BUSY") == inputText) {
        // Busy call status update
        callStatus = SIM_CALL_STATUS_BUSY;
        i += 3;
      } else if (strstr(inputText, "+CLIP: ") == inputText) {
        // Caller information received.
        // +CLIP: "+1234567890",145,"",,"",0
        char* args[1];
        if (getSimTextArgs(inputText+7, args, 1)) {
          // Voice call
          strcpy(incomingNumber, args[0]);
          callStatus = SIM_CALL_STATUS_CALLED;
        }
      } else if (strstr(inputText, "+CMTI: ") == inputText) {
        // +CMTI: "SM",11
        // Text message received
        // Read arguments
        char* args[2];
        if (getSimTextArgs(inputText+7, args, 2)) {
          // Receiving a new text message
          if (!strcmp(args[0], "SM")) {
            latestInbox = atoi(args[1]) - 1;
          }
        }
      } else if (strstr(inputText, "GPS Ready") == inputText) {
        gpsReady = true;
      } else if (strstr(inputText, "Call Ready") == inputText) {
        callReady = true;
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

bool PHN_Sim::enterPuk(const char* puk, const char* newPin) {
  // Build the enter puk+pin command
  char pukPin[50];
  String pukPinStr;
  pukPinStr += puk;
  pukPinStr += ",";
  pukPinStr += newPin;
  pukPinStr.toCharArray(pukPin, sizeof(pukPin));
  return enterPin(pukPin);
}

bool PHN_Sim::enterPin(const char* pin) {
  // Build the enter pin command
  char command[50];
  String commandStr;
  commandStr += "AT+CPIN=";
  commandStr += pin;
  commandStr.toCharArray(command, sizeof(command));

  if (sendATCommand(command) && (getPinStatus() == SIM_PIN_STATUS_READY)) {
    return true;
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

bool PHN_Sim::isSimCardInserted() {
  char resp[20];
  if (!sendATCommand("AT+CSMINS?", resp, sizeof(resp))) {
    return false;
  }
  return resp[2] == '1';
}

void PHN_Sim::setDate(Date newDate) {
  char command[50];
  strcpy(command, "AT+CCLK=");
  writeDate(command+8, newDate);
  sendATCommand(command);
}

Date PHN_Sim::getDate() {
  Date date;
  char resp[40];
  char* args[1];
  if (sendATCommand("AT+CCLK?", resp, 40) && getSimTextArgs(resp, args, 1) == 1) {
    date = readDate(args[0]);
  }
  return date;
}

int PHN_Sim::getRegStatus() {
  char resp[50];
  char *regArgs[2];
  sendATCommand("AT+CREG?", resp, sizeof(resp));
  if (getSimTextArgs(resp, regArgs, 2) == 2) {
    return atoi(regArgs[1]);
  }
  return 0;
}

bool PHN_Sim::isConnected() {
  int status = getRegStatus();
  return (status == 1) || (status == 5);
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
  char resp[30];
  char *args[3];
  if (!sendATCommand("AT+CBC", resp, sizeof(resp))) {
    return 0.0F;
  }
  if (getSimTextArgs(resp, args, 3) != 3) {
    return 0.0;
  }
  return 0.01 * atoi(args[1]);
}

int PHN_Sim::readSignalLevel() {
  char resp[30];
  char *args[2];
  if (!sendATCommand("AT+CSQ", resp, sizeof(resp))) {
    return 0;
  }
  if (getSimTextArgs(resp, args, 2) != 2) {
    return 0;
  }

  uint8_t raw = atoi(args[0]);
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
  
  if (sendATCommand(cmd))
    callStatus = SIM_CALL_STATUS_CALLING;
}

void PHN_Sim::endCall() {
  sendATCommand("ATH");
  callStatus = SIM_CALL_STATUS_NONE;
}

void PHN_Sim::rejectCall() {
  sendATCommand("AT+GSMBUSY");
  callStatus = SIM_CALL_STATUS_NONE;
}

void PHN_Sim::acceptCall() {
  sendATCommand("ATA");
  callStatus = SIM_CALL_STATUS_CALLING;
}

bool PHN_Sim::isCalled() {
  return getCallStatus() == SIM_CALL_STATUS_CALLED;
}

int PHN_Sim::getCallStatus() {
  update();
  return callStatus;
}

const char* PHN_Sim::getIncomingNumber() {
  update();
  return incomingNumber;
}

void PHN_Sim::sendDTMF(char character) {
  if (character) {
    char command[9];
    memcpy(command, "AT+VTS=", 7);
    command[7] = character;
    command[8] = 0;
    sendATCommand(command, 0, 0, SIM_ATCOMMAND_DTFM_TIMEOUT);
  }
}

bool PHN_Sim::setVolume(int level) {
  char command[20];
  memcpy(command, "AT+CLVL=", 8);
  itoa(level, command+8, 10);
  return sendATCommand(command);
}

int PHN_Sim::getMessageCount() {
  // Response: +CPMS: <used_space>,<max_space>
  char resp[50];
  if (!sendATCommand("AT+CPMS=\"SM\"", resp, sizeof(resp))) {
    return 0;
  }
  char* args[2];
  if (getSimTextArgs(resp, args, 2) != 2) {
    return false;
  }
  return atoi(args[0]);
}

int PHN_Sim::getMessageLimit() {
  // Response: +CPMS: <used_space>,<max_space>
  char resp[50];
  if (!sendATCommand("AT+CPMS=\"SM\"", resp, sizeof(resp))) {
    return 0;
  }
  char* args[2];
  if (getSimTextArgs(resp, args, 2) != 2) {
    return false;
  }
  return atoi(args[1]);
}

bool PHN_Sim::hasNewMessage() {
  update();
  return latestInbox != -1;
}

SimMessage PHN_Sim::getNewMessage() {
  int idx = latestInbox;
  latestInbox = -1;
  return getMessage(idx);
}

void PHN_Sim::deleteMessage(int messageIndex) {
  char command[15] = "AT+CMGD=";
  itoa(messageIndex+1, command+8, 10);
  strcat(command, ",0");
  sendATCommand(command, 0, 0);
}

SimMessage PHN_Sim::getMessage(int messageIndex) {
  // Put into text mode
  sendATCommand("AT+CMGF=1");

  // Set up a command to read the message
  char command[13] = "AT+CMGR=";
  char resp[400];
  char *args[5];
  itoa(messageIndex+1, command+8, 10);
  sendATCommand(command, resp, 400);

  // Allocate the message to return
  SimMessage message;

  // Split the response arguments
  // If this fails, message will be left all-0
  message.valid = getSimTextArgs(resp, args, 5) == 5;
  if (message.valid) {    
    // Parse the text arguments
    message.index = messageIndex;
    message.read = !strcmp(args[0], "REC READ");
    strcpy(message.sender.number, args[1]);
    strcpy(message.sender.text, args[2]);
    message.date = readDate(args[3]);

    // If the entire message consists of HEX characters, convert to ASCII
    char* msgText = args[4];
    int msgLen = strlen(msgText);
    bool allHex = (msgLen >= 2);
    for (int i = 0; (i < msgLen) && allHex; i++) {
      allHex = (msgText[i] >= 48) && (msgText[i] <= 70);
    }
    if (allHex) {
      unsigned char data_ctr = 1;
      char *buff = message.text-1;
      for (int i = 0; i < msgLen; i++) {
        // Skip every 2 first bytes
        if (!(i & 0x2)) continue;

        // Read the new byte
        char c = msgText[i];

        // Start of a new byte, increment and set to an initial 0x00
        data_ctr++;
        if (!(data_ctr & 0x1)) {
          *(++buff) = 0x00;
        }

        // Convert the character into HEX, put it into the buffer
        *buff <<= 4;
        if (c & 0x40) {
          c -= ('A' - '0') - 10;
        }
        *buff |= (c - '0') & 0xF;
        
        // For 'weird' unreadable ASCII characters, replace with ???
        if ((data_ctr & 0x1) && ((*buff >= 127) || (*buff <= 8))) {
          *buff = '?';
        }
      }
      
      // Make sure we null-terminate
      *(++buff) = 0;
    } else {
      // Just copy it over
      strcpy(message.text, msgText);
    }
  }
  return message;
}

bool PHN_Sim::sendMessage(const char* receiverAddress, const char* messageText) {
  char command[300];
  int index;

  // Set to text message mode
  if (!sendATCommand("AT+CMGF=1")) {
    return false;
  }

  // Write the message start command containing the receiver address
  // No further response happens, use writeATCommand instead of sendATCommand
  index = 0;
  index += strcpy_count(command+index, "AT+CMGS=");
  command[index++] = '"';
  index += strcpy_count(command+index, receiverAddress);
  command[index++] = '"';
  command[index++] = '\r';
  command[index++] = '\n';
  command[index++] = 0;
  if (!writeATCommand(command)) {
    return false;
  }

  // Wait until the > token is read indicating SIM is ready for the message
  readToken("\r\n> ", 100);

  // Write the text message, with end token. Message is echo'd back by the SIM.
  // Response includes '+CMGS: 70', where 70 is the index of the message
  // We are discarding this response here
  index = 0;
  index += strcpy_count(command+index, messageText);
  command[index++] = 0x1A;
  command[index++] = 0;
  return sendATCommand(command, 0, 0, SIM_ATCOMMAND_SENDTEXT_TIMEOUT);
}

bool PHN_Sim::setContactBook(const char* bookName) {
  // Compose command to switch to a different contact book
  char command[40];
  int bookLen = strlen(bookName);
  memcpy(command, "AT+CPBS=\"", 9);
  memcpy(command+9, bookName, bookLen);
  command[bookLen+9] = '\"';
  command[bookLen+10] = 0;
  if (!sendATCommand(command)) {
    return false;
  }
  
  // Next, request the information about this book (ranges)
  char resp[50];
  if (!sendATCommand("AT+CPBR=?", resp, sizeof(resp))) {
    return false;
  }
  char* args[3];
  if (getSimTextArgs(resp, args, 3) != 3) {
    return false;
  }

  // Verify that the ranges are correct, and parse them
  char* recordRange = args[0];
  char* recordMid = strchr(recordRange, '-');
  if (recordMid == NULL) {
    return false;
  }
  recordRange[strlen(recordRange)-1] = 0;
  recordMid[0] = 0;
  bookOffset = atoi(recordRange+1);
  bookSize = atoi(recordMid+1) - bookOffset + 1;
  bookNameLength = atoi(args[1]);
  bookAddressLength = atoi(args[2]);
  return true;
}

int PHN_Sim::getContactCount() {
  // Response: +CPBS: "<storage>",<used>,<total>
  char resp[50];
  if (!sendATCommand("AT+CPBS?", resp, sizeof(resp))) {
    return 0;
  }
  char* args[3];
  if (getSimTextArgs(resp, args, 3) != 3) {
    return false;
  }
  return atoi(args[1]);
}

SimContact PHN_Sim::getContact(int contactIndex) {
  // Compose and send command to retrieve contact details
  char resp[200];
  char *args[4];
  char command[13] = "AT+CPBR=";
  itoa(contactIndex+this->bookOffset, command+8, 10);

  // Parse details to a contact struct
  SimContact contact;
  contact.valid = sendATCommand(command, resp, 400) && getSimTextArgs(resp, args, 4) == 4;
  if (contact.valid) {
    contact.index = atoi(args[0])-this->bookOffset;
    strcpy(contact.number, args[1]);
    contact.type = atoi(args[2]);
    strcpy(contact.text, args[3]);
  }
  return contact;
}

bool PHN_Sim::addContact(SimContact contact) {
  return setContact(-1, contact);
}

bool PHN_Sim::setContact(int contactIndex, SimContact contact) {
  //AT+CPBW=([index]),"[address]",[type],"[name]"

  // Convert some things to a string array buffer
  char idxPart[6];
  char typePart[6];
  if (contactIndex == -1) {
    idxPart[0] = 0;
  } else {
    itoa(contactIndex+this->bookOffset, idxPart, 10);
  }
  itoa(contact.type, typePart, 10);

  // Generate a list of Strings to concatenate
  const int parts_count = 9;
  const char* parts[parts_count];
  parts[0] = "AT+CPBW=";
  parts[1] = idxPart;
  parts[2] = ",\"";
  parts[3] = contact.number;
  parts[4] = "\",";
  parts[5] = typePart;
  parts[6] = ",\"";
  parts[7] = contact.text;
  parts[8] = "\"";

  // Concatenate into a single command and send
  char command[100];
  command[0] = 0;
  for (int i = 0; i < parts_count; i++) {
    strcat(command, parts[i]);
  }
  return sendATCommand(command);
}

bool PHN_Sim::deleteContact(int contactIndex) {
  // Compose and send command to delete contact details
  char command[13] = "AT+CPBW=";
  itoa(contactIndex+this->bookOffset, command+8, 10);
  return sendATCommand(command);
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
    //Serial.println("Command got no response!");
    return false;
  }

  // Prepare a buffer for storing the last bytes for checking for OK/ERROR status
  const int statusBufferLen = 9;
  char statusBuffer[statusBufferLen+1];
  memset(statusBuffer, 0, sizeof(statusBuffer));

  // Read the response from the command  
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
    //Serial.println("NO RESULTCODE!");
  }
  return ok;
}

unsigned char PHN_Sim::getSimTextArgs(char *text, char **args, unsigned char maxArgs) {
  unsigned char argCount = 0;
  bool isInArg = false;

  // Text format:
  // 12,35,"hello world",12:23:23.55\r\nPayload

  // Go by the characters, claiming arguments
  while (argCount < maxArgs && *text) {
    // Newline mode: everything after this is the payload data
    if (*text == '\n') {
      args[argCount++] = text+1;
      break;
    }

    // Read quoted arguments fully
    if (*text == '"') {
      args[argCount++] = text+1;
      while (*(++text) && (*text != '"'));
      *(text++) = 0;
      isInArg = true;
      continue;
    }

    // Store newly acquired arguments
    if (!isInArg) {
      isInArg = true;
      args[argCount++] = text;
      continue;
    }

    // Waiting until the current argument is finished
    if (*text == ',' || *text == '\r') {
      *text = 0;
      isInArg = false;
    }

    text++;
  }

  return argCount;
}

/*
 * Date parsing logic
 * Format: "14/06/28,20:58:18+00"
 */

Date PHN_Sim::readDate(char *text) {
  // Skip the first "-character if available
  if (*text == '\"') text++;

  // Allocate a temporary copy to write in
  char buff_arr[21];
  char* buff = buff_arr;
  memcpy(buff, text, 20);

  // Read the first 6 arguments
  Date date;
  for (int i = 0; i < 6; i++) {
    buff[2] = 0;
    date[i] = atoi(buff);
    buff += 3;
  }
  return date;
}

void PHN_Sim::writeDate(char* buffer, Date date) {
  // Fill the buffer with the output format
  strcpy(buffer, "\"00/00/00,00:00:00+00\"");

  // Fill in the details
  for (int i = 0; i < 6; i++) {
    char valueText[4];
    itoa(date[i], valueText, 10);
    unsigned char len = strlen(valueText);
    memcpy(buffer+3+i*3-len, valueText, len);
  }
}