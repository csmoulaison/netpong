g++ -g -o ../bin/server \
	../src/game/server/main.cpp ../src/network/unix/unix_network.cpp ../src/time/unix/unix_time.cpp \
	-I ../src/ \
