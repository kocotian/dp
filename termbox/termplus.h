#include <stdarg.h>
#include <stdio.h>

#include "termbox.h"

void tb_puts(const char *str, int x, int y, uint32_t fg, uint32_t bg);
void tb_printf(const char *str, int x, int y, uint32_t fg, uint32_t bg, ...);
int tb_drawbox(int ax, int ay, int bx, int by, uint32_t fg, uint32_t bg);
void tb_die(const char *fmt, ...);

void
tb_puts(const char *str, int x, int y, uint32_t fg, uint32_t bg)
/* puts str string, first letter will be at x, y and color will be fg & bg */
{
	while (*str) {
		uint32_t uni;
		str += utf8_char_to_unicode(&uni, str);
		tb_change_cell(x, y, uni, fg, bg);
		x++;
	}
}

void
tb_printf(const char *format, int x, int y, uint32_t fg, uint32_t bg, ...)
/* same as puts, but with format at the end (uses puts btw) */
{
	va_list data;
	char buf[BUFSIZ];
	va_start(data, bg);
	vsnprintf(buf, BUFSIZ, format, data);
	va_end(data);
	tb_puts(buf, x, y, fg, bg);
}

int
tb_drawbox(int ax, int ay, int bx, int by, uint32_t fg, uint32_t bg)
/* draws box from ax, ay to bx, by with fg and bg color */
{
	int iter;

	if(ax >= bx || ay >= by) return -1;

	tb_change_cell(ax, ay, 0x250C, fg, bg);
	tb_change_cell(bx, ay, 0x2510, fg, bg);
	tb_change_cell(ax, by, 0x2514, fg, bg);
	tb_change_cell(bx, by, 0x2518, fg, bg);

	iter = ax;
	while(++iter < bx)
	{
		tb_change_cell(iter, ay, 0x2500, fg, bg);
		tb_change_cell(iter, by, 0x2500, fg, bg);
	}

	iter = ay;
	while(++iter < by)
	{
		tb_change_cell(ax, iter, 0x2502, fg, bg);
		tb_change_cell(bx, iter, 0x2502, fg, bg);
	}

	tb_present();
	return 0;
}

void
tb_die(const char *fmt, ...)
/* based on suckless' libsl die()
   git.suckless.org/libsl
   see /LICENSE_LIBSL for copyright details */
{
	va_list ap;
	tb_shutdown();

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if(fmt[0] && fmt[strlen(fmt) - 1] == ':')
	{
		fputc(' ', stderr);
		perror(NULL);
	}
	else
	{
		fputc('\n', stderr);
	}

	exit(1);
}
