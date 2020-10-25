#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#define _XOPEN_SOURCE 700

/*
 * daemonize.h
 * simply daemonize your program
 * version 1.1.2
 * (c) 2020 kocotian <kocotian@kocotian.pl>
 *
 * for copyright details, see
 * git.kocotian.pl/daemonize.h/blob/main/LICENSE
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

static int
daemonize(char *chdir_, int umask_, void (*signalize)(void))
{
	pid_t pid;
	int fd;

	if ((pid = fork()) < 0)
		return EXIT_FAILURE;
	if (pid > 0)
		exit(EXIT_SUCCESS);

	if (setsid() < 0)
		return EXIT_FAILURE;

	signalize();

	if ((pid = fork()) < 0)
		return EXIT_FAILURE;
	if (pid > 0)
		exit(EXIT_SUCCESS);

	chdir(chdir_);
	umask(umask_);

	fd = sysconf(_SC_OPEN_MAX) + 1;
	while (fd-- > 0)
		close(fd);

	return EXIT_SUCCESS;
}

#endif
