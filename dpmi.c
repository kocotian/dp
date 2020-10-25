#define _XOPEN_SOURCE 700

/*
 * dpmi - don't panic module installer
 * tool for adding optional modules
 * for a don't panic daemon
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

#include "arg.h"
#include "http.h"
#include "util.c"

#define DPMI

typedef struct {
	int time;
	char *name;
} Module;

static void getmodule(char *argv[]);
static void removemodule(char *argv[]);
static int parsecsv(char *csvpath);
static int savecsv(char *csvpath);

static Module *mods;
static volatile int modlen;
char *argv0;

#include "config.h"

static void
getmodule(char *argv[])
{
	char *response, modpath[BUFSIZ];
	size_t responseSize;
	int iter = -1, uiter;
	FILE *modf;

	while (argv[++iter] != NULL) {
		printf("\033[1;34m:: \033[0mPobieram moduł \033[1m%s\033[0m...\n",
				argv[iter]);
		responseSize = httpGET("dp.kocotian.pl", 80, "/dl/", argv[iter], &response);
		if (getResponseStatus(response, responseSize) != 200) {
			udie("Wystąpił błąd \033[1m%d\033[0m",
					getResponseStatus(response, responseSize));
		}
		printf("\033[1;34m:: \033[0mOdebrano \033[1m%s\033[0m, zapisywanie...\n",
				argv[iter]);
		snprintf(modpath, 256, "%s/modules/%s",
				pwdir, argv[iter]);
		if((modf = fopen(modpath, "w")) == NULL)
			udie("Nie udało się otworzyć pliku do zapisu:");
		fprintf(modf, "%s", truncateHeader(response));
		chmod(modpath, 0755);
		fclose(modf);
		mods = realloc(mods, sizeof(Module) * (++modlen + 1));
		mods[modlen - 1].time = 60;
		mods[modlen - 1].name = malloc(strlen(argv[iter]) + 9);
		strcpy(mods[modlen - 1].name, "modules/");
		strcat(mods[modlen - 1].name, argv[iter]);
		strcat(mods[modlen - 1].name, "\n");
		uiter = -1;
		while (++uiter < modlen - 1) {
			if (mods[uiter].name != NULL)
				if (!strcmp(mods[modlen - 1].name, mods[uiter].name)) {
					mods[modlen - 1].name[strlen(mods[modlen - 1].name) - 1] = '\0';
					printf("\033[1;33m==> \033[0mModuł \033[1m%s \033[0mbył już zainstalowany, tylko aktualizuję.\n",
							mods[modlen - 1].name);
					mods[modlen - 1].name = NULL;
					break;
				}
		}
		free(response);
		savecsv(csvpath);
	}
}

static void
removemodule(char *argv[])
{
	char *buf, modpath[256], isRemoved = 0;
	int iter = -1, uiter;

	while (argv[++iter] != NULL) {
		printf("\033[1;34m:: \033[0mUsuwam moduł \033[1m%s\033[0m...\n",
				argv[iter]);
		snprintf(modpath, 256, "%s/modules/%s",
				pwdir, argv[iter]);
		buf = malloc(strlen(argv[iter]) + 9);
		strcpy(buf, "modules/");
		strcat(buf, argv[iter]);
		strcat(buf, "\n");
		uiter = -1;
		while (++uiter < modlen) {
			if (mods[uiter].name != NULL)
				if (!strcmp(buf, mods[uiter].name)) {
					if (unlink(modpath) < 0)
						udie("Nie udało się usunąć modułu %s:",
								argv[iter]);
					mods[uiter].name = NULL;
					isRemoved = 1;
					break;
				}
		}
		if(!isRemoved)
			udie("Moduł \033[1m%s\033[0m nie jest zainstalowany",
					argv[iter]);
		free(buf);
		savecsv(csvpath);
	}
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

static void
usage(void)
{
	udie("użycie: %s [-g STR] [-r STR]\n    wpisz \"man dpmi\" po więcej informacji",
			argv0);
}

int
main(int argc, char *argv[])
{
	struct stat dpstat;
	char flag = 0;
	FILE *dppidf;
	pid_t dppid;

	ARGBEGIN {
	case 'g': case 'r': /* fallthrough */
		flag = ARGC();
		break;
	default:
		usage();
		break;
	} ARGEND;

	if (stat(pidpath, &dpstat) < 0)
		dppid = -1;
	else {
		if ((dppidf = fopen(pidpath, "r")) == NULL)
			udie("Błąd odczytu pliku zawierającego PID DP");
		fscanf(dppidf, "%d", &dppid);
		fclose(dppidf);
	}

	chdir(pwdir);
	modlen = parsecsv(csvpath);

	switch (flag) {
	case 'g':
		getmodule(argv);
		break;
	case 'r':
		removemodule(argv);
		break;
	}
	if (dppid != -1) {
		printf("\033[1;34m:: \033[0mOdświeżam DP...\n");
		kill(dppid, SIGUSR1);
	}
	printf("\033[1;32m:: \033[0mZakończono...\n");
	return 0;
}
