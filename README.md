# i2c_fram
Example I2C FRAM Utility for Raspberry Pi using MB85RC256
REQUIRES BCM2835 LIBRARY!


read [16-bit addr]

write [16-bit addr] [data]

dump [16-bit addr] [len]

erase_all

read_image [16-bit addr] [len] [file]

write_image [16-bit addr] [file]

This is just some more practice C/C++ while I learn to work with I2C and SPI on the Raspberry Pi. The code is far from perfect, but a good starting point for those interested in the revelation of FRAM chips.
