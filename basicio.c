#include <msp.h>
#include <stdio.h>

//MARK: Constants
#define DEBOUNCE_TIME 5000

#define LED_RED 0
#define LED_RGB 1

#define RGB_LED_MASK 0x07
#define RGB_LED_OFFSET 0

//MARK: Function prototypes
static void main_loop(void);
void select_led_button_action(void);
void select_mode_button_action(void);

//MARK: Variable Definitions
static uint8_t selected_led_g;

/*
 Ensure that both the SEL0 and SEL1 bits for a GPIO pin are cleared without
 the possibility of going through an intermediate state.
 
 @param port Base address for the GPIO port that the pin is in.
 @param mask Bit mask for the GPIO pin.
*/
static inline void ensure_func_gpio_odd(DIO_PORT_Odd_Interruptable_Type *port, uint8_t mask){
	if((port->SEL0 & mask) && (port->SEL1 & mask)){
		port->SELC |= mask;
	} else {
		port->SEL0 &= ~(mask);
		port->SEL1 &= ~(mask);
	}
}

/*
Ensure that both the SEL0 and the SEL1 bits for a GPIO pin are cleared without the 
possibility of going through an intermediate state.

@param port Base address for the GPIO port that the pin is in
@param mask Bit mask for the GPIO pin.
*/
static inline void ensure_func_gpio_even(DIO_PORT_Even_Interruptable_Type *port, uint8_t mask){
	if((port->SEL0 & mask) && (port->SEL1 & mask)){
		port->SELC |= mask;
	} else {
		port->SEL0 &= ~(mask);
		port->SEL1 &= ~(mask);
	}
}

/*
 Initialize the GPIO pins as required for the application
*/
static void init_gpio(void){
	/* Buttons (P1.1, P1.4) */
	
	//Ensure that function is GPIO
	ensure_func_gpio_odd(P1, (1<<1));
	ensure_func_gpio_odd(P1, (1<<4));
	
	//Set direction to input
	P1->DIR &= ~((1<<1) | (1<<4));
	
	//Enable pull resistor
	P1->REN |= ((1<<1) | (1<<4));
	
	//Set pull direction to up
	P1->OUT |= ((1<<1) | (1<<4));
	
	//Ensure that interrupts are disabled
	P1->IE &= ~((1<<1) | (1<<4));
	
	/* LEDs (P1.0, P2.0, P2.1, P2.2) */
    // Ensure that function is GPIO
    ensure_func_gpio_odd(P1, (1<<0));
    ensure_func_gpio_even(P2, (1<<0));
    ensure_func_gpio_even(P2, (1<<1));
    ensure_func_gpio_even(P2, BIT2);
    // Set direction to output
    P1->DIR |= (1<<0);
    P2->DIR |= ((1<<0) | (1<<1) | BIT2);
    // Ensure that high drive strength is disabled
    P1->DS &= ~(1<<0);
    P2->DS &= ~((1<<0) | (1<<1) | BIT2);
    // Initialize to driven low
    P1->OUT &= ~(1<<0);
    P2->OUT &= ~((1<<0) | (1<<1) | BIT2);
    // Ensure that interupts are disabled
    P1->IE &= ~((1<<0));
    P1->IE &= ~((1<<0) | (1<<1) | BIT2);

}
int main(){

	// Disable the watchdog timer
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;
	
	/* Configure GPIO */
	init_gpio();
	
	/* Make sure that the LED selection is correct */
	selected_led_g = LED_RED;
	
	
	/* Main Loop */
	for(;;){
		main_loop();
	}
}
static void main_loop ()
{
    static uint8_t select_led_button_pressed;
    static uint8_t select_mode_button_pressed;
    
    // Select LED Button
    if ((!(P1->IN & (1<<1))) && !select_led_button_pressed) {
        uint16_t n = DEBOUNCE_TIME;
        while (n--) {
            // Convince the compiler that this loop contains important code
            __asm volatile ("");
        }
        if (!(P1->IN & (1<<1))) {
            select_led_button_pressed = 1;
            select_led_button_action();
        }
    } else if (P1->IN & (1<<1)) {
        select_led_button_pressed = 0;
    }
    
    // Select Mode Button
    if (!(P1->IN & (1<<4)) && !select_mode_button_pressed) {
        uint16_t n = DEBOUNCE_TIME;
        while (n--) {
            // Convince the compiler that this loop contains important code
            __asm volatile ("");
        }
        if (!(P1->IN & (1<<4))) {
            select_mode_button_pressed = 1;
            select_mode_button_action();
        }
    } else if (P1->IN & (1<<4)) {
        select_mode_button_pressed = 0;
    }
}

void select_led_button_action(void)
{
    selected_led_g = (selected_led_g == LED_RED) ? LED_RGB : LED_RED;
}

void select_mode_button_action(void)
{
    if (selected_led_g == LED_RED) {
        P1->OUT ^= (1<<0);
    } else if (selected_led_g == LED_RGB) {
        uint8_t led_state = ((P2->OUT & RGB_LED_MASK) >> RGB_LED_OFFSET);
        led_state++;
        P2->OUT &= ~RGB_LED_MASK;
        P2->OUT |= (led_state & RGB_LED_MASK) << RGB_LED_OFFSET;
    }
}
