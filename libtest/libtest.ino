#include "Libtest.h"

Libtest libtest(13);

char test9[9];
char test8[]="1234567";

void setup() {
  //test9="123";
  const char test7[]="123456";
  strcpy(test9,"12345678");
  strcpy(test9,"123");
  Serial.begin(9600);
  Serial.println("starting");
  Serial.println(htmlmid);
  //Serial.println(libtest.tempssid);
  Serial.println(test7);
  Serial.println(test8);
  Serial.print(test9);
  Serial.println("<<<");
  Serial.print(libtest.test10);
  Serial.println("<<<");
  libtest.testfunc();

}

void loop() {


}
