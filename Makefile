
include config.mk
HERMES_CFLAGS+=$(CFLAGS)
include hermes_prebuild.mk
default: flash



ram: $(HERMES_OBJ)
	$(CC) $(HERMES_CFLAGS) -nostartfiles -o $(HERMES_OBJFILENAME) -T linkerscript-ram.ld $(HERMES_OBJ)
	cp $(HERMES_OBJFILENAME) $(HERMES_OBJFILENAME).elf
	cp $(HERMES_OBJFILENAME) $(HERMES_OBJFILENAME).ram
	$(OBJCOPY) -O srec $(HERMES_OBJFILENAME).ram
	$(SIZE) --common $(HERMES_OBJFILENAME).elf

clean:
	rm -f lib$(HERMES_OBJFILENAME).a $(HERMES_OBJFILENAME).ram  $(HERMES_OBJFILENAME).flash $(HERMES_OBJFILENAME).elf $(HERMES_ODIR)/*

disassemble:
	$(OBJDUMP) --source $(HERMES_OBJFILENAME).elf

program: devCheck fileCheck
	@echo "Erasing..."
	@echo -n "5" >> $(dev)
	@sleep 1
	@echo -n "y" >> $(dev)
	@sleep 2
	@echo "Programming..."
	@echo -n "2" >> $(dev)
	@sleep 0.1
	./xmit_srec.sh $(dev) $(file)

devCheck:
ifndef dev
	@echo "Defaulting to /dev/ttyUSB0"
dev=/dev/ttyUSB0
endif

fileCheck:
ifndef file
	@echo "Defaulting to meter.flash"
file=$(HERMES_OBJFILENAME).flash
endif

config: src/config/config.c
	gcc -o config src/config/config.c
	./config
	rm config

