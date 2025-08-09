gcc -o ../bin/proto \
	../src/main.c ../src/base/base.c ../src/game/game.c ../src/platforms/xlib/xlib.c ../src/renderers/opengl/opengl.c \
	-I ../src/
