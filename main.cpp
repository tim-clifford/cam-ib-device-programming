#include "mbed.h"

DigitalOut myled(LED1);

int main() {
        while(true) {
                myled = 1; // LED is ON
                ThisThread::sleep_for(1000*0.2); // 200 ms
                myled = 0; // LED is OFF
                ThisThread::sleep_for(1000*1.0); // 1 sec
        }
}
