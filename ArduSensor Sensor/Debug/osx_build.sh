avr-gcc -x c -funsigned-char -funsigned-bitfields -DDEBUG  -O1 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -g2 -Wall -mmcu=atmega48p -c "../ArduSensor.c" -std=gnu99 

avr-gcc ArduSensor.o uart.o -mmcu=atmega48p -o ArduSensor.elf

avr-objcopy -j .text -j .data -O ihex ArduSensor.elf ArduSensor.hex
