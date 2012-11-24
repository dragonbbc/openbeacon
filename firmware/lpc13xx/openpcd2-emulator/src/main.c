/***************************************************************
 *
 * OpenBeacon.org - main file for OpenPCD2 basic demo
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
 *
 ***************************************************************

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */
#include <openbeacon.h>
#include "main.h"
#include "usbserial.h"

#define MAX_EDGES 128

static volatile uint32_t edges;
static uint16_t g_counter_prev = 0;
static uint8_t g_counter_overflow = 0;

static uint16_t g_edge[MAX_EDGES];
static uint16_t g_edge_pos = 0;

static void
rfid_hexdump (const void *buffer, int size)
{
	int i;
	const unsigned char *p = (unsigned char *) buffer;

	for (i = 0; i < size; i++)
	{
		if (i && ((i & 3) == 0))
			debug_printf (" ");
		debug_printf (" %02X", *p++);
	}
	debug_printf (" [size=%02i]\n", size);
}

void TIMER16_1_IRQHandler(void)
{
	uint8_t reason;
	uint16_t counter,diff;

	/* get interrupt reason */
	reason = (uint8_t)LPC_TMR16B1->IR;

	/* if overflow - reset pulse length detection */
	if(reason & 0x01)
	{
		if(!g_counter_overflow)
		{
			g_counter_overflow = TRUE;
			if(g_edge_pos<MAX_EDGES)
				g_edge[g_edge_pos++]=0xFFFF;
		}
		GPIOSetValue (LED_PORT, LED_BIT, 0);
	}
	else
		/* we captured an edge */
		if(reason & 0x10)
		{
			/* get captured value */
			counter = LPC_TMR16B1->CR0;
			/* update overflow register */
			LPC_TMR16B1->MR0 = counter;

			/* ignore first value after overflow */
			if(g_counter_overflow)
				g_counter_overflow=FALSE;
			else
			{
				/* calculate difference */
				diff = (counter-g_counter_prev);
				if(diff && (g_edge_pos<MAX_EDGES))
					g_edge[g_edge_pos++]=diff;
			}
			/* remember current counter value */
			g_counter_prev=counter;

			GPIOSetValue (LED_PORT, LED_BIT, 1);
		}

	/* acknowledge IRQ reason */
	LPC_TMR16B1->IR = reason;
}

static inline void
rfid_init_emulator (void)
{
	/* enable timer 32B0 & 16B1 */
	LPC_SYSCON->SYSAHBCLKCTRL |= EN_CT32B0|EN_CT16B1;

	/* reset and stop counter 16B0 */
	LPC_TMR16B1->TCR = 0x02;
	LPC_TMR16B1->TCR = 0;
	LPC_TMR16B1->TC = 0;
	LPC_TMR16B1->CTCR = 0;
	/* match on MR0 for overflow detection */
	LPC_TMR16B1->MR0 = 0xFFFF;
	LPC_TMR16B1->MCR = 1;
	LPC_TMR16B1->EMR = 0;
	LPC_TMR16B1->PWMC = 0;
	/* Capture rising edge & trigger interrupt */
	LPC_TMR16B1->CCR = 5;
	/* disable prescaling */
	LPC_TMR16B1->PR = 0;
	/* enable CT16B1_CAP0 for signal duration capture */
	LPC_IOCON->PIO1_8 = 0x1;
	/* enable timer capture interrupt */
	NVIC_EnableIRQ(TIMER_16_1_IRQn);
	NVIC_SetPriority(TIMER_16_1_IRQn, 1);
	/* reset previous counter value */
	g_counter_prev = g_counter_overflow = 0;
	/* run counter */
	LPC_TMR16B1->TCR = 0x01;

	/* reset and stop counter 32B0 */
	LPC_TMR32B0->TCR = 0x02;
	LPC_TMR32B0->TCR = 0x00;
	/* enable CT32B0_CAP0 timer clock */
	LPC_IOCON->PIO1_5 = 0x02;
	/* no capturing */
	LPC_TMR32B0->CCR = 0x00;
	/* no match */
	LPC_TMR32B0->MCR = 0;
	LPC_TMR32B0->EMR = 0;
	LPC_TMR32B0->PWMC = 0;
	/* incremented upon CAP0 input rising edge */
	LPC_TMR32B0->CTCR = 0x01;
	/* disable prescaling */
	LPC_TMR32B0->PR = 0;
	/* run counter */
	LPC_TMR32B0->TCR = 0x01;

	/* enable RxMultiple mode */
	rfid_write_register (0x6303, 0x0C);
	/* disable parity for TX/RX */
	rfid_write_register (0x630D, 0x10);

	/* enable SVDD switch */
	rfid_write_register (0x6306, 0x2F);
	rfid_write_register (0x6307, 0x03);
	rfid_write_register (0x6106, 0x10);
	/* enable secure clock */
	rfid_write_register (0x6330, 0x80);
	rfid_write_register (0x6104, 0x00);
	/* output envelope to AUX1 */
	rfid_write_register (0x6321, 0x34);
	rfid_write_register (0x6322, 0x0E);
	rfid_write_register (0x6328, 0xF9);

	/* WTF? FIXME !!! */
	LPC_SYSCON->SYSAHBCLKCTRL |= EN_CT32B0|EN_CT16B1;
}

void
PIOINT3_IRQHandler (void)
{
	static int i = 0;

	LPC_GPIO3->IC = 1 << PN532_SIGOUT_PIN;

	edges++;
//  GPIOSetValue (LED_PORT, LED_BIT, GPIOGetValue(1,8));
	GPIOSetValue (LED_PORT, LED_BIT, (i++) & 1);
}

static void
trigger_init (void)
{
	edges = 0;
	/* enable SIGOUT input pin */
	GPIOSetDir (PN532_SIGOUT_PORT, PN532_SIGOUT_PIN, 0);
	/* enable SIGOUT interrupt */
//  LPC_GPIO3->IE |= 1<<PN532_SIGOUT_PIN;

//  NVIC_EnableIRQ(EINT3_IRQn);
//  NVIC_SetPriority(EINT3_IRQn, 1);
}

static void
loop_rfid (void)
{
	int res, line, i;
	uint32_t counter;
	static unsigned char data[80];

	/* fully initialized */
	GPIOSetValue (LED_PORT, LED_BIT, LED_ON);

	/* read firmware revision */
	debug_printf ("\nreading firmware version...\n");
	data[0] = PN532_CMD_GetFirmwareVersion;
	while ((res = rfid_execute (&data, 1, sizeof (data))) < 0)
	{
		debug_printf ("Reading Firmware Version Error [%i]\n", res);
		pmu_wait_ms (450);
		GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
		pmu_wait_ms (10);
		GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);
	}

	debug_printf ("PN532 Firmware Version: ");
	if (data[1] != 0x32)
		rfid_hexdump (&data[1], data[0]);
	else
		debug_printf ("v%i.%i\n", data[2], data[3]);

	/* Set PN532 to virtual card */
	data[0] = PN532_CMD_SAMConfiguration;
	data[1] = 0x02;																/* Virtual Card Mode */
	data[2] = 0x00;																/* No Timeout Control */
	data[3] = 0x00;																/* No IRQ */
	if ((res = rfid_execute (&data, 4, sizeof (data))) > 0)
	{
		debug_printf ("SAMConfiguration: ");
		rfid_hexdump (&data, res);
	}

	/* init RFID emulator */
	rfid_init_emulator ();

	/* enable debug output */
	GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
	line = 0;

	while (1)
	{
		GPIOSetValue (LED_PORT, LED_BIT, 1);
		counter = LPC_TMR32B0->TC;
		pmu_wait_ms (100);
		counter = (LPC_TMR32B0->TC - counter) * 10;
		GPIOSetValue (LED_PORT, LED_BIT, 0);

		debug_printf ("LPC_TMR32B0[%08u]: %uHz [edges=%u]\n", line++, counter, g_edge_pos);

		/* debug all edges */
		for(i=0;i<g_edge_pos;i++)
		{
			debug_printf(" %5u",g_edge[i]);
			if((i&7)==7)
			{
				debug_printf("\n");
				pmu_wait_ms (50);
			}
		}
		debug_printf("\n%u EDGES detected\n\n",g_edge_pos);
	}
}

int
main (void)
{
	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* Set LED port pin to output */
	GPIOSetDir (LED_PORT, LED_BIT, 1);
	GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);

	/* Init Power Management Routines */
	pmu_init ();

	/* UART setup */
	UARTInit (115200, 0);

	/* CDC USB Initialization */
	init_usbserial ();

	/* Init RFID */
	rfid_init ();

	/* Init emulation triggers */
	trigger_init ();

	/* RUN RFID loop */
	loop_rfid ();

	return 0;
}
