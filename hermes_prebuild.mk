RTOS_IDIR =-I./src/includes -I../includes
HERMES_ODIR=./obj
HERMES_SDIR=./src
HERMES_OBJS = \
	start.o		\
	atmel_same70_drivers.o \
	hermes.o \
	instdecode.o \
	nalloc.o \
	scheduler.o \
	guestinit.o \
	virt_eth.o
	
# Not used	
#	syslog.o
#	23k256.o
#	adc.o
HERMES_OBJ = $(patsubst %,$(HERMES_ODIR)/%,$(HERMES_OBJS))

HERMES_OBJFILENAME=hermes
HERMES_CFLAGS+=-DDEBUG -Os -g -s -Wall -mcpu=$(CPUTYPE) $(RTOS_IDIR) -fno-inline -fno-keep-static-consts -fmerge-all-constants
CC=arm-linux-gnueabi-gcc
OBJDUMP=arm-linux-gnueabi-objdump
OBJCOPY=arm-linux-gnueabi-objcopy
AR=arm-linux-gnueabi-ar


# was in next line at end: $(DEPS)
$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/%.c
		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/%.s
		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/%.sx
		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
#$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/xbee/%.c
#		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
#$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/xbee/%.s
#		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
#$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/xbee/%.sx
#		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/uip/uip/%.c
		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/uip/uip/%.s
		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/uip/uip/%.sx
		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/uip/apps/webserver/%.c
		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/uip/apps/webserver/%.s
		$(CC) $(HERMES_CFLAGS) -c -o $@ $^
$(HERMES_ODIR)/%.o: $(HERMES_SDIR)/uip/apps/webserver/%.sx
		$(CC) $(HERMES_CFLAGS) -c -o $@ $^

flash: $(HERMES_OBJ)
	$(AR) rcs lib$(HERMES_OBJFILENAME).a $(HERMES_OBJ)
#	$(CC) $(HERMES_CFLAGS) -nostartfiles -o $(HERMES_OBJFILENAME) -T linkerscript-flash.ld $(HERMES_OBJ)
#	cp $(OBJFILENAME) $(OBJFILENAME).elf
#	cp $(OBJFILENAME) $(OBJFILENAME).flash
#	$(OBJCOPY) -O srec $(OBJFILENAME).flash
	m68k-elf-size --common lib$(HERMES_OBJFILENAME).a
