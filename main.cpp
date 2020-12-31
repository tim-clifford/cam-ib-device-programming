#include "mbed.h"

//Serial pc(SERIAL_TX, SERIAL_RX);

// Green LED
DigitalOut led1(LED1);
// Blue LED
DigitalOut led2(LED2);
// Red LED
DigitalOut led3(LED3);

void select_led(int l)
{
	if (l == 1) {
		led1 = true;
		led2 = false;
		led3 = false;
	} else if (l == 2) {
		led1 = false;
		led2 = true;
		led3 = false;
	} else if (l == 3) {
		led1 = false;
		led2 = false;
		led3 = true;
	}
}

int main()
{
	//pc.baud(9600);
	int t = 1;

	printf("Start!\r\n");

	while (true) {
		select_led(t);
		printf("LED %d is ON.\r\n", t);
		ThisThread::sleep_for(1000*0.5);
		t = (t % 3) + 1;
	}
}
