#include "mbed.h"
#define DEBUG

DigitalOut led1(LED1);			// Green LED
DigitalOut led2(LED2);			// Blue LED
DigitalOut led3(LED3);			// Red LED
InterruptIn button(USER_BUTTON);

// For long ISRs
EventQueue *queue;

Timeout button_debounce_timeout;
float debounce_time_interval = 0.1;

Ticker cycle_ticker;
float cycle_time_interval = 1;

int led_show_index;
int led_values_n = 3;
int led_values[3] = { 1, 2, 3 };

// Basic linked list for the user sequence
typedef struct node_struct {
	int value;
	struct node_struct *next;
} node;

node *first_sequence_node;
node *current_sequence_node;

// I am going to interpret a "double click" as two clicks without
// any change of LED color
bool double_click_available = false;

void onButtonStopDebouncing();
void onButtonPress();
void addToSequence(int led_show_index_i);
void onCycleTick();
void onCycleTickUser();
void start_user_sequence();
void select_led(int l);
void next_led_sequence();
int main();

void select_led(int l)
{
	if (l > 3 || l < 0)
		return;
	led1 = l == 1;
	led2 = l == 2;
	led3 = l == 3;
}

void onButtonStopDebouncing();

void addToSequence(int led_show_index_i)
{
	// This is too slow for the ISR
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
	printf("Current contents of linked list");
	node *temp_ptr = first_sequence_node;
	while (temp_ptr->next != NULL) {
		printf(" %d", temp_ptr->value);
		temp_ptr = temp_ptr->next;
	}
	printf(" %d\n", temp_ptr->value);
#endif

}

void onButtonPress()
{
	if (double_click_available) {
		// disable the button until a reset
		button.rise(NULL);
		// There has been a second click without any
		// change of LED color, so start the new sequence
		queue->call(&start_user_sequence);
	} else {
		double_click_available = true;
		queue->call(&addToSequence, led_show_index);
		button.rise(NULL);
		button_debounce_timeout.attach(onButtonStopDebouncing,
									   debounce_time_interval);
	}
}

void onButtonStopDebouncing()
{
	button.rise(onButtonPress);
}

void onCycleTick()
{
	led_show_index = (led_show_index + 1) % led_values_n;
	select_led(led_values[led_show_index]);
	double_click_available = false;
}

void next_led_sequence()
{
	select_led(led_values[current_sequence_node->value]);
	current_sequence_node = current_sequence_node->next;
}

void onCycleTickUser()
{
	queue->call(&next_led_sequence);
}

void start_user_sequence()
{
	// make the linked list circular
	// since a double click initiated the sequence,
	// we also have to remove the last value
	current_sequence_node = first_sequence_node;
	if (current_sequence_node->next == NULL) {
		// there is no sequence, fail by doing nothing
		first_sequence_node->value = -1;
		first_sequence_node->next = NULL;
		button.rise(onButtonPress);
		return;
	}
	while (current_sequence_node->next->next != NULL)
		current_sequence_node = current_sequence_node->next;
	current_sequence_node->next = first_sequence_node;

	// reset to the start of the sequence and replace cycle ticker
	current_sequence_node = first_sequence_node;
#ifdef DEBUG
	printf("Starting new sequence\n");
#endif
	cycle_ticker.detach();
	cycle_ticker.attach(onCycleTickUser, cycle_time_interval);
}

int main()
{
	printf("Program start\n");
	queue = mbed_event_queue();
	led_show_index = -1;

	first_sequence_node = (node *) malloc(sizeof(node));
	if (first_sequence_node == NULL)
		return 1;
	first_sequence_node->value = -1;	// initialize to nonsensical value
	first_sequence_node->next = NULL;
	current_sequence_node = first_sequence_node;

	cycle_ticker.attach(onCycleTick, cycle_time_interval);
	button.rise(onButtonPress);
}
