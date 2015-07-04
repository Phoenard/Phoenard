/* This structure is used to pass along test results */
#ifndef _TESTRESULT_H_
#define _TESTRESULT_H_

typedef struct TestResult {
  boolean success;
  char device[20];
  char status[50];
  
  TestResult() {
    success = false;
  }
  TestResult(const TestResult& result) {
    this->success = result.success;
    memcpy(this->status, result.status, sizeof(status));
    memcpy(this->device, result.device, sizeof(device));
  }
  TestResult(boolean success, const char* status) {
    this->success = success;
    memcpy(this->status, status, strlen(status) + 1);
  }
  TestResult(boolean success, String status) {
    this->success = success;
    status.toCharArray(this->status, sizeof(this->status));
  }
} TestResult;

#endif
