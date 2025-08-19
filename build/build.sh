g++ -g -o ../bin/2d_prototyper \
	../src/main.cpp ../src/base/base.cpp ../src/game/game.cpp ../src/platforms/xlib/xlib.cpp ../src/renderers/opengl/opengl.cpp \
	../src/renderers/opengl/GL/gl3w.c \
	-I ../src/ \
	-lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes
