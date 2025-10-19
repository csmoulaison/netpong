mkdir ../bin
cp -r fonts ../bin/fonts
cp -r ../src/shaders ../bin/shaders

g++ -g -o ../bin/netpong \
	../src/game/client/main.cpp ../src/platform/xlib/xlib_media.cpp ../src/network/unix/unix_network.cpp ../src/time/unix/unix_time.cpp ../src/renderer/opengl/opengl.cpp \
	../src/renderer/opengl/GL/gl3w.c \
	-I ../src/ \
	-lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes
