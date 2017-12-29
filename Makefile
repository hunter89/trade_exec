CC=clang++
CFLAGS=-c -std=c++11 -pthread

all: trader

trader: traderMain.o
	$(CC) traderMain.o -o trader

traderMain.o: traderMain.cpp
	$(CC) $(CFLAGS) traderMain.cpp

clean:
		rm -rf trader *.o