APPNAME=test
CC=g++
CFLAGS=-pg -O2 -g2 -Wall $(shell pkg-config --libs --cflags glfw3 glew)
#LDFLAGS=-lGL
LDFLAGS=-framework OpenGL

CFILES = test.cc

OBJFILES=$(patsubst %.cc,%.o,$(CFILES))

all: $(APPNAME)

$(APPNAME): $(OBJFILES)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJFILES)

$(OBJFILES): %.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(APPNAME) *.o || true

run:
	make clean
	make all
	rm ./out.bin ./out.png || true
	./$(APPNAME)
	ffmpeg -vcodec rawvideo -f rawvideo -pix_fmt rgb24 -s 1024x768 -i out.bin -f image2 -pix_fmt rgb24 out.png || true
	open ./out.png
