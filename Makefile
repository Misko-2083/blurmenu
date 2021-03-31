CFLAGS=-Wall -g

blurmenu: blurmenu.c cairo.c stackblur.c select.c
	$(CC) $(CFLAGS) -o $@ $^ `pkg-config --cflags --libs x11 xrender xrandr cairo pango pangocairo` -lpthread

clean:
	@rm -f blurmenu
