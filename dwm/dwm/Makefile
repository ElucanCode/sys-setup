# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.c dwm.c util.c
OBJ = ${SRC:.c=.o}

all: dwm

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

dwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f dwm ${OBJ} dwm-${VERSION}.tar.gz config.h

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwm ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm
	cp -f dwm-cmd ${DESTDIR}${PREFIX}/bin/dwm-cmd
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm-cmd
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < dwm.1 > ${DESTDIR}${MANPREFIX}/man1/dwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm.1
	cp -f ./dwm.desktop /usr/share/xsessions/dwm-${VERSION}.desktop

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm\
		${DESTDIR}${PREFIX}/bin/_dwm-cmd\
		${DESTDIR}${MANPREFIX}/man1/dwm.1\
		/usr/share/xsessions/dwm-${VERSION}.desktop

.PHONY: all clean install uninstall
