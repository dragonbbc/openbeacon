#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

typedef unsigned int BOOL;
#define TRUE 1
#define FALSE 0

#define PACKED __attribute__((packed))
#define WEAK __attribute__ ((weak))
#define ALIAS(f) __attribute__ ((weak, alias (#f)))

#include <stdint.h>
#include <config.h>

/* this definition is linked weakly against UARTSendChar */
extern BOOL default_putchar (uint8_t data);

#ifdef  __LPC13xx__

#include <LPC13xx.h>
#include <uart.h>
#ifndef ENALBLE_USB_FULLFEATURED
#include <usb.h>
#include <usbdesc.h>
#include <rom_drivers.h>
#endif/*ENALBLE_USB_FULLFEATURED*/
#include <gpio.h>

/* LPC_SYSCON->SYSAHBCLKCTRL bits */
#define EN_SYS              (1<< 0)
#define EN_ROM              (1<< 1)
#define EN_RAM              (1<< 2)
#define EN_FLASHREG         (1<< 3)
#define EN_FLASHARRAY       (1<< 4)
#define EN_I2C              (1<< 5)
#define EN_GPIO             (1<< 6)
#define EN_CT16B0           (1<< 7)
#define EN_CT16B1           (1<< 8)
#define EN_CT32B0           (1<< 9)
#define EN_CT32B1           (1<<10)
#define EN_SSP              (1<<11)
#define EN_UART             (1<<12)
#define EN_ADC              (1<<13)
#define EN_USB_REG          (1<<14)
#define EN_WDT              (1<<15)
#define EN_IOCON            (1<<16)

#ifdef  __LPC1343__
#define LPC_RAM_SIZE (8*1024)
#define LPC_FLASH_SIZE (32*1024)
#endif/*__LPC1343__*/
#ifdef  __LPC1342__
#define LPC_RAM_SIZE (4*1024)
#define LPC_FLASH_SIZE (16*1024)
#endif/*__LPC1342__*/

#else /*__LPC13xx__*/
#error Please specify architecture
#endif/*__LPC13xx__*/

#define DEVICEID_LPC1311 0x2C42502BUL
#define DEVICEID_LPC1313 0x2C40102BUL
#define DEVICEID_LPC1342 0x3D01402BUL
#define DEVICEID_LPC1343 0x3D00002BUL

#ifdef  ENABLE_FREERTOS
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#endif/*ENABLE_FREERTOS*/

#include <debug_printf.h>
#include <string.h>
#include <crc16.h>

static inline uint16_t htons(uint16_t x)
{
  __asm__ ("rev16 %0, %1" : "=r" (x) : "r" (x));
  return x;
}

static inline uint32_t htonl(uint32_t x)
{
  __asm__ ("rev %0, %1" : "=r" (x) : "r" (x));
  return x;
}

#define ntohl(l) htonl(l)
#define ntohs(s) htons(s)

#endif/*__OPENBEACON_H__*/
