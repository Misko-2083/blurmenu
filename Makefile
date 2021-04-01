CFLAGS  ?=-Wall -g -std=c11 -Wextra -Wno-sign-compare
CFLAGS  +=`pkg-config --cflags x11 xrender xrandr cairo pango pangocairo`
LDFLAGS +=`pkg-config --libs x11 xrender xrandr cairo pango pangocairo`
LDFLAGS +=-lpthread

blurmenu: blurmenu.o cairo.o select.o stackblur.o x11.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	@rm -f *.o blurmenu
