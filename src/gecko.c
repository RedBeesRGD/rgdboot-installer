/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	USBGecko support code

Copyright (c) 2008		Nuke - <wiinuke@gmail.com>
Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2024		nitr8

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include <stdarg.h>
#include <stdio.h>
#include <ogc/machine/processor.h>

#include "hollywood.h"
#include "gecko.h"
#include "tools.h"


/* #define GECKO_PRINT_ALT			1 */
/* #define GECKO_LFCR			0 */
/* #define GECKO_SAFE			0 */

/* [nitr8]: Not working yet... */
/* #define SCREEN_TO_CONSOLE_OUTPUT	0 */


static u8 gecko_found = 0;
static u8 gecko_console_enabled = 0;


static u32 _gecko_command(u32 command)
{
	u32 i;

	/* Memory Card Port B (Channel 1, Device 0, Frequency 3 (32Mhz Clock)) */
	write32(EXI1_CSR, 0xd0);
	write32(EXI1_DATA, command);
	write32(EXI1_CR, 0x19);

	while (read32(EXI1_CR) & 1);

	i = read32(EXI1_DATA);
	write32(EXI1_CSR, 0);

	return i;
}

static u32 _gecko_getid(void)
{
	u32 i;

	/* Memory Card Port B (Channel 1, Device 0, Frequency 3 (32Mhz Clock)) */
	write32(EXI1_CSR, 0xd0);
	write32(EXI1_DATA, 0);
	write32(EXI1_CR, 0x19);

	while (read32(EXI1_CR) & 1);

	write32(EXI1_CR, 0x39);

	while (read32(EXI1_CR) & 1);

	i = read32(EXI1_DATA);
	write32(EXI1_CSR, 0);

	return i;
}

#ifndef NDEBUG
static u32 _gecko_sendbyte(u8 sendbyte)
{
	u32 i = _gecko_command(0xB0000000 | (sendbyte<<20));

	if (i&0x04000000)
		return 1; /* Return 1 if byte was sent */

	return 0;
}
#endif

static u32 _gecko_recvbyte(u8 *recvbyte)
{
	u32 i = 0;
	*recvbyte = 0;

	i = _gecko_command(0xA0000000);

	if (i&0x08000000)
	{
		/* Return 1 if byte was received */
		*recvbyte = (i>>16)&0xff;

		return 1;
	}

	return 0;
}

#if (!defined(NDEBUG) && (defined(GECKO_SAFE) || defined(GECKO_PRINT_ALT)))
static u32 _gecko_checksend(void)
{
	u32 i = 0;
	i = _gecko_command(0xC0000000);

	if (i&0x04000000)
		return 1; /* Return 1 if safe to send */

	return 0;
}
#endif

static int gecko_isalive(void)
{
	u32 i;

	i = _gecko_getid();

	if (i != 0x00000000)
		return 0;

	i = _gecko_command(0x90000000);

	if ((i & 0xFFFF0000) != 0x04700000)
		return 0;

	return 1;
}

static void gecko_flush(void)
{
	u8 tmp;

	while (_gecko_recvbyte(&tmp));
}

#if (!defined(NDEBUG) && !defined(GECKO_SAFE) && !defined(GECKO_PRINT_ALT))
static int gecko_sendbuffer(const void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;

	while (left > 0)
	{
		if (!_gecko_sendbyte(*ptr))
			break;

		if (*ptr == '\n')
		{
#ifdef GECKO_LFCR
			_gecko_sendbyte('\r');
#endif
		}

		ptr++;
		left--;
	}

	return (size - left);
}
#endif

#if (!defined(NDEBUG) && defined(GECKO_SAFE) && !defined(GECKO_PRINT_ALT))
static int gecko_sendbuffer_safe(const void *buffer, u32 size)
{
	u32 left = size;
	char *ptr = (char*)buffer;

	if ((read32(HW_EXICTRL) & EXICTRL_ENABLE_EXI) == 0)
		return left;
		
	while (left > 0)
	{
		if (_gecko_checksend())
		{
			if (!_gecko_sendbyte(*ptr))
				break;
			
			if (*ptr == '\n')
			{
#ifdef GECKO_LFCR
				_gecko_sendbyte('\r');
#endif
			}

			ptr++;
			left--;
		}
	}

	return (size - left);
}
#endif

#ifdef GECKO_PRINT_ALT

static int gecko_putc(const char s)
{
	char *ptr = (char *)&s;
	int tries = 10000; /* about 200ms of tries; if we fail, fail gracefully instead of just hanging */

	if (!gecko_found)
		return -1;

	if ((read32(HW_EXICTRL) & EXICTRL_ENABLE_EXI) == 0)
		return 0;

	if (_gecko_checksend())
	{
		if (!_gecko_sendbyte(*ptr))
			return 0;

		if (*ptr == '\n')
		{
#ifdef GECKO_LFCR
			_gecko_sendbyte('\r');
#endif
		}
	}
	else
	{
		/* if gecko is hung, time out and disable further attempts
		   only affects gecko users without an active terminal */
		if (tries-- == 0)
		{
			gecko_found = 0;
			return 0;
		}
	}

	return 1;
}

/* [nitr8]: Thanks bushing */
int gecko_printf(const char *str, ...)
{
	va_list arp;
	u8 c;
	u8 f;
	u8 r;
	u32 val;
	u32 pos;
	char s[16];
	s32 i;
	s32 w;

	/* [nitr8]: Support for USB-Gecko debug output must be
		    put here due to Haxx_GetBusAccess() FAIL */
	gecko_init(1);

	/* [nitr8]: Return early if the console isn't enabled at all */
	if (!gecko_console_enabled)
		return 0;

	va_start(arp, str);

	for (pos = 0;; )
	{
		c = *str++;

		/* End of string */
		if (c == 0)
			break;

		/* Non escape cahracter */
		if (c != '%')
		{
			gecko_putc(c);
			pos++;
			continue;
		}

		w = 0;
		f = 0;
		c = *str++;

		/* Flag: '0' padding */
		if (c == '0')
		{
			f = 1;
			c = *str++;
		}

		/* Precision */
		while (c >= '0' && c <= '9')
		{
			w = w * 10 + (c - '0');
			c = *str++;
		}

		/* Type is string */
		if (c == 's')
		{
			char *param = va_arg(arp, char*);

			for (i = 0; param[i]; i++)
			{
				gecko_putc(param[i]);
				pos++;
			}

			continue;
		}

		/* [nitr8]: Add a case for checking chars ('c') */
		/* Type is char */
		if (c == 'c')
		{
			char param = va_arg(arp, int);

			gecko_putc(param);
			pos++;

			continue;
		}

		r = 0;

		/* Type is signed decimal */
		if (c == 'd')
			r = 10;

		/* Type is unsigned decimal */
		if (c == 'u')
			r = 10;

		/* [nitr8]: Add a case for checking pointers ('p') */
		/* Type is unsigned hexdecimal */
		if (c == 'p' || c == 'x')
			r = 16;

		/* Unknown type */
		if (r == 0)
			break;

		val = (c == 'd') ? (u32)(long)va_arg(arp, int) : 
				(u32)va_arg(arp, unsigned int);

		/* Put numeral string */
		if (c == 'd')
		{
			if (val & 0x80000000)
			{
				val = 0 - val;
				f |= 4;
			}
		}

 		i = sizeof(s) - 1;
		s[i] = 0;

		do
		{
			c = (u8)(val % r + '0');

			if (c > '9')
				c += 7;

			s[--i] = c;
			val /= r;
		} while (i && val);

		if (i && (f & 4))
			s[--i] = '-';

		w = sizeof(s) - 1 - w;

		while (i && i > w)
			s[--i] = (f & 1) ? '0' : ' ';

		for (; s[i] ; i++)
		{
			gecko_putc(s[i]);
			pos++;
		}
	}

	va_end(arp);

	return pos;
}

#else

int gecko_printf(const char *fmt, ...)
{
	va_list args;
	char buffer[256];
	int i;

	if (!gecko_console_enabled)
		return 0;

	va_start(args, fmt);
	i = vsprintf(buffer, fmt, args);
	va_end(args);
#ifdef GECKO_SAFE
	return gecko_sendbuffer_safe(buffer, i);
#else
	return gecko_sendbuffer(buffer, i);	
#endif
}

#endif

/* [nitr8]: Add support for output to a specific EXI port */
void gecko_init(int port)
{
/* [nitr8]: Not working yet... */
#ifndef SCREEN_TO_CONSOLE_OUTPUT
	UNUSED(port);
#else

/* [nitr8]: Not working yet... */
#ifdef GECKO_SAFE
	int safe = 1;
#else
	int safe = 0;
#endif

#endif

	/* [nitr8]: Return early if the console isn't enabled at all */
	if (gecko_console_enabled)
		return;

	write32(EXI0_CSR, 0);
	write32(EXI1_CSR, 0);
	write32(EXI2_CSR, 0);
	write32(EXI0_CSR, 0x2000);
	write32(EXI0_CSR, 3<<10);
	write32(EXI1_CSR, 3<<10);

	if (!gecko_isalive())
		return;

	gecko_found = 1;

	gecko_flush();
	gecko_console_enabled = 1;

/* [nitr8]: Not working yet... */
#ifdef SCREEN_TO_CONSOLE_OUTPUT
	CON_EnableGecko(port, safe);
#endif
}

