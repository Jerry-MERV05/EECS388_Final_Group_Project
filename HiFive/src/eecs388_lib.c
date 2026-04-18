#include <stdint.h>
#include "eecs388_lib.h"

#define MAX_INTERRUPTS 16

char __buf[80];

void gpio_mode(int gpio, int mode)
{
  uint32_t val;
  
  if (mode == OUTPUT) {
    val = *(volatile uint32_t *) (GPIO_CTRL_ADDR + GPIO_OUTPUT_EN);
    val |= (1<<gpio);
    *(volatile uint32_t *) (GPIO_CTRL_ADDR + GPIO_OUTPUT_EN) = val;

    if (gpio == RED_LED || gpio == GREEN_LED || gpio == BLUE_LED) {
      // active high
      val = *(volatile uint32_t *) (GPIO_CTRL_ADDR + GPIO_OUTPUT_XOR);
      val |= (1<<gpio);
      *(volatile uint32_t *) (GPIO_CTRL_ADDR + GPIO_OUTPUT_XOR) = val;
    }
  } else if (mode == INPUT) {
    val = *(volatile uint32_t *) (GPIO_CTRL_ADDR + GPIO_INPUT_EN);
    val |= (1<<gpio);
    *(volatile uint32_t *) (GPIO_CTRL_ADDR + GPIO_INPUT_EN) = val;
  }
  return;

}

void gpio_write(int gpio, int state)
{
  uint32_t val = *(volatile uint32_t *) (GPIO_CTRL_ADDR + GPIO_OUTPUT_VAL);
  if (state == ON) 
    val |= (1<<gpio);
  else
    val &= (~(1<<gpio));    
  *(volatile uint32_t *) (GPIO_CTRL_ADDR + GPIO_OUTPUT_VAL) = val;
  return;
}

void set_cycles(uint64_t cycle)
{
  *(volatile uint64_t *)(CLINT_CTRL_ADDR + CLINT_MTIMECMP) = cycle;
}

uint64_t get_cycles(void)
{
  return *(volatile uint64_t *)(CLINT_CTRL_ADDR + CLINT_MTIME);
}

void delay(int msec)
{
  uint64_t tend; 
  tend = get_cycles() + msec * 32768 / 1000;
  while (get_cycles() < tend) {}; 
}

void delay_usec(int usec)
{
  uint64_t tend; 
  tend = get_cycles() + (uint64_t)usec * 32768 / 1000000;
  while (get_cycles() < tend) {}; 
}

void ser_setup(int devid)
{
  /* initialize UART0 TX/RX */
  *(volatile uint32_t *)(UART_ADDR(devid) + UART_TXCTRL) |= 0x1;
  *(volatile uint32_t *)(UART_ADDR(devid) + UART_RXCTRL) |= 0x1;

  *(volatile uint32_t *)(UART_ADDR(devid) + UART_IE) |= 0x3; /* UART interrupt enable */
  *(volatile uint32_t *)(UART_ADDR(devid) + UART_DIV) = 139; /* baudrate ~115200 bps */

  /* enable UART1 IOF */
  *(volatile uint32_t *)(GPIO_CTRL_ADDR + GPIO_IO_FUNC_EN) |= 0x840000;  
}

int  ser_isready(int devid)
{
  uint32_t regval = *(volatile uint32_t *)(UART_ADDR(devid) + UART_IP);
  return regval;
}

void ser_write(int devid, char c)
{
  uint32_t regval;
  /* busy-wait if tx FIFO is full  */
  do {
    regval = *(volatile uint32_t *)(UART_ADDR(devid) + UART_TXDATA);
  } while (regval & 0x80000000);

  /* write the character */
  *(volatile uint32_t *)(UART_ADDR(devid) + UART_TXDATA) = c;
}

void ser_printline(int devid, char *str)
{
  int i;
  for (i = 0;; i++) {
    if (str[i] == '\n') {
      ser_write(devid, '\r');
      ser_write(devid, '\n');
    } else if (str[i] == '\0') {
      break;
    } else {
      ser_write(devid, str[i]);
    }
  }
}

char ser_read(int devid)
{
  uint32_t regval;
  /* busy-wait if receive FIFO is empty  */
  do {
    regval = *(volatile uint32_t *)(UART_ADDR(devid) + UART_RXDATA);
  } while (regval & 0x80000000);
  /* return a byte */
  return (uint8_t)(regval & 0xFF);
}

int ser_readline(int devid, int n, char *str)
{
  int i = 0; 
  for (i = 0; i < n; i++) {
#if 0     
    if (!ser_isready(devid)) {// non-blocking io
      str[i] = 0;
      return i;
    }
#endif
    str[i] = ser_read(devid);
    ser_write(0, str[i]);
    if (str[i] == '\r' || str[i] == '\n') {
      str[i] = 0;
      return i;      
    } 
  }
  str[i-1] = 0;
  return i; 
}