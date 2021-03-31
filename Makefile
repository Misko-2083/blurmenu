CFLAGS=-Wall -g

blurmenu: blurmenu.c stackblur.c select.c
	$(CC) $(CFLAGS) -o $@ $^ `pkg-config --cflags --libs x11 xrender cairo` -lpthread

clean:
	@rm -f blurmenu
