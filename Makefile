TARGET=tinyhttpd.c
CC=g++

CFLAGS += -g -Wall
LFLAGS += -lpthread
OBJ = tinyhttpd.o

$(TARGET):$(OBJ)
	$(CC) $^ $(LFLAGS) -o $@
	mv *.o ./build/
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	$(RM) -r ./build/*.o tinyhttpd
