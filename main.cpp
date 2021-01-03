// only small brains use other editors
/* vim: set ts=4 sw=4 tw=0 noet :*/
// tab gang

#include "mbed.h"

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
#	define endl "\n"			// can you tell I'm a linux user?
#else
#	define endl "\r\n"
#endif

#ifndef A_DECENT_COMPILER
// Slight difference here between modern GCC and the online compiler
Serial pc(SERIAL_TX, SERIAL_RX);
#define printf pc.printf
#endif

#define DEBUG

DigitalOut led1(LED1);			// Green LED
DigitalOut led2(LED2);			// Blue LED
DigitalOut led3(LED3);			// Red LED
InterruptIn button(USER_BUTTON);

EventQueue *queue;				// For long ISRs

Timeout button_debounce_timeout;
#ifdef A_DECENT_COMPILER
auto debounce_time_interval = 300ms;
#else
auto debounce_time_interval = 0.3;
#endif

Ticker cycle_ticker;
#ifdef A_DECENT_COMPILER
auto cycle_time_interval = 1s;
#else
auto cycle_time_interval = 1;
#endif

int led_show_index;

// could easily change this to other combinations by just changing the bitmasks!
int led_values_n = 3;
char led_values[3] = { 0b001, 0b010, 0b100 };

// Basic linked list for the user sequence
typedef struct node_struct {
	int value;
	struct node_struct *next;
} node;

node *first_sequence_node;
node *current_sequence_node;

// I am going to interpret a "double click" as two clicks without
// any change of LED color

void onButtonStopDebouncing();
void onButtonPressISR();
void onButtonPress(int led_show_index_i);
void onCycleTick();
void onCycleTickUser();
void start_user_sequence();
void set_led(char mask);
void next_led_sequence();
int main();

void set_led(char mask)
{
	// doing this the way it is in the
	// examples is pretty ugly tbh
	led1 = mask & 1;
	led2 = (mask >> 1) & 1;
	led3 = (mask >> 2) & 1;
}

void onButtonStopDebouncing();

void onButtonPress(int led_show_index_i)
{
	// Add a new node to the list
	// but not if the current node is still unpopulated
	// (at the start)
	if (current_sequence_node->value != -1) {
		node *new_node = (node *) malloc(sizeof(node));
		if (new_node == NULL)
			exit(1);
		current_sequence_node->next = new_node;
		current_sequence_node = new_node;
	}
	current_sequence_node->value = led_show_index_i;
	current_sequence_node->next = NULL;

#ifdef DEBUG
	printf("Currently selected indices:");
	node *temp_ptr = first_sequence_node;
	while (temp_ptr->next != NULL) {
		printf(" %d", temp_ptr->value);
		temp_ptr = temp_ptr->next;
	}
	printf(" %d" endl, temp_ptr->value);
#endif

}
void onDoubleClick()
{
	// disable the button until a reset
	button.rise(NULL);
	// There has been a second click without any
	// change of LED color, so start the new sequence
	queue->call(&start_user_sequence);
}

void onButtonPressISR()
{
	button.rise(NULL);
	button_debounce_timeout.attach(onButtonStopDebouncing,
								   debounce_time_interval);
	// don't let the queued function check the led in case it is too slow
	queue->call(&onButtonPress, led_show_index);
}

void onButtonStopDebouncing()
{
	// This will be overriden when the Ticker goes over to the next LED state
	button.rise(onDoubleClick);
}

void onCycleTick()
{
	// I could also use my CircularArray from Activity 2 here, but this is fine
	++led_show_index %= led_values_n;	// Don't tell me this is unreadable, it's beautiful!
	set_led(led_values[led_show_index]);
	// Reset the button ISR to the ordinary single click function
	button.rise(onButtonPressISR);
}

void next_led_sequence()
{
	// would be nice if I bothered to turn the linked list
	// into a proper class, then I could use a ++ here!
	set_led(led_values[current_sequence_node->value]);
	current_sequence_node = current_sequence_node->next;
}

void onCycleTickUser()
{
	queue->call(&next_led_sequence);
}

void start_user_sequence()
{
	// Make the linked list circular since we will be cycling around it. Since
	// a double click initiated the sequence, we also have to remove the last value

	// There is a potential race condition if onButtonPress finishes after the
	// debounce timeout and a double click happens, but it really isn't going
	// to happen in practice and it's a lot of work to allow for that condition
	current_sequence_node = first_sequence_node;
	if (current_sequence_node->next == NULL) {
		// there is no sequence, fail by ignoring the double click completely
		first_sequence_node->value = -1;
		first_sequence_node->next = NULL;
		button.rise(onButtonPressISR);
		return;
	}
	while (current_sequence_node->next->next != NULL)
		current_sequence_node = current_sequence_node->next;
	current_sequence_node->next = first_sequence_node;

	// reset to the start of the sequence and replace cycle ticker
	current_sequence_node = first_sequence_node;
#ifdef DEBUG
	printf("Starting new sequence" endl);
#endif
	cycle_ticker.detach();
	cycle_ticker.attach(onCycleTickUser, cycle_time_interval);
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

	queue = mbed_event_queue();
	// just a pecularity, we want to start at 0 but we also want to use ++index
	// so that the index isn't ahead.
	led_show_index = -1;

	first_sequence_node = new node;
	if (first_sequence_node == NULL)
		return 1;
	first_sequence_node->value = -1;	// initialize to nonsensical value
	first_sequence_node->next = NULL;
	current_sequence_node = first_sequence_node;

	cycle_ticker.attach(onCycleTick, cycle_time_interval);
	button.rise(onButtonPressISR);

#ifdef DEBUG
	printf("End of main()" endl);
#endif
}
