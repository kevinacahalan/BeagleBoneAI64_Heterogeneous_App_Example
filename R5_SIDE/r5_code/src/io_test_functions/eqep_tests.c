#include <stdio.h>
#include <stdlib.h>
#include <ai64/sys_eqep.h>
#include <ai64/bbai64_eqep.h>
#include <ai64/bbai64_gpio.h>

#include <ti/csl/csl_gpio.h>
#include <ti/csl/soc.h>

#include <ti/osal/osal.h>

// Quadrature encoder state sequence
const int cw_sequence[4][2] = {
    {GPIO_PIN_LOW,  GPIO_PIN_LOW},
    {GPIO_PIN_HIGH, GPIO_PIN_LOW},
    {GPIO_PIN_HIGH, GPIO_PIN_HIGH},
    {GPIO_PIN_LOW,  GPIO_PIN_HIGH} 
};

// Index of cw_sequence
int state_idx = 0;
const int states_per_pulse = 4;


// Pin definitions for encoder simulation
#define PIN_GPIO_TO_EQEPA P8_36  // Connect to P8_35(eqepA)
#define PIN_GPIO_TO_EQEPB P8_34  // Connect to P8_33(eqepB)


void reset_eqep_and_simulation_io(){
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_DIRECTION_OUTPUT);
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_DIRECTION_OUTPUT);
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_LOW);
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_LOW);
    eqep_reset(EQEP1_P8_35_P8_33);
    state_idx = 0;
}

/**
 * Generate a specified number of quadrature encoder ticks at the given rate
 * 
 * @param tick_count  Number of ticks to generate, negative for counterclockwise, positive for clockwise
 * @param tick_rate_ms  Delay between encoder steps/ticks in milliseconds
 */
void simulate_encoder_ticks(int tick_count, int tick_rate_ms) {
    // Generate encoder ticks
    for (int j = 0; j < abs(tick_count); j++) {
        // Select the correct state based on direction
        state_idx += (tick_count < 0) ? -1 : 1;
        
        int index = (state_idx % states_per_pulse + states_per_pulse) % states_per_pulse;
        
        // Set A and B pins according to state
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), 
                        cw_sequence[index][0]);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), 
                        cw_sequence[index][1]);
        
        // Wait for specified duration
        Osal_delay(tick_rate_ms);
    }
}

/**
 * Tests EQEP1 functionality using GPIO simulation with with jumper wires
 * - MAKE SURE to connect LOOP jumper wires P8_33<-->P8_34 and P8_35<-->P8-36
 */
void test_eqep1_with_gpio_encoder_simulation() {
    // Initialize EQEP module
    eqep_init(EQEP1_P8_35_P8_33);
    
    // Setup simulation pins and reset EQEP.
    reset_eqep_and_simulation_io();
    
    int sum = 0;
    int expected_sum = 0;
    int requested = 0;
    int actual = 0;
    int current = eqep_get_position(EQEP1_P8_35_P8_33);

    // Get initial position
    int initial_position = current;
    printf("EQEP counter initial position: %d\n", initial_position);
    
    requested = 100;
    expected_sum += requested;
    simulate_encoder_ticks(requested, 1);
    current = eqep_get_position(EQEP1_P8_35_P8_33);
    printf("EQEP: requested = %d, actual = %d, current_pos = %d, sum_of_actual = %d, expected_sum = %d\n", requested, actual, current, sum, expected_sum);

    requested = -100;
    expected_sum += requested;
    simulate_encoder_ticks(requested, 1);
    current = eqep_get_position(EQEP1_P8_35_P8_33);
    printf("EQEP: requested = %d, actual = %d, current_pos = %d, sum_of_actual = %d, expected_sum = %d\n", requested, actual, current, sum, expected_sum);

    requested = -100;
    expected_sum += requested;
    simulate_encoder_ticks(requested, 1);
    current = eqep_get_position(EQEP1_P8_35_P8_33);
    printf("EQEP: requested = %d, actual = %d, current_pos = %d, sum_of_actual = %d, expected_sum = %d\n", requested, actual, current, sum, expected_sum);

    requested = 500;
    expected_sum += requested;
    simulate_encoder_ticks(requested, 1);
    current = eqep_get_position(EQEP1_P8_35_P8_33);
    printf("EQEP: requested = %d, actual = %d, current_pos = %d, sum_of_actual = %d, expected_sum = %d\n", requested, actual, current, sum, expected_sum);


    Osal_delay(100);

    // Print expected final position
    printf("Expected final position: %d\n", expected_sum);
    
    if (current == expected_sum) {
        printf("Test PASSED: Final position matches expected value\n");
    } else {
        printf("Test FAILED: Final position %d doesn't match expected %d\n", current, expected_sum);
        printf("MAKE SURE to connect LOOP jumper wires P8_33<-->P8_34 and P8_35<-->P8-36\n");
    }
}
