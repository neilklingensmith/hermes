


Hermes
======


Building
--------

An app based around Hermes is compiled in two phases---first we will build the
hypervisor as a static library, and second we will build the guests, linking the
hypervisor library code in. In order to build the hypervisor, we will need to
have a list of guests that will be initialized on boot.


### Initializing Guests

To initialize guests, we need (1) a lists of guests and (2) peripheral devices
associated with each guest. The `guestsetup.py` script generates a C file that
will initialize guests and their I/O devices on boot. This C file will contain
functions that are called by the Hermes startup code. The `guestsetup.py` script
takes a config file as input with the following syntax:


    [guests]

    dummyguest dummyVectorTable

    [io]

    45 0xc0 dummyguest # UART3 interrupt

The guest setup config file format opens with a list of guests preceeded by a
`[guests]` line. Each line in the guest list specifies (1) the guest's name and
(2) the guest's vector table. The guest's stack and entry point are implied by
the vector table. The guest's name can be whatever---it does not have to
correspond to a symbol name in the guest's code. It will only be used to
identify the guest in the I/O section. The vector table however does need to
correspond to an array of pointers or integers in the guest's C code that
contains the guest's vector table. For example, in the guest's C code we might
initialize `dummyVectorTable` in the above example as:

    void *dummyVectorTable[] = { dummyStack + sizeof(dummyStack),
                                 dummyEntryPoint,
                                 dummyVector1,
                                 dummyVector2,
                                 ...



After the guest list, there is a list of I/O device associations preceeded by a
`[io]` line. Each I/O line contains (1) the peripheral's exception number, (2)
the peripheral's exception priority, and (3) the associated guest. In ARM the
exception number can be confusing. In this case, the exception number refers NOT
to the index of the exception vector within the vector table, but to the offset
into the peripheral interrupt portion of the vector table. ARM vector tables
have two sections: the first 16 vectors are exceptions (bus fault, usage fault,
etc.), and the remaining vectors are for peripheral interrupts. In the above
example, the UART3 interrupt is at offset 45 in the peripheral section of the
vector table, which is at offset 45+16=60 from the start of the vector table.

To create guestinit.c from guests.conf, run:

    # ./guestsetup.py guests.conf > src/guestinit.c

Be aware that this will overwrite the existing guestinit.c.


