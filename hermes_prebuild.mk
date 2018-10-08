HERMES_IDIR =-I./src
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
HERMES_CFLAGS+=-Wall -Wstrict-prototypes -Wmissing-prototypes -std=gnu99 -fno-strict-aliasing -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wwrite-strings -Wsign-compare -Waggregate-return -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 
HERMES_CFLAGS+=-O0 -g -s -Wall -mthumb -mcpu=$(CPUTYPE) $(HERMES_IDIR)
CC=arm-none-eabi-gcc
OBJDUMP=arm-none-eabi-objdump
OBJCOPY=arm-none-eabi-objcopy
AR=arm-none-eabi-ar
SIZE=arm-none-eabi-size

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
	$(SIZE) --common lib$(HERMES_OBJFILENAME).a
