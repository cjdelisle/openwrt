// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 *  Copyright (C) 2025 Caleb James DeLisle <cjd@cjdns.fr>
 */

#include <linux/string.h>

#include <asm/bootinfo.h>
#include <asm/addrspace.h>

#include "common.h"

#ifdef __BIG_ENDIAN
#define CR_UART_OFFSET		(0x03)
#else
#define CR_UART_OFFSET		(0x0)
#endif
#define	CR_UART_BASE    	0xBFBF0000

const char *get_system_type(void)
{
	return "EcoNet EN75XX";
}

/*************************
 * HSUART Module Registers *
 *************************/
#define	CR_HSUART_BASE    	0xBFBF0300
#define	CR_HSUART_RBR     	(0x00+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_THR     	(0x00+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_IER     	(0x04+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_IIR     	(0x08+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_FCR     	(0x08+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_LCR     	(0x0c+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_MCR     	(0x10+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_LSR     	(0x14+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_MSR     	(0x18+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_SCR     	(0x1c+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_BRDL    	(0x00+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_BRDH    	(0x04+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_WORDA		(0x20+CR_HSUART_BASE+0x00)
#define	CR_HSUART_HWORDA	(0x28+CR_HSUART_BASE+0x00)
#define	CR_HSUART_MISCC		(0x24+CR_HSUART_BASE+CR_UART_OFFSET)
#define	CR_HSUART_XYD     	(0x2c+CR_HSUART_BASE)

#ifndef VPchar
#define VPchar			*(volatile unsigned char *)
#endif
#ifndef VPint
#define VPint			*(volatile unsigned long int *)
#endif
#define	UART_BRD_ACCESS		0x80
#define	UART_XYD_Y		65000
#define	UART_UCLK_115200	0
#define	UART_UCLK_57600		1
#define	UART_UCLK_38400		2
#define	UART_UCLK_28800		3
#define	UART_UCLK_19200		4
#define	UART_UCLK_14400		5
#define	UART_UCLK_9600		6
#define	UART_UCLK_4800		7
#define	UART_UCLK_2400		8
#define	UART_UCLK_1200		9
#define	UART_UCLK_600		10
#define	UART_UCLK_300		11
#define	UART_UCLK_110		12
#define	UART_BRDL		0x03
#define	UART_BRDH		0x00
#define	UART_BRDL_20M		0x01
#define	UART_BRDH_20M		0x00
#define	UART_LCR		0x03
#define	UART_FCR		0x0f
#define	UART_WATERMARK		(0x0<<6)
#define	UART_MCR		0x0
#define	UART_MISCC		0x0
#define	UART_IER		0x01

static inline void uart_setup(void)
{
	unsigned int div_x, div_y, word;

	/* Set FIFO controo enable, reset RFIFO, TFIFO, 16550 mode, watermark=0x00 (1 byte) */
	VPchar(CR_HSUART_FCR) = UART_FCR | UART_WATERMARK;

	/* Set modem control to 0 */
	VPchar(CR_HSUART_MCR) = UART_MCR;

	/* Disable IRDA, Disable Power Saving Mode, RTS , CTS flow control */
	VPchar(CR_HSUART_MISCC) = UART_MISCC;

	/* Access the baudrate divider */
	VPchar(CR_HSUART_LCR) = UART_BRD_ACCESS;

	div_y = UART_XYD_Y;
//#if defined(CONFIG_RT2880_UART_115200)
	div_x = 59904;	// Baud rate 115200
//#else
//	div_x = 29952;	// Baud rate 57600
//#endif
	word = (div_x << 16) | div_y;
	VPint(CR_HSUART_XYD) = word;

	/* Set Baud Rate Divisor to 1*16 */
	VPchar(CR_HSUART_BRDL) = UART_BRDL_20M;
	VPchar(CR_HSUART_BRDH) = UART_BRDH_20M;

	/* Set DLAB = 0, clength = 8, stop = 1, no parity check */
	VPchar(CR_HSUART_LCR) = UART_LCR;
}

void __init prom_init(void)
{
	prom_debugf("Before uart_setup()\n");
	uart_setup();
	prom_debugf("ITS ALIVE!!!\n");

	setup_8250_early_printk_port(CKSEG1ADDR(CR_UART_BASE), 0, 0);
}
