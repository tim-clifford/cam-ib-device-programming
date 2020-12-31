
#include "mbed.h"

// Green LED
DigitalOut led1(LED1);
// Blue LED
DigitalOut led2(LED2);
// Red LED
DigitalOut led3(LED3);

void select_led(int l)
{
	if (l == -1)
		led1 = led2 = led3 = 1;
	else if (l == 0)
		led1 = led2 = led3 = 0;
	else if (l <= 3) {
		led1 = l == 1;
		led2 = l == 2;
		led3 = l == 3;
	}
}

int main()
{
	int t = 0;
	while (true) {
		select_led(t);
		ThisThread::sleep_for(500);
		t = (t + 2) % 5 - 1;
	}
}

