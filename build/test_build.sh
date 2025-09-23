cp -r fonts ../bin/
cp -r ../src/shaders ../bin/

g++ -g -o ../bin/test \
	../src/test/main.cpp ../src/platform/xlib/xlib_media.cpp ../src/platform/xlib/xlib_network.cpp ../src/platform/xlib/xlib_time.cpp ../src/renderer/opengl/opengl.cpp \
	../src/renderer/opengl/GL/gl3w.c \
	-I ../src/ \
	-lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes
