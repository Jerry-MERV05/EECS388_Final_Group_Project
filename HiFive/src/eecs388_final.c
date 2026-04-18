#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "eecs388_lib.h"

/*****NOTE TO OURSELVES*****/
void handle_trap(void); //compiler needs this declaration to compile the code, but the actual function is defined in eecs388_Lib.c
//-------------------------

//Array of function points for interrupts and exceptions
void (*interrupt_handler[MAX_INTERRUPTS])();
void (*exception_handler[MAX_INTERRUPTS])();

// Task 3 (Our group).
#define BUFF_SIZE (32)
char buff[BUFF_SIZE]; // buffer to store the data read from the lidar (LAB 8)

volatile float steering_angle = 0.0; // global variable to store the steering angle

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
        else // otherwise flash the red light
        {           
            gpio_write(GREEN_LED, OFF);
            gpio_write(RED_LED, ON);
            delay(100);
            gpio_write(RED_LED, OFF);
            delay(100);
        }
            
    }
    
} // check if the lidar is ready to send data
    

int read_from_pi(int devid)
{
    // Task-3:
    if (ser_isready(devid)) 
    {
        ser_readline(devid, BUFF_SIZE, buff);
        // convert received string to float
        sscanf(buff, "%f", &steering_angle);
    }   

    return (int)steering_angle;

}

void steering(int gpio, int pos)
{
    // Task-4:
    // Your code goes here (Use Lab 05 for reference)
    // Check the project document to understand the task

    int pulse = 544 + ((2400 - 544) * pos) / 180;     //converts angle in to pulse

    gpio_write(gpio, ON);
    delay_usec(pulse);
    gpio_write(gpio, OFF);

}

int main()
{
    // initialize UART channels
    ser_setup(0);            // uart0
    ser_setup(1);            // uart1
    int pi_to_hifive = 1;    // The connection with Pi uses uart 1
    int lidar_to_hifive = 0; // the lidar uses uart 0
    int runNext = 0;
    

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
        int angle = read_from_pi(pi_to_hifive); // getting turn direction from pi
        printf("\nangle=%d", angle); 
        int gpio = PIN_19;

        int runNext;
        if(get_cycles() >= runNext){
            runNext = get_cycles() + 20000; // run every 100ms
            steering(gpio, angle);
        }
    }
    return 0;
}
