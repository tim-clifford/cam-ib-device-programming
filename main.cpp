#include "mbed.h"

DigitalIn button(USER_BUTTON);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

int main()
{
  led2=0;
  while(true)
  {
	led3 = button == 0;
	led2 = !led3;
	ThisThread::sleep_for(20); // 20 ms
  }
}
