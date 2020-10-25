CC="gcc"
DESTDIR="/usr/local/"
BINDIR="bin"
DPDIR="share/dp"
MANDIR="man"

dp: dp.c config.h daemonize.h
	${CC} -o $@ $< -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -pedantic -std=c99

dpm: dpm.c config.h
	${CC} -o $@ $< termbox/bin/termbox.a -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -pedantic -std=c99

dpmi: dpmi.c config.h
	${CC} -o $@ $< -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -pedantic -std=c99

all: dp dpm dpmi

install: all
	mkdir -p ${DESTDIR}${BINDIR} ${DESTDIR}${DPDIR} ${DESTDIR}${MANDIR}/man1
	install -Dm 755 dp ${DESTDIR}${BINDIR}
	install -Dm 755 dpm ${DESTDIR}${BINDIR}
	install -Dm 755 dpmi ${DESTDIR}${BINDIR}
	cp -f dp.1 dpmi.1 ${DESTDIR}${MANDIR}/man1/
	dpmi -g fortune sentences covid
