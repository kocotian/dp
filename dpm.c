#define _XOPEN_SOURCE 700

/*
 * dpm - don't panic manager
 * more user-friendly program
 * that manages a don't panic daemon
 *
 * Copyright (c) 2020 by:
 * - Karolina Nocek
 * - Wojciech Tomasik
 * - Kacper Kocot
 *
 * For copying, redistribution, etc,
 * see LICENSE file.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tbkeys.h"
#include "termbox/termbox.h"
#include "termbox/termplus.h"
#include "util.c"

#define DPM

typedef struct {
	int time;
	char *name;
} Module;

static int parsecsv(char *csvpath);
static int savecsv(char *csvpath);
static int drawmenu(pid_t dppid, int selitem);
static pid_t getdppid(void);

static Module *mods;
static volatile int modlen;

#include "config.h"

static int
drawmenu(pid_t dppid, int selitem)
{
	int iter;

	tb_clear();
	tb_printf("Status DP: ", 1, 1, TB_WHITE, TB_DEFAULT);
	if (dppid < 0)
		tb_printf("zatrzymany", 12, 1, TB_RED | TB_BOLD, TB_DEFAULT);
	else
		tb_printf("pracuje (PID: %d)", 12, 1, TB_GREEN | TB_BOLD, TB_DEFAULT,
				dppid);

	iter = -1;
	while (++iter < modlen) {
		tb_printf(iter == selitem ? "=>" : " *", 1, iter + 3,
				iter == selitem ? COLOR_ACCENT | TB_BOLD : TB_WHITE, TB_DEFAULT);
		tb_printf("co %2d minut", 4, iter + 3, COLOR_ACCENT | TB_BOLD, TB_DEFAULT,
				mods[iter].time);
		tb_printf("wykonywanie",   16, iter + 3, TB_WHITE | TB_BOLD, TB_DEFAULT);
		tb_printf("%s",            28, iter + 3, TB_WHITE, TB_DEFAULT,
				mods[iter].name);
	};
	tb_printf("Wciśnij ", 1, tb_height() - 4, COLOR_PRIMARY, TB_DEFAULT);
	tb_printf("t", 9, tb_height() - 4, COLOR_ACCENT | TB_BOLD, TB_DEFAULT);
	tb_printf("aby wyłączyć DP", 11, tb_height() - 4, COLOR_PRIMARY, TB_DEFAULT);
	tb_printf("Wciśnij ", 1, tb_height() - 3, COLOR_PRIMARY, TB_DEFAULT);
	tb_printf("s", 9, tb_height() - 3, COLOR_ACCENT | TB_BOLD, TB_DEFAULT);
	tb_printf("aby zapisać", 11, tb_height() - 3, COLOR_PRIMARY, TB_DEFAULT);
	tb_printf("Wciśnij ", 1, tb_height() - 2, COLOR_PRIMARY, TB_DEFAULT);
	tb_printf("q", 9, tb_height() - 2, COLOR_ACCENT | TB_BOLD, TB_DEFAULT);
	tb_printf("aby wyjść", 11, tb_height() - 2, COLOR_PRIMARY, TB_DEFAULT);
	tb_drawbox(0, 0, tb_width() - 1, tb_height() - 1, COLOR_PRIMARY, TB_DEFAULT);
}

static int
parsecsv(char *csvpath)
{
	FILE *csvf;
	int modlen = 0, iter = -1;
	char buf[256]; char *tok;
	free(mods);
	mods = malloc(sizeof(Module));
	if ((csvf = fopen(csvpath, "r")) == NULL) {
		tb_shutdown();
		perror(csvpath);
		exit(EXIT_FAILURE);
	}
	while (fgets(buf, 256, csvf) != NULL) {
		tok = strtok(buf, ",");
		mods[++iter].time = atoi(tok);
		tok = strtok(NULL, ",");
		mods[iter].name = malloc(strlen(tok));
		strncpy(mods[iter].name, tok, strlen(tok));
		mods = realloc(mods, sizeof(Module) * (++modlen + 1));
	}
	fclose(csvf);
	return modlen;
}

static int
savecsv(char *csvpath)
{
	FILE *csvf;
	int iter = -1;
	if ((csvf = fopen(csvpath, "w")) == NULL) {
		tb_shutdown();
		perror(csvpath);
		exit(EXIT_FAILURE);
	}
	while (++iter < modlen) {
		if (mods[iter].name != NULL)
			fprintf(csvf, "%d,%s",
					mods[iter].time, mods[iter].name);
	}
	fclose(csvf);
	return modlen;
}

static pid_t
getdppid(void)
{
	struct stat dpstat;
	FILE *dppidf;
	pid_t dppid;

	if (stat(pidpath, &dpstat) < 0)
		dppid = -1;
	else {
		if ((dppidf = fopen(pidpath, "r")) == NULL)
			die("Błąd odczytu pliku zawierającego PID DP");
		fscanf(dppidf, "%d", &dppid);
		fclose(dppidf);
	}
	return dppid;
}

int
main(int argc, char *argv[])
{
	pid_t dppid;
	char selitem = 0;
	struct tb_event ev;

	dppid = getdppid();

	chdir(pwdir);
	modlen = parsecsv(csvpath);

	if (tb_init() < 0)
		die("Inicjalizacja interfejsu zakończona niepowodzeniem");

	tb_select_input_mode(TB_INPUT_ESC);
	tb_select_output_mode(TB_OUTPUT_NORMAL);

	if (tb_width() < 48 || tb_height() < 16)
		tb_die("Terminal jest zbyt mały");

	drawmenu(dppid, selitem);
	tb_present();

	while (tb_poll_event(&ev)) {
		if (ev.ch == 'q' || ev.key == TB_KEY_ESC) {
			tb_shutdown();
			return 0;
		} else if (ev.ch == 'j' || ev.key == TB_KEY_ARROW_DOWN) {
			if (selitem < modlen - 1)
				++selitem;
		} else if (ev.ch == 'k' || ev.key == TB_KEY_ARROW_UP) {
			if (selitem > 0)
				--selitem;
		} else if (ev.ch == 'h' || ev.key == TB_KEY_ARROW_LEFT) {
			if (mods[selitem].time > 1)
				--(mods[selitem].time);
		} else if (ev.ch == 'l' || ev.key == TB_KEY_ARROW_RIGHT) {
			if (mods[selitem].time < 60)
				++(mods[selitem].time);
		} else if (ev.ch == 's') {
			savecsv(csvpath);
			if (dppid != -1)
				kill(dppid, SIGUSR1);
		} else if (ev.ch == 't') {
			if (!(dppid < 0)) {
				kill(dppid, SIGINT);
				dppid = getdppid();
			}
		}
		drawmenu(dppid, selitem);
		tb_present();
	}
}
