g++ -g -o ../bin/server \
	../src/game/server/main.cpp ../src/network/unix/network_unix.cpp ../src/platform/xlib/xlib_time.cpp \
	-I ../src/ \
