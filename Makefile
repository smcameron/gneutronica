
PROGRAM=gneutronica
BINDIR=/usr/local/bin
SHAREDIR=/usr/local/share/${PROGRAM}

all:	gneutronica documentation/gneutronica.1 drumtab


fractions.o:	fractions.c fractions.h
	gcc -g -c fractions.c

drumtab:	drumtab.c fractions.o
	gcc -g -o drumtab drumtab.c fractions.o

documentation/gneutronica.1:	documentation/gneutronica.1.template versionnumber.txt
	chmod +x ./make_manpage
	./make_manpage > documentation/gneutronica.1

version.h:	versionnumber.txt
	@echo '#define VERSION "'`cat versionnumber.txt`'"' > version.h

gneutronica:	gneutronica.c old_fileformats.o sched.o midi_file.o \
		version.h gneutronica.h midi_file.h fractions.o
	gcc -g -o gneutronica -I/usr/include/libgnomecanvas-2.0 old_fileformats.o sched.o \
		midi_file.o fractions.o gneutronica.c `pkg-config --cflags --libs gtk+-2.0`

sched.o:	sched.c sched.h	midi_file.h

midi_file.o:	midi_file.c midi_file.h

old_fileformats.o:	old_fileformats.c old_fileformats.h gneutronica.h
		gcc -g -c `pkg-config --cflags gtk+-2.0` old_fileformats.c

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
