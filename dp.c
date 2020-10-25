#define _XOPEN_SOURCE 700

/*
 * dp - don't panic
 * simple unix-philosophy wrote
 * modular daemon that shows notifications
 * received from healthy modules
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

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "daemonize.h"
#include "util.c"

#define DP
#define UMASK 0022
#define SHELL(cmd) {.v = (const char*[]){"/bin/sh", "-c", cmd, NULL}}
#define SHNOTIFY(progname) {.v = (const char *)progname}

typedef union {
	int i;
	unsigned int ui;
	float f;
	void *v;
} Arg;

typedef struct {
	int time;
	char *name;
} Module;

typedef struct {
	int time;
	void (*func)(const Arg *);
	Arg arg;
} Command;

void notifySend(char *notify);
void shellnotify(const Arg *arg);
void run(const Arg *arg);
int  parsecsv(char *csvpath);
void sighandler(int signo);
void signalize(void);

static Command *cmd = NULL;
static volatile int cmdlen = 0;
static int daemonized = 0;

#include "config.h"

void
notifySend(char *notify)
{
	char str[BUFSIZ];
	snprintf(str, BUFSIZ, "%s \"%s\"",
			notifycmd, notify);
	Arg argnotify = SHELL(str);
	run(&argnotify);
}

void
shellnotify(const Arg *arg)
{
	/* (+ 4) because: '"$(' - 2 chars, ')"' - 1 chars, plus NULL at end */
	char *command = malloc(strlen(arg -> v) + 4);
	snprintf(command, strlen(arg -> v) + 4, "$(%s)",
			(arg -> v));
	notifySend(command);
	free(command);
}

void
run(const Arg *arg)
{
    if (!fork()) {
		setsid();
        execvp(((char **)arg -> v)[0], (char **)arg -> v);
		syslog(LOG_PERROR, "execvp failed");
        unlink(pidpath);
        exit(EXIT_FAILURE);
    }
}

int
parsecsv(char *csvpath)
{
	FILE *csvf;
	int cmdlen = 0, iter = -1;
	char buf[256]; char *tok;
	free(cmd);
	cmd = malloc(sizeof(Command));
	if ((csvf = fopen(csvpath, "r")) == NULL) {
		if (!daemonized) perror(csvpath);
		exit(EXIT_FAILURE);
	}
	while (fgets(buf, 256, csvf) != NULL) {
		tok = strtok(buf, ",");
		cmd[++iter].time = atoi(tok);
		cmd[iter].func = shellnotify;
		tok = strtok(NULL, ",");
		cmd[iter].arg.v = malloc(strlen(tok));
		strncpy(cmd[iter].arg.v, tok, strlen(tok));
		cmd = realloc(cmd, sizeof(Command) * (++cmdlen + 1));
	}
	fclose(csvf);
	return cmdlen;
}

void
sighandler(int signo)
{
	signal(signo, sighandler);

    switch (signo) {
	case SIGINT:
		unlink(pidpath);
		free(cmd);
		exit(EXIT_SUCCESS);
		break;
	case SIGUSR1:
		cmdlen = parsecsv(csvpath);
		break;
    }
}

void
signalize(void)
{
	signal(SIGINT, sighandler);
	signal(SIGUSR1, sighandler);
}

int
main(void)
{
	struct stat statvar;
	FILE *pidf = NULL;
	char iter = -1;
	time_t t;

	if (!(stat(pidpath, &statvar) < 0))
		die("another dp process is running");

	if (stat(pwdir, &statvar) < 0)
		die("%s doesn't exist", pwdir);

	cmdlen = parsecsv(csvpath);
	if (!(daemonized = !daemonize(pwdir, UMASK, signalize)))
		die("failed to daemonize");

	if ((pidf = fopen(pidpath, "w")) == NULL)
		exit(EXIT_FAILURE);

	fprintf(pidf, "%d\n", getpid());
	fclose(pidf);

	while (1) {
		t = time(NULL);
		while (++iter < cmdlen)
            if (cmd[iter].time) {
                if (!((t / 60 % 60) % cmd[iter].time))
                    cmd[iter].func(&(cmd[iter].arg));
            }
            else
				kill(getpid(), SIGINT);
		iter = -1;
		sleep(60 - t % 60);
	}
	return 0;
}
