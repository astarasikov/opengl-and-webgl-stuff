APPNAME=gl3
CC=gcc
CFLAGS=-pg
LDFLAGS=-framework Cocoa \
	-framework CoreVideo \
	-framework OpenGL

CFILES = \
	gl3.m \
	inset_gl_controller.m \
	opengl_view.m

OBJFILES=$(patsubst %.m,%.o,$(CFILES))

all: $(APPNAME)

$(APPNAME): $(OBJFILES)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJFILES)

$(OBJFILES): %.o: %.m
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(APPNAME)
	rm *.o

run:
	make clean
	make all
	cp $(APPNAME) $(APPNAME).app/Contents/MacOS/$(APPNAME)
	#open $(APPNAME).app
	./$(APPNAME)
