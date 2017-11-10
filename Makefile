CC = gcc
INCLUDES = -I ./

all:
	$(CC) $(INCLUDES) i2c_fram.c -lbcm2835 -o i2c_fram

clean:
	rm i2c_fram
