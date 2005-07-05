
all:	gneutronica	

gneutronica:	gneutronica.c sched.o
	gcc -g -o gneutronica -I/usr/include/libgnomecanvas-2.0 sched.o gneutronica.c `pkg-config --cflags --libs gtk+-2.0` 

sched.o:	sched.c sched.h		

clean:
	/bin/rm -f gneutronica *.o
