#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "eecs388_lib.h"

// Task 3 (Our group).
#define BUFF_SIZE (32)
char buff[BUFF_SIZE]; // buffer to store the data read from the lidar (LAB 8)
volatile float steering_angle = 0.0; // global variable to store the steering angle
int led_state = 0;
int next_toggle = 0;

//--------------------------

void auto_brake(int devid)
{
    // Task1 & 2:
    // Your code here (Use Lab 02 - Lab 04 for reference)
    // Use the directions given in the project document

    uint16_t dist = 0; // distance from the lidar

    if ('Y' == ser_read(devid) && 'Y' == ser_read(devid)) // check for the start of a new data packet
    {
        uint8_t dist_L = ser_read(devid); // Read third byte (low byte of distance)
        uint8_t dist_H = ser_read(devid); // Read fourth byte (high byte of distance)
        dist = (dist_H << 8) | dist_L;    // Combine high and low bytes to get distance

        if (dist > 200) // if dist is greater than 200 make the led green
        {
            gpio_write(RED_LED, OFF);
            gpio_write(GREEN_LED, ON); 
        }
        else if (100 < dist && dist <= 200) // if the distance is between 100 and 200 turn the light yellow
        {          
            gpio_write(GREEN_LED, ON);
            gpio_write(RED_LED, ON);
        }
        else if (60 < dist && dist <= 100) // if the distance is between 60 and 100 turn the light red
        {           
            gpio_write(GREEN_LED, OFF);
            gpio_write(RED_LED, ON);
        }
        else
        {
        gpio_write(GREEN_LED, OFF);

        if (get_cycles() >= next_toggle) 
        {
            led_state = !led_state;
            gpio_write(RED_LED, led_state);

            next_toggle = get_cycles() + 2500; 
        }
        }   
            
    }
    
} // check if the lidar is ready to send data
    

int read_from_pi(int devid, int *angle_out)
{
    if (ser_isready(devid)) 
    {
        ser_readline(devid, BUFF_SIZE, buff);

        int temp;
        if (sscanf(buff, "%d", &temp) == 1) {
            *angle_out = temp;
            return 1; // new data
        }
    }
    return 0; // no new data
}

void steering(int gpio, int pos)
{
    // Task-4:
    // Your code goes here (Use Lab 05 for reference)
    // Check the project document to understand the task
    if (pos < 0) pos = 0;
    if (pos > 180) pos = 180;

    int pulse = (544 + ((2400 - 544) * pos)) / 180;     //converts angle in to pulse

    gpio_write(gpio, ON);
    delay_usec(pulse);
    gpio_write(gpio, OFF);
    delay_usec(20000 - pulse); // wait for the rest of the 20ms period

}

int main()
{
    // initialize UART channels
    ser_setup(0);            // uart0
    ser_setup(1);            // uart1
    int pi_to_hifive = 1;    // The connection with Pi uses uart 1
    int lidar_to_hifive = 0; // the lidar uses uart 0
    int runNext = 0;
    int gpio = PIN_19;

    printf("\nUsing UART %d for Pi -> HiFive", pi_to_hifive);
    printf("\nUsing UART %d for Lidar -> HiFive", lidar_to_hifive);

    // Initializing PINs
    gpio_mode(PIN_19, OUTPUT);
    gpio_mode(RED_LED, OUTPUT);
    gpio_mode(BLUE_LED, OUTPUT);
    gpio_mode(GREEN_LED, OUTPUT);

    printf("Setup completed.\n");
    printf("Begin the maiget_cycles();n loop.\n");

    while (1)
    {

        auto_brake(lidar_to_hifive);            // measuring distance using lidar and braking
        
        int angle;
        if (read_from_pi(pi_to_hifive, &angle)) 
        {
            printf("\n>>> NEW angle=%d <<<\n", angle);
        }

        for (int i = 0; i < 25; i++) {
            steering(gpio, angle); // steering the car based on the angle received from the pi
        }
        
        // if(get_cycles() >= runNext){
        //     runNext = get_cycles() + 20000; // run every 100ms
        
        // }
    }
    return 0;
}