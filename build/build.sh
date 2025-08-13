gcc -o ../bin/2d_prototyper \
	../src/main.c ../src/base/base.c ../src/game/game.c ../src/platforms/xlib/xlib.c ../src/renderers/opengl/opengl.c \
	../src/renderers/opengl/GL/gl3w.c \
	-I ../src/ \
	-lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes
