# i2c_fram
Example I2C FRAM Utility for Raspberry Pi using MB85RC256/Adafruit I2C FRAM chip

This project requires the bcm2835 library (http://www.airspayce.com/mikem/bcm2835/)

read [16-bit addr]

write [16-bit addr] [data]

dump [16-bit addr] [len]

erase_all

read_image [16-bit addr] [len] [file]

write_image [16-bit addr] [file]

This is just some more practice C while I learn to work with I2C and SPI on the Raspberry Pi. The code is far from perfect, but a good starting point for those interested in the revelation of FRAM chips.
