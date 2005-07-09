
PROGRAM=gneutronica
BINDIR=/usr/local/bin
SHAREDIR=/usr/local/share/${PROGRAM}

all:	gneutronica	

gneutronica:	gneutronica.c sched.o
	gcc -g -o gneutronica -I/usr/include/libgnomecanvas-2.0 sched.o gneutronica.c `pkg-config --cflags --libs gtk+-2.0` 

sched.o:	sched.c sched.h		

install:	gneutronica
	echo "Installing ${BINDIR}/${PROGRAM} and ${SHAREDIR}/drumkits"
	cp gneutronica ${BINDIR}/${PROGRAM} 
	chmod +x ${BINDIR}/${PROGRAM}
	mkdir -p ${SHAREDIR}/drumkits
	cp drumkits/*.gdk drumkits/*.dk ${SHAREDIR}/drumkits 

uninstall:
	/bin/rm -f ${BINDIR}/${PROGRAM}
	/bin/rm -fr ${SHAREDIR} 

clean:
	/bin/rm -f gneutronica *.o
