/*
 * fram.cpp
 *
 *  Created on: Apr 26, 2017
 *      Author: ryan
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <bcm2835.h>

#define PRINT_DATA(a, b, c) \
if(a) { \
printf("%s\n", a); \
} \
   for(int i = 0; i < c; i++) { \
    if(i && !(i%16)) \
    printf("\n"); \
    printf("%02X", *(b+i)); \
   } \
   printf("\n");

uint8_t slave_address = 0x50;

int check_boundary(uint8_t type, uint16_t addr, uint16_t len)
{
	switch(type)
	{
		case 1:
			if (addr > 0x8000)
			{
				printf("[-] Address out of bounds\n");
				return 1;
			}
			break;
		case 2:
			if (addr+len > 0x8000)
			{
				printf("[-] Data out of bounds\n");
				return 1;
			}
			break;
		case 3:
			if (addr > 0x8000)
			{
				printf("[-] Address out of bounds\n");
				return 1;
			}
			if (addr+len > 0x8000)
			{
				printf("[-] Data out of bounds\n");
				return 1;
			}
			break;
		default:
			return 0;
	}
	return 0;
}

int read_byte(uint16_t addr)
{
	int err;
	char buf[2] = {0};
	uint8_t rsp = 0;

	err = check_boundary(1, addr, 0x0);
	if (err) return 1;

	buf[0] = addr >> 8;
	buf[1] = addr;

	printf("Reading from 0x%04hX\n", addr);

	rsp = bcm2835_i2c_write(buf, 2);
	if (rsp)
	{
		printf("Error setting address! rsp: %d\n", rsp);
		return 1;
	}
	memset(buf, 0, sizeof(buf));
	rsp = bcm2835_i2c_read(buf, 1);
	if (rsp)
	{
		printf("Error reading from chip! rsp: %d\n", rsp);
		return 1;
	}
	printf("Data: 0x%02hhX\n\n", buf[0]);
	printf("[+] READ OKAY!\n");
	return 0;
}

int write_byte(uint16_t addr, uint8_t data)
{
	int err;
	char buf[3] = {0};
	uint8_t rsp = 0;

	err = check_boundary(1, addr, 0x0);
	if (err) return 1;

	buf[0] = addr >> 8;
	buf[1] = addr;
	buf[2] = data;

	printf("Writing 0x%02hhX to 0x%04hX\n", data, addr);

	rsp = bcm2835_i2c_write(buf, 3);
	if (rsp)
	{
		printf("Error writing data to chip! rsp: %d\n", rsp);
		return 1;
	}

	rsp = bcm2835_i2c_write(buf, 2); // set address
	if (rsp)
	{
		printf("Error setting address for verify! rsp: %d\n", rsp);
		return 1;
	}
	buf[2] = 0x00;
	rsp = bcm2835_i2c_read(&buf[2], 1);

	if (rsp)
	{
		printf("Failed to read back write data!\n");
		return 1;
	}

	if (buf[2] != data)
	{
		printf("Write verification failed! %02hhx %02hhx\n", buf[2], data);
		return 1;
	}

	printf("\n[+] WRITE OKAY!\n");
	return 0;
}

int dump_bytes(uint16_t addr, uint16_t len)
{
	int err;
	char buf[2] = {0};
	char * data;
	uint8_t rsp = 0;

	err = check_boundary(3, addr, len);
	if (err) return 1;

	data = (char*)malloc(sizeof(char)*len);
	if (!data)
	{
		printf("Error allocating memory for buffer!\n");
		return 1;
	}
	buf[0] = addr >> 8;
	buf[1] = addr;
	rsp = bcm2835_i2c_write(buf, 2);
	if (rsp)
	{
		printf("Error setting address for dump!\n");
		return 1;
	}
	rsp = bcm2835_i2c_read(data, len);
	if (rsp)
	{
		printf("Error reading data for dump!\n");
		return 1;
	}

	PRINT_DATA("[+] Dumping...\n", data, len);
	printf("[+] DUMP OKAY!\n");
	return 0;
}

void erase_all(void)
{
	char buf[66] = {0};
	uint16_t addr = 0;
	int i;
	
	for (i=0;i<512;i++)
	{
		buf[0] = addr >> 8;
		buf[1] = addr;
		bcm2835_i2c_write(buf, 66);
		addr += 64;
		printf("\rErasing 64-byte chunks: %d", i+1);
		fflush(stdout);
	}
	printf("\n[+] ERASE OKAY\n");
}

int read_image(uint16_t addr, uint16_t len, char *file)
{
	int err;
	FILE *outf;
	char buf[2] = {0};
	char * data;
	uint8_t rsp = 0;

	err = check_boundary(3, addr, len);
	if (err) return 1;

	data = (char*)malloc(sizeof(char)*len);
	if (!data)
	{
		printf("Failed to allocate buffer!\n");
		return 1;
	}

	printf("Reading %d bytes to file %s\n", len, file);

	buf[0] = addr >> 8;
	buf[1] = addr;

	rsp = bcm2835_i2c_write(buf, 2);
	if (rsp)
	{
		printf("Error setting address for image read!\n");
		return 1;
	}
	rsp = bcm2835_i2c_read(data, len);
	if (rsp)
	{
		printf("Error reading data for image read!\n");
		return 1;
	}

	outf = fopen(file, "wb");
	if (!outf)
	{
		printf("Error opening file %s!\n", file);
		return 1;
	}
	if (!fwrite(data, 1, len, outf))
	{
			printf("Failed to write file %s\n", file);
			fclose(outf);
			return 1;
	}
	fsync(fileno(outf));
	fclose(outf);
	printf("[+] READ IMAGE OKAY!\n");
	return 0;
}

int write_image(uint16_t addr, char *file)
{
	int err;
	FILE *inf;
	char * data;
	uint16_t size = 0;
	uint16_t result = 0;
	uint8_t rsp = 0;

	inf = fopen(file, "rb");
	if (!inf)
	{
		printf("Error opening file %s!\n", file);
		return 1;
	}

	fseek(inf, 0, SEEK_END);
	size = ftell(inf);
	rewind(inf);

	err = check_boundary(3, addr, size);
	if (err) return 1;

	data = (char*)malloc(sizeof(char)*(size+2));
	if (!data)
	{
		printf("Failed to allocate buffer!\n");
		return 1;
	}

	result = fread((data+2), 1, size, inf);
	if (result != size)
	{
		printf("Error reading file %s!\n", file);
		fclose(inf);
		return 1;
	}
	fclose(inf);

	printf("Writing %d bytes from %s...\n", size-2, file);

	data[0] = addr >> 8;
	data[1] = addr;
	rsp = bcm2835_i2c_write(data, size+2);
	if (rsp)
	{
		printf("Error writing data from image!\n");
		return 1;
	}
	printf("\n[+] WRITE IMAGE OKAY!\n");
	return 0;
}

int init_fram(void)
{
	if (!bcm2835_init())
	{
		printf("[-] bcm2835_init failed. Are you running as root?\n");
		return 1;
	}
	if (!bcm2835_i2c_begin())
	{
		printf("[-] bcm2835_i2c_begin failed. Are you running as root?\n");
		return 1;
	}

	bcm2835_i2c_setSlaveAddress(slave_address); // 0x50
	bcm2835_i2c_setClockDivider(BCM2835_I2C_CLOCK_DIVIDER_626);

	return 0;
}

int main(int argc, char **argv)
{
	int ret;
	char file[200] = {0};
	uint16_t len = 0;
	uint16_t addr = 0;
	uint8_t data = 0;

	ret = init_fram();
	if (ret) return ret;
	if (argc < 2)
	{
		printf("\nI2C FRAM Utility\n");
		printf("----------------\n");
		printf("by OscarDaGrach\n\n");
		printf("read [16-bit addr]\n");
		printf("write [16-bit addr] [data]\n");
		printf("dump [16-bit addr] [len]\n");
		printf("erase_all\n");
		printf("read_image [16-bit addr] [len] [file]\n");
		printf("write_image [16-bit addr] [file]\n");
		printf("\n");

		return 1;
	}
	if (strcmp("read", argv[1]) == 0)
	{
		if (argc != 3)
		{
			printf("Wrong args!\n");
			printf("read [16-bit addr]\n");
			return 1;
		}
		sscanf(argv[2], "%hx", &addr);
		read_byte(addr);
	}

	if (strcmp("write", argv[1]) == 0)
	{
		if (argc != 4)
		{
			printf("Wrong args!\n");
			printf("write [16-bit addr] [data]\n");
			return 1;
		}
		sscanf(argv[2], "%hx", &addr);
		sscanf(argv[3], "%hhx", &data);
		write_byte(addr, data);
	}
	if (strcmp("dump", argv[1]) == 0)
	{
		if (argc != 4)
		{
			printf("Wrong args!\n");
			printf("dump [16-bit addr] [len]\n");
			return 1;
		}
		sscanf(argv[2], "%hx", &addr);
		sscanf(argv[3], "%hx", &len);
		dump_bytes(addr, len);
	}
	if (strcmp("erase_all", argv[1]) == 0)
	{
		erase_all();
	}
	if (strcmp("read_image", argv[1]) ==0)
	{
		if (argc != 5)
		{
			printf("Wrong args!\n");
			printf("read_image [16-bit addr] [len] [file]\n");
			return 1;
		}
		sscanf(argv[2], "%hx", &addr);
		sscanf(argv[3], "%hx", &len);
		sscanf(argv[4], "%s", file);
		read_image(addr, len, file);
	}
	if (strcmp("write_image", argv[1]) ==0)
	{
		if (argc != 4)
		{
			printf("Wrong args!\n");
			printf("write_image [16-bit addr] [file]\n");
			return 1;
		}
		sscanf(argv[2], "%hx", &addr);
		sscanf(argv[3], "%s", file);
		write_image(addr, file);
	}

	bcm2835_i2c_end();
	bcm2835_close();

	return 0;
}


