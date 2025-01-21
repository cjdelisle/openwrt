#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <asm/io.h>

#ifndef VPchar
#define VPchar			*(volatile unsigned char *)
#endif

#ifdef __BIG_ENDIAN
#define CR_UART_OFFSET		(0x03)
#else
#define CR_UART_OFFSET		(0x0)
#endif
#define	CR_UART_BASE    	0xBFBF0000
#define	CR_UART_THR     	(0x00+CR_UART_BASE+CR_UART_OFFSET)
#define	CR_UART_RBR     	(0x00+CR_UART_BASE+CR_UART_OFFSET)
#define	CR_UART_LSR     	(0x14+CR_UART_BASE+CR_UART_OFFSET)

#define	LSR_INDICATOR		VPchar(CR_UART_LSR)

#define	LSR_RECEIVED_DATA_READY	0x01
#define	LSR_THRE		0x20

static char ppbuf[1024];

void prom_putchar(char data)
{
	while (!(LSR_INDICATOR & LSR_THRE))
		;
	VPchar(CR_UART_THR) = data;
}

char prom_getchar(void)
{
	while (!(LSR_INDICATOR & LSR_RECEIVED_DATA_READY))
		;
	return VPchar(CR_UART_RBR);
}

static void uart_write_buf(const char *buf, unsigned int n)
{
	char ch;

	while (n != 0) {
		--n;
		if ((ch = *buf++) == '\n')
			prom_putchar('\r');
		prom_putchar(ch);
	}
}

void prom_debugf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vscnprintf(ppbuf, sizeof(ppbuf), fmt, args);
	va_end(args);

	uart_write_buf(ppbuf, i);
}
EXPORT_SYMBOL(prom_debugf);
