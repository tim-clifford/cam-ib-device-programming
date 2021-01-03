// only small brains use other editors
/* vim: set ts=4 sw=4 tw=0 noet :*/
// tab gang

#include "mbed.h"
#include "stdint.h"				//This allow the use of integers of a known width
#include "CircularArray.cpp"	// circular array for storing the data

#define LM75_REG_TEMP  (0x00)	// Temperature Register
#define LM75_REG_CONF  (0x01)	// Configuration Register
#define LM75_ADDR      (0x90)	// LM75 address
#define LM75_REG_TOS   (0x03)	// TOS Register
#define LM75_REG_THYST (0x02)	// THYST Register

#define T_DEFAULT      -273.15
#define F_EPSILON       0.001	// acceptable floating point error
#define N_DATA          60		// number of datapoints to take

// The online compiler doesn't support chrono types
// and other cool stuff,
// but i'm using GCC_ARM 10 and:
//   a) i don't like warning messages
//   b) I'm not going to use deprecated functions
//      just to support a compiler that was outdated
//      in 2008!!
#ifdef __GNUC__
#	if __GNUC__ >= 10
#		define A_DECENT_COMPILER
#	endif
#endif

#ifdef A_DECENT_COMPILER
#	define DATA_RATE    1s		// time between data captures
#else
#	define DATA_RATE    1
#endif

#ifdef A_DECENT_COMPILER
#	define endl "\n"			// can you tell I'm a linux user?
#else
#	define endl "\r\n"
#endif

#ifndef A_DECENT_COMPILER
// Slight difference here between modern GCC and the online compiler
Serial pc(SERIAL_TX, SERIAL_RX);
#define printf pc.printf
#endif

#define TOS             28		// TOS temperature
#define THYST           26		// THYST tempertuare

//#define DEBUG

EventQueue *queue;				// For long ISRs

I2C i2c(I2C_SDA, I2C_SCL);

DigitalOut led1(LED1);			// Green LED
DigitalOut led2(LED2);			// Blue LED
DigitalOut led3(LED3);			// Red LED

InterruptIn lm75_int(D7);		// TOS interrupt pin

Ticker cycle_ticker;
Ticker alarm_ticker;

char data_write[3];
char data_read[3];

// circular arrays make sense here since
// we expect them to loop continuously
// See CircularArray.cpp for the implementation
CircularArray<float> temp_data;

// Bitwise state of the LEDs
CircularArray<char> led_pattern = {
	0x7, 0x0, 0x7, 0x0, 0x7,    // S
	0x0, 0x0, 0x0,              // _
	0x7, 0x7, 0x7, 0x0, 0x7, 0x7, 0x7,
	0x0, 0x7, 0x7, 0x7,         // 0
	0x0, 0x0, 0x0,              // _
	0x7, 0x0, 0x7, 0x0, 0x7,    // S
	0x0, 0x0, 0x0,              // _
	0x0, 0x0, 0x0, 0x0,         // _
};

#ifdef A_DECENT_COMPILER
auto led_pattern_interval = 100ms;
#else
float led_pattern_interval = 0.1;
#endif

void set_led(char mask)
{
	// doing this the way it is in the
	// examples is pretty ugly tbh
	led1 =  mask     & 1;
	led2 = (mask>>1) & 1;
	led3 = (mask>>2) & 1;
}

void setup_registers(float tos, float thyst)
{
	/* Configure the Temperature sensor device STLM75:
	   - Comparator mode
	   - Fault tolerance: 0
	   - Comparator mode means we can keep reading temperature data
	   and use the OS pin as an interrupt to control the alarm
	   - Another approach would be to stop reading temperature data
	   until the rising edge of the OS pin, but I'd rather take more data
	 */
	data_write[0] = LM75_REG_CONF;
	data_write[1] = 0x00;
	int status = i2c.write(LM75_ADDR, data_write, 2, 0);
	if (status != 0) {            // Error
		while (1) {
			led1 = !led1;
#ifdef a_DECENT_COMPILER
			ThisThread::sleep_for(200ms);
#else
			// here's another reason chrono types are way better:
			// ThisThread::sleep_for uses milliseconds while the
			// tickers use seconds!
			ThisThread::sleep_for(200);
#endif
		}
	}

	// Set the TOS register
	data_write[0] = LM75_REG_TOS;
	int16_t i16 = (int16_t) (tos * 256) & 0xFF80;
	data_write[1] = (i16 >> 8) & 0xff;
	data_write[2] = i16 & 0xff;
	i2c.write(LM75_ADDR, data_write, 3, 0);

	// Set the THYST register
	data_write[0] = LM75_REG_THYST;
	i16 = (int16_t) (thyst * 256) & 0xFF80;
	data_write[1] = (i16 >> 8) & 0xff;
	data_write[2] = i16 & 0xff;
	i2c.write(LM75_ADDR, data_write, 3, 0);
}

void add_temp()
{
	// Read temperature register
	data_write[0] = LM75_REG_TEMP;
	i2c.write(LM75_ADDR, data_write, 1, 1);    // no stop
	i2c.read(LM75_ADDR, data_read, 2, 0);

	// Calculate temperature value in Celcius
	int16_t i16 = (data_read[0] << 8) | data_read[1];
	// Read data as twos complement integer so sign is correct
	temp_data.add(i16 / 256.0);
#ifdef DEBUG
	printf("Adding temperature: %f" endl, i16 / 256.0);
#endif
}

void on_cycle_tick()
{
	queue->call(add_temp);
}

void next_alarm_state()
{
	set_led(led_pattern++);
}

void print_all_temp()
{
	// stop writing to the array while we read from it
	cycle_ticker.detach();
	printf("========BEGIN TEMPERATURE DUMP========" endl);
	for (int i = 0; i < N_DATA; i++) {
		// print value only if it is has been set
		// the current position in the circular array is
		// the last taken temperature, so we start from
		// the next one - which is the oldest.
		if (abs(T_DEFAULT - ++temp_data) > F_EPSILON) {
			printf("%.3f" endl, *temp_data);
		}
		// at the end of this printing the circular array
		// has looped back to the original position
	}
	printf("=========END TEMPERATURE DUMP=========" endl);
	cycle_ticker.attach(on_cycle_tick, DATA_RATE);
}

void on_over_temp()
{
	alarm_ticker.attach(next_alarm_state, led_pattern_interval);
	queue->call(print_all_temp);
}

void on_under_temp()
{
	alarm_ticker.detach();
	set_led(0);
}

int main()
{
#ifdef DEBUG
	printf("Program start" endl);
#	ifdef __GNUC__
#		ifdef __GNUC_MINOR__
	printf("GCC_VERSION: %d.%d" endl, __GNUC__, __GNUC_MINOR__);
#		else
	printf("GCC_VERSION: %d.x" endl, __GNUC__);
#		endif
#	endif
#endif

#ifndef A_DECENT_COMPILER
	// This is set by a config file in the mbed-cli setup
	pc.baud(9600);
#endif

	temp_data = CircularArray<float>(N_DATA, T_DEFAULT);

	setup_registers(TOS, THYST);

	queue = mbed_event_queue();

	// The interrupt line is active low so we trigger on a falling edge
	lm75_int.fall(&on_over_temp);

	// Stop the alarm if the temperature returns to normal
	lm75_int.rise(&on_under_temp);

	cycle_ticker.attach(on_cycle_tick, DATA_RATE);

#ifdef DEBUG
	printf("End of main()" endl);
#endif
}
