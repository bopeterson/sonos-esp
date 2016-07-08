#include "Arduino.h"
#include "Libtest.h"




Libtest::Libtest(int buttonpin) {
    //do som init stuff
    WiFiSSID[999] = '\0';
    WiFiPSK[999] = '\0';
    tempssid[999] = '\0';
    strcpy(test10,"12345");
    strcpy(htmlstart,"<!doctype html>\r\n<html><head><meta charset='UTF-8'><title>Connect</title></head><body onload=\"");


}

void Libtest::testfunc() {
  Serial.print(htmlstart);
  
}



