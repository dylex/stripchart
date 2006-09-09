PKGS=libgnomeui-2.0
CFLAGS = -g -Wall $(shell pkg-config $(PKGS) --cflags) -DGTK_ENABLE_BROKEN
LDFLAGS=$(shell pkg-config $(PKGS) --libs)

stripchart: stripchart.o chart-app.o prefs.o utils.o params.o strip.o chart.o eval.o
	$(CC) $(LDFLAGS) -o $@ $^

Makefile.dep: *.c
	$(CC) -MM $(CFLAGS) $^ > Makefile.dep

$(HOME)/bin/stripchart: stripchart
	install $< $@

install: $(HOME)/bin/stripchart

clean: 
	rm -f *.o stripchart

include Makefile.dep
