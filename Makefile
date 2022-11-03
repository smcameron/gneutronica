CC=gcc
PROGRAM=gneutronica
BINDIR=/usr/local/bin
SHAREDIR=/usr/local/share/${PROGRAM}
# DEBUG=-g
DEBUG=
OPTIMIZE=-O3
GNEUTRONICA_LANGUAGE=
# GNEUTRONICA_LANGUAGE=-DGNEUTRONICA_FRENCH

CFLAGS=${DEBUG} ${OPTIMIZE} ${GNEUTRONICA_LANGUAGE} -Wall -Wextra --pedantic \
	-D_FORTIFY_SOURCE=2 -fsanitize=bounds -Wstringop-truncation -Warray-bounds \
	-Wstringop-overflow -fstack-protector-strong -Wvla \
	-Wimplicit-fallthrough

all:	gneutronica documentation/gneutronica.1

write_bytes.o:	write_bytes.c write_bytes.h
	$(CC) ${CFLAGS} -c write_bytes.c

midioutput_raw.o:	midioutput_raw.c midioutput_raw.h midioutput.h
	$(CC) ${CFLAGS} -c midioutput_raw.c

midioutput_alsa.o:	midioutput_alsa.c midioutput_alsa.h midioutput.h
	$(CC) ${CFLAGS} -c midioutput_alsa.c

drumtab.o:	drumtab.c drumtab.h dt_known_insts.h
	$(CC) ${CFLAGS} -c drumtab.c

fractions.o:	fractions.c fractions.h
	$(CC) ${CFLAGS} -c fractions.c

documentation/gneutronica.1:	documentation/gneutronica.1.template versionnumber.txt
	chmod +x ./make_manpage
	./make_manpage > documentation/gneutronica.1

version.h:	versionnumber.txt
	@echo '#define VERSION "'`cat versionnumber.txt`'"' > version.h

gneutronica:	gneutronica.c old_fileformats.o sched.o midi_file.o \
		version.h gneutronica.h midi_file.h fractions.o drumtab.o \
		midi_reader.o midioutput_raw.o midioutput_alsa.o \
		midioutput_raw.h midioutput_alsa.h lang.h write_bytes.o
	$(CC) ${CFLAGS} -o gneutronica -I/usr/include/libgnomecanvas-2.0 -lasound \
		old_fileformats.o sched.o \
		midi_reader.o midi_file.o fractions.o drumtab.o \
		midioutput_raw.o midioutput_alsa.o write_bytes.o \
		gneutronica.c `pkg-config --cflags --libs gtk+-2.0` \
		-lasound -lm

midi_reader.o:	midi_reader.c midi_reader.h

sched.o:	sched.c sched.h	midi_file.h

midi_file.o:	midi_file.c midi_file.h

old_fileformats.o:	old_fileformats.c old_fileformats.h gneutronica.h
		$(CC) ${CFLAGS} -c `pkg-config --cflags gtk+-2.0` old_fileformats.c

install:	gneutronica
	echo "Installing ${BINDIR}/${PROGRAM} and ${SHAREDIR}/drumkits"
	cp gneutronica ${BINDIR}/${PROGRAM} 
	chmod +x ${BINDIR}/${PROGRAM}
	mkdir -p ${SHAREDIR}/drumkits
	cp drumkits/*.dk ${SHAREDIR}/drumkits 
	mkdir -p ${SHAREDIR}/documentation
	cp documentation/gneutronica.html ${SHAREDIR}/documentation 
	cp documentation/*.png ${SHAREDIR}/documentation
	@if [ -d /usr/share/man/man1 ] ; then \
		echo "Installing gneutronica man page." ; \
		gzip < documentation/gneutronica.1 > /usr/share/man/man1/gneutronica.1.gz ; \
	fi
	@if [ -d /usr/share/pixmaps ] ; then \
		echo "Installing gneutronica icon in /usr/share/pixmaps" ; \
		cp -f icons/gneutronica_icon.png /usr/share/pixmaps/gneutronica_icon.png ; \
	fi
	

uninstall:
	/bin/rm -f ${BINDIR}/${PROGRAM}
	/bin/rm -fr ${SHAREDIR} 
	/bin/rm -f /usr/share/man/man1/gneutronica.1.gz
	/bin/rm -f /usr/share/pixmaps/gneutronica_icon.png

clean:
	/bin/rm -f gneutronica *.o documentation/gneutronica.1 version.h drumtab

scan-build:
	make clean
	rm -fr /tmp/snis-scan-build-output
	scan-build -o /tmp/snis-scan-build-output make CC=clang
	xdg-open /tmp/snis-scan-build-output/*/index.html

