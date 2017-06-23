# README #

This README would normally document whatever steps are necessary to get your application up and running.

## Introduction ##



## Getting Started ##

### Prereqs ###

**You will need:**

* ** A windows machine/VM **
* ** An Atmel SAM E70 Xplained Demo Board **

## Procedure ##

1. Download the FreeRTOS+Hermes Source code from [here](http://hermes.wings.cs.wisc.edu/files/hermes-FreeRTOS.zip).
2. Download Atmel Studio 7.0 from [here](http://atmel.com).
3. Uncompress the source code archive, remove the hermes directory that's there (it's old) and clone the hermes git repo into that directory.
4. Open the blinky demo project in Atmel Studio, located in the same directory, compile, and run.
5. Atmel Studio is quirky when running FreeRTOS. There's a trick to attaching Atmel Studio IDE to the debugger so you can step through the code. See [this video] (https://youtu.be/UuvNLr9QJzg).

## Porting Code to run as a Guest in Hermes ##

There are two things you need to do to make your code run as a guest within Hermes.

1. Add `UDF #0` instruction after every `MRS` and `MSR` instruction. `MRS` and
`MSR` are privileged instructions in the sense that they must be executed in
master mode to affect the CPU state. If they are execited in thread mode, they
behave like a NOP: the CPU will not commit the instruction, and it won't throw
an exception. We need to add the `UDF #0` immediately after `MRS` and `MSR` in
order to trap to the hypervisor and alert it that the guest is attempting to
execute a privileged instruction.

2. Add an `ISB` instruction immediately after each access to the system control
block (SCB). Accesses to the SCB do cause exceptions, but they are imprecise,
meaning that the exception may not be generated until many instructions after
the SCB access. If that happens, the processor state may have changed (ie. core
register values overwritten), making it impossible for the hypervisor to
emulate the SCB access. Adding the `ISB` instruction after the SCB access will
force the SCB access to complete before proceeding to later instrucitons. This
means that the exception will be generated immediately following the SCB access,
before CPU state has changed, and the hypervisor will be able to emulate it
properly.


## Contributing ##

Contact [Neil Klingensmith](http://neilklingensmith.com)