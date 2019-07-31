CC=g++
bin=Sosuke
src=Sosuke.cc
INCLUDE=-Ispeech/
LIB=-ljsoncpp -lcurl -lcrypto -lpthread
FLAGS=-D_DEBUG_
STD=-std=c++11

$(bin):$(src)
	$(CC) -o $@ $^ $(INCLUDE) $(LIB) $(STD) $(FLAGS)
.PHONY:clean
clean:
	rm -f $(bin)
