
all:	gneutronica	

gneutronica:	gneutronica.c sched.o
	gcc -g -o gneutronica -I/usr/include/libgnomecanvas-2.0 sched.o gneutronica.c `pkg-config --cflags --libs gtk+-2.0` 

sched.o:	sched.c sched.h		

install:	gneutronica
	@echo "Installing /usr/local/bin/gneutronica and /usr/local/share/gneutronica/drumkits"
	@cp gneutronica /usr/local/bin/gneutronica
	@chmod +x /usr/local/bin/gneutronica
	@mkdir -p /usr/local/share/gneutronica/drumkits
	@cp drumkits/*.gdk /usr/local/share/gneutronica/drumkits 

uninstall:
	/bin/rm -f /usr/local/bin/gneutronica
	/bin/rm -fr /usr/local/share/gneutronica/

clean:
	/bin/rm -f gneutronica *.o
