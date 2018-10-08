
DEVPATH := ./src/devices/microchip/sam

GITHASH := -DGIT_HASH=$(shell git log -n 1 --pretty=format:\"%H\")

LDFLAGS := -T $(DEVPATH)/same70q21.ld

# Device-specific CMSIS includes
CFLAGS += -I$(DEVPATH)/cmsis
CFLAGS += -I$(DEVPATH)/cmsis/same70/include/

CFLAGS += -DCONFIG_HERMES_RELEASE="\"0.1.0\""
CFLAGS += $(GITHASH)

CPUTYPE=cortex-m7
CFLAGS+=-D CPUTYPE=same70q21
# Hypervisor Stack Size
CFLAGS+=-D HV_STACK_SIZE=4096
CFLAGS+=-D HERMES_ETHERNET_BRIDGE=0
CFLAGS+=-D HERMES_INTERNAL_DMA=0
