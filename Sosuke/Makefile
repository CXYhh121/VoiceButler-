CC=g++
bin=Jarvis
src=Jarvis.cc
INCLUDE=-Ispeech/
LIB=-ljsoncpp -lcurl -lcrypto -lpthread
STD=-std=c++11

$(bin):$(src)
	$(CC) -o $@ $^ $(INCLUDE) $(LIB) $(STD)
.PHONY:clean
clean:
	rm -f $(bin)
