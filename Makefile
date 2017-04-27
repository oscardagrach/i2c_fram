CC = g++
INCLUDES = -I ./

all:
	$(CC) $(INCLUDES) fram.cpp -lbcm2835 -o fram

clean:
	rm fram
