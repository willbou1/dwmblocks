PREFIX ?= /usr/local
CC ?= cc

output: dwmblocks.c blocks.def.h blocks.h
	${CC} dwmblocks.c -o dwmblocks -lX11
blocks.h:
	cp blocks.def.h $@


clean:
	rm -f *.o *.gch dwmblocks
install: output
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 0755 dwmblocks $(DESTDIR)$(PREFIX)/bin/dwmblocks
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks
