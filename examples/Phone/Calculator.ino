/* An advanced calculator application with expression solving abilities */

/* Shows a calculator made out of widgets, allowing calculations to be performed */
void showCalculator() {
  header.setNavigation(" Exit ", NULL);
  
  // Buffer size for operating with equations
  const int BUFFER_SIZE = 500;
  
  // Text field with the equation to work with
  PHN_TextBox calcResult;
  calcResult.setBounds(12, 18, 296, 24);
  calcResult.showBackspace(true);
  calcResult.setMaxLength(BUFFER_SIZE);
  display.addWidget(calcResult);

  // Keyboard to enter calculations on
  PHN_Keyboard calcKeys;
  calcKeys.setBounds(12, 52, 296, 140);
  calcKeys.setDimension(6, 4);
  calcKeys.setSpacing(4);
  calcKeys.setKeys("123/EC456x^%789-()00.+==");
  calcKeys.setColor(FOREGROUND, BUTTON_COLOR);
  display.addWidget(calcKeys);

  boolean resultNeedsClearing = false;
  do {
    display.update();
    char clicked = calcKeys.clickedKey();

    // If digit clicked after having pressed =, reset text
    if (clicked && resultNeedsClearing) {
      if (char_is_number(clicked)) calcResult.setText("");
      resultNeedsClearing = false;
    }

    // If the result field is interacted with, don't clear
    if (calcResult.isClicked()) {
      resultNeedsClearing = false;
    }

    // Handle characters entered    
    if (clicked == '=') {
      // Execute equation
      char buff[BUFFER_SIZE];
      strcpy(buff, calcResult.text());
      solveEquation(buff);
      
      // Round off to a double value
      double_to_string(buff, string_to_double(buff));
      
      // Save value
      calcResult.setText(buff);
      resultNeedsClearing = true;
    } else if (clicked == 'C') {
      calcResult.setText("");
    } else if (clicked) {
      calcResult.setSelection(clicked);
    }
  } while (!header.isCancelled());
  
  display.removeWidget(calcResult);
  display.removeWidget(calcKeys);
}

/* 
 * Calculates the result of a mathematical expression recursively
 */
double calculate(const char* expression) {
  int expressionLength = strlen(expression)+1;
  char buffer[expressionLength+30];
  memcpy(buffer, expression, expressionLength);

  // Simplify the expression for a little while and finally parse result
  solveEquation(buffer);
  double result = string_to_double(buffer);

  // Clean up and done!
  return result;
}

/* 
 * Continuously simplifies an equation until a final result is found
 * First all contents withing parentheses are simplified
 * When these are all simplified, the parantheses are removed.
 * During simplification mathematical expressions are executed in order.
 * Powers -> Multiplication/Division/Modulus -> Substraction/Addition.
 * After sufficient repetition all that remains will be the result value.
 * This result value is what will remain in the buffer once completed.
 */
void solveEquation(char* buffer) {
  // First filter the input expression
  filterExpression(buffer);

  // Convert '-2^' expressions into '-1*2^' 
  // This solves a problem where -2^2 = 4 (should be -4)
  // We can not adjust the ^ to see - as subtraction
  // This would cause (-2)^2 -> -2^2 -> -4 (should be 4)
  int len = strlen(buffer);
  int lastNegIdx = -1;
  boolean changesApplied = false;
  for (int i = 0; i < len; i++) {
    if (buffer[i] == '-') {
      lastNegIdx = i;
      continue;
    }
    if (buffer[i] == '^' && (lastNegIdx != -1)) {
      memmove(buffer+lastNegIdx+2, buffer+lastNegIdx, len-lastNegIdx);
      buffer[lastNegIdx+1] = '1';
      buffer[lastNegIdx+2] = '*';
      len += 2;
      i += 2;
      changesApplied = true;
    }
    if (!char_is_number(buffer[i])) {
      lastNegIdx = -1;
    }
  }

  // Make sure to filter one more time, maybe new things popped up?
  if (changesApplied) {
    filterExpression(buffer);
  }

  do {
    changesApplied = false;

    // Attempt to find (nested) ( and ) characters
    len = strlen(buffer);
    int startIndex = -1;
    int endIndex = len;
    boolean foundStartNesting = false;
    boolean foundEndNesting = false;
    for (int i = 0; i < len; i++) {
      char c = buffer[i];
      if (c == '(') {
        startIndex = i;
        foundStartNesting = true;
      } else if (c == ')') {
        endIndex = i;
        foundEndNesting = true;
        break;
      }
    }

    // Proceed to execute expressions
    for (unsigned char mode = 0; mode < 3; mode++) {
      for (int i = (startIndex+2); i < endIndex; i++) {
        char c = buffer[i];
        boolean isMult = false;
        boolean isDiv = false;
        boolean isAdd = false;
        boolean isSub = false;
        boolean isMod = false;
        boolean isPow = false;
        if (mode==0) {
          // Handle powers
          isPow = (c == '^');
        } else if (mode==1) {
          // Handle multiplication, division and modulus
          isDiv = (c == '/');
          isMult = (c == '*') || (c == 'x');
          isMod = (c == '%');
        } else if (mode==2) {
          // Handle addition and subtraction
          isSub = (c == '-') && buffer[i-1] != 'E';
          isAdd = (c == '+');
        }
        if (isMod || isDiv || isMult || isAdd || isSub || isPow) {
          // Find the values on the left and right of the expression
          double a = takeValue(buffer, i, -1);
          double b = takeValue(buffer, i, 1);
          double r = 0.0;
          if (isDiv)   r = a/b;
          if (isMult)  r = a*b;
          if (isSub)   r = a-b;
          if (isAdd)   r = a+b;
          if (isMod)   r = ((long)a)%((long)b);
          if (isPow)   r = pow_double(a, b);

          // Convert the result into a String
          // We assume this number will never be longer than 50
          char result[50];
          double_to_string(result, r);
          int resultLen = strlen(result);

          // Clear space in the buffer and copy over the result
          memmove(buffer+i+resultLen, buffer+i+1, len-i);
          memcpy(buffer+i, result, resultLen);          
          filterExpression(buffer);

          // Adjust buffer end position
          int newLen = strlen(buffer);
          endIndex += (newLen - len);
          len = newLen;
          changesApplied = true;
          i = (startIndex+2);
        }
      }
    }

    // If no changes were applied but we exposed a nesting before, remove the nesting now
    // This allows the equation to be further parsed
    if (!changesApplied && (foundStartNesting || foundEndNesting)) {
      changesApplied = true;
      if (foundStartNesting) buffer[startIndex] = ' ';
      if (foundEndNesting) buffer[endIndex] = ' ';
      filterExpression(buffer);
    }

    // Continue processing until nothing is left to do
  } while (changesApplied);
}

/* Parses and removes a value on the left or right of a position in the buffer */
double takeValue(char* buff, int index, char dir) {
  int endIndex = index;
  boolean hasNumber = false;
  while (true) {
    endIndex += dir;
    char c = (endIndex < 0) ? ' ' : buff[endIndex];
    boolean isNumber = char_is_number(c);

    // Check if the current character is subtraction and not a number
    if ((c == '-' && buff[endIndex-1] != 'E') && hasNumber) {
      isNumber = false;
      if (dir==-1) endIndex--;
    }

    // Wait until a non-number character is read
    if (!isNumber) {
      // Calculate the range of the found number
      int val_start = min(index+dir, endIndex-dir);
      int val_end = (dir==1) ? endIndex : index;
      char val_endchar = buff[val_end];
      double value;

      // Delimit and parse as double
      buff[val_end] = 0;
      value = string_to_double(buff+val_start);
      buff[val_end] = val_endchar;

      // Clear the value range with spaces
      memset(buff+val_start, ' ', val_end - val_start);

      return value;
    }
    
    hasNumber = true;
  }
  return 0.0;
}

/*
 * Filters an expression into a format that can be more easily parsed
 * - Spaces are removed
 * - Empty string changed to '0'
 * - Invalid characters at the start trimmed
 * - ')5' -> ')*5' and '5(' -> '5*('
 */
void filterExpression(char* buffer) {
  char* tmp;

  // Filter spaces and invalid character at the start by data-shifting
  tmp = buffer;
  int offset = 0;
  boolean isStart = true;
  for (;;) {
    char c = tmp[offset];
    if (c == ' ' || (isStart && !char_is_number(c) && c != '(')) {
      offset++;
    } else {
      isStart = false;
      tmp[0] = c;
      if (!*(tmp++)) break;
    }
  }

  // Fix expression to include * for () multiplication
  tmp = buffer--;
  while (*tmp) {
    // Replace )[number] and [number]( to include a * in between
    boolean is_para_a = (tmp[0] == ')' && char_is_number(tmp[1]) && tmp[1]!='-');
    boolean is_para_b = (tmp[1] == '(' && char_is_number(tmp[0]) && tmp[0]!='-');
    if (is_para_a || is_para_b) {
      memmove(tmp+2, tmp+1, strlen(tmp));
      tmp[1] = '*';
    }

    tmp++;
  }

  // If buffer all empty, set to a single 0
  if (!buffer[0]) {
    buffer[0] = '0';
    buffer[1] = 0;
  }

  // You can provide debug logging here
  Serial.print("Equation: ");
  Serial.println(buffer);
}

/* Converts a text String in technical notation to a double value */
double string_to_double(const char* valueText) {
  // If value starts with just a E, return power
  if (valueText[0]=='E') {
    return pow_double(10.0, string_to_double(valueText+1));
  }
  
  // strtod(input, endptr);
  return strtod(valueText, NULL);
}

/* Converts a double value to a text String in technical notation */
void double_to_string(char* outputStr, double value) {
  // First handle special cases to prevent bugs down below
  const char* special = NULL;
  if (value == 0.0) special = "0";
  if (value == INFINITY) special = "inf";
  if (value == -INFINITY) special = "-inf";
  if (value == NAN) special = "nan";
  if (special) {
    strcpy(outputStr, special);
    return;
  }

  // Find base-10 exponent and floating-point decimals
  int exponent = 0;
  boolean negative = (value < 0.0);
  if (negative) value = -value;
  if (value >= 1000000.0) {
    while (value >= 10) { value/=10; exponent++; }
  }
  if (value <= 0.001) {
    while (value < 1.0) { value*=10; exponent--; }
  }
  if (negative) value = -value;

  // Convert double to a String
  dtostrf(value, 6, 6, outputStr);

  // Trim trailing .0's
  int len = strlen(outputStr);
  char *b = outputStr+len;
  boolean foundDot = false;
  while (!foundDot && *(--b)) {
    foundDot = *b == '.';
    if (*b == '0' || foundDot) {
      *b = 0;
      len--;
    } else {
      break;
    }
  }

  // Save exponent
  if (exponent) {
    outputStr[len] = 'E';
    itoa(exponent, outputStr+len+1, 10);
  }
}

/* Raises a value to a certain power */
double pow_double(double a, double b) {
  // Standard pow function has problems with trailing decimals
  // This function takes care of the integer part first to fix this
  double r = 1.0;
  while (r!=0.0 && r!=INFINITY && r!=-INFINITY && r!=NAN) {
    if (b >= 1.0) {
      r *= a;
      b -= 1.0;
    } else if (b <= -1.0) {
      r /= a;
      b += 1.0;
    } else {
      break;
    }
  }
  return r * pow(a, b);
}

/* Checks whether a certain character is part of a valid number */
boolean char_is_number(char c) {
  return (c >= '0' && c <= '9') || c=='-' || c=='.' || c=='E' || c=='i' || c=='n' || c=='f' || c=='a';
}
