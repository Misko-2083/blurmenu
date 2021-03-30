CFLAGS=-Wall -g

blurmenu: blurmenu.c
	$(CC) $(CFLAGS) -o $@ $^ `pkg-config --cflags --libs x11 xrender cairo` -lpthread

clean:
	@rm -f blurmenu
