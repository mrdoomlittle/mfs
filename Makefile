ifndef inc_flags
 inc_flags=-I/usr/local/include
endif
ifndef arc
 arc=arc64
endif
ifndef f_cpu
 f_cpu=16000000UL
endif
ifndef device
 device=atmega328p
endif
all: clean
	avr-gcc -c -g -D__$(arc) $(inc_flags) -std=c11 -DF_CPU=$(f_cpu) -Os -mmcu=$(device) -o mfs.o mfs.c
	ar rc lib/libmdl-mfs.a mfs.o
	cp *.h inc/mdl
clean:
	sh clean.sh
