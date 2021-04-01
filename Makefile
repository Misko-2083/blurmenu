CFLAGS  ?=-Wall -g -std=c11 -Wextra -Wno-sign-compare
CFLAGS  +=`pkg-config --cflags x11 xrender xrandr cairo pango pangocairo`
LDFLAGS +=`pkg-config --libs x11 xrender xrandr cairo pango pangocairo`
LDFLAGS +=-lpthread

# Run `make ASAN=1` to build with address,undefined sanitizer
ifdef ASAN
ASAN_FLAGS = -O0 -fsanitize=address,undefined -fno-common -fno-omit-frame-pointer -rdynamic
CFLAGS  += $(ASAN_FLAGS)
LDFLAGS += $(ASAN_FLAGS) -fuse-ld=gold
endif

blurmenu: blurmenu.o cairo.o select.o stackblur.o x11.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	@rm -f *.o blurmenu
