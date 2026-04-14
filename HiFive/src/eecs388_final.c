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

// Task 1 & 2 (Our group)
// led_state is defined in eecs388_lib.h and is used to keep track of the current state of the braking system.
extern volatile led_state braking_state;
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
            braking_state = LED_GREEN;
        }
        else if (100 < dist && dist <= 200) // if the distance is between 100 and 200 turn the light yellow
        {
            braking_state = LED_YELLOW;
        }
        else if (60 < dist && dist <= 100) // if the distance is between 60 and 100 turn the light red
        {
            braking_state = LED_RED;
        }
        else // otherwise flash the red light
        {
            braking_state = LED_FLASHING_RED;
        }
            
    }
} // check if the lidar is ready to send data
    

int read_from_pi(int devid)
{
    // Task-3:
    // You code goes here (Use Lab 09-option1 for reference)
    // After performing Task-2 at dnn.py code, modify this part to read angle values from Raspberry Pi.
    
    /* ---SAFER METHOD ?---
    if (ser_isready(devid)){
        steering_angle = ser_readline(devid, BUF_SIZE, buff);
    }

    float temp = 0.0;
    sscanf(buff, "%f", &temp); // convert the string read from the Pi to a float
    steering_angle = temp;

    return (int)steering_angle;
    */

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
    delay_us(pulse);
    gpio_write(gpio, OFF);
    delay_us(20000 - pulse);

}

int main()
{
    // initialize UART channels
    ser_setup(0);            // uart0
    ser_setup(1);            // uart1
    int pi_to_hifive = 1;    // The connection with Pi uses uart 1
    int lidar_to_hifive = 0; // the lidar uses uart 0
    

    printf("\nUsing UART %d for Pi -> HiFive", pi_to_hifive);
    printf("\nUsing UART %d for Lidar -> HiFive", lidar_to_hifive);

    // Initializing PINs
    gpio_mode(PIN_19, OUTPUT);
    gpio_mode(RED_LED, OUTPUT);
    gpio_mode(BLUE_LED, OUTPUT);
    gpio_mode(GREEN_LED, OUTPUT);

    // install timer interrupt handler
    interrupt_handler[MIE_MTIE_BIT] = timer_handler;

    // write handle_trap address to mtvec
    register_trap_handler(handle_trap);

    // enable timer interrupt
    enable_timer_interrupt();

    // enable global interrupt
    enable_interrupt(); 

    // cause timer interrupt for some time in future 
    set_cycles( get_cycles() + 40000 );

    printf("Setup completed.\n");
    printf("Begin the main loop.\n");

    while (1)
    {

        auto_brake(lidar_to_hifive);            // measuring distance using lidar and braking
        int angle = read_from_pi(pi_to_hifive); // getting turn direction from pi
        printf("\nangle=%d", angle); 
        int gpio = PIN_19;
        for (int i = 0; i < 10; i++)
        {
            // Here, we set the angle to 180 if the prediction from the DNN is a positive angle
            // and 0 if the prediction is a negative angle.
            // This is so that it is easier to see the movement of the servo.
            // You are welcome to pass the angle values directly to the steering function.
            // If the servo function is written correctly, it should still work,
            // only the movements of the servo will be more subtle
            if (angle > 0)
            {
                steering(gpio, 180);
            }
            else
            {
                steering(gpio, 0);
            }

            // Uncomment the line below to see the actual angles on the servo.
            // Remember to comment out the if-else statement above!
            // steering(gpio, angle);
        }
    }
    return 0;
}
