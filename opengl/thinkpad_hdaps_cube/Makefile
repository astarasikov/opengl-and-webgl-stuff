APPNAME=gl_tests
CC ?= gcc
CFLAGS=-std=c99 -Wall -pg -lGL -lGLU -lglut

CFILES = gl_tests.c
OBJFILES = $(patsubst %.c,%.o,$(CFILES))

all: $(APPNAME)

$(APPNAME): $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $(OBJFILES)

$(OBJFILES): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(APPNAME)
	rm -f *.o
