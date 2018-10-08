#!/usr/bin/python

import sys
from enum import Enum

def parseGuestConfigFile(configFileName):
    COMMENT_CHAR = '#'

    # Read all lines of confFile into configLines
    with open(configFileName) as confFile:
        configLines = confFile.readlines()

    # Strip whitespace characters
    configLines = [x.strip() for x in configLines]

    class ParseState(Enum):
        searchGuestStart = 1 # search for [guests]
        readGuestNames = 2   # Read names & vector tables for each guest
        readIO = 3           # Read IO configuration

    state = ParseState.searchGuestStart
    guests = dict() # Create an empty dictionary of guests
    for line in configLines:
        if not line:
            # Empty line
            continue
        # Strip comments from end of line
        if '#' in line:
            line, comment = line.split(COMMENT_CHAR, 1)
            line = line.strip() # Re-strip whitespace preceeding comment
        if state is ParseState.searchGuestStart:
            # Searching for a line that contains [guests] to indicate the start
            # of the list of guests
            if "[guests]" in line:
                state = ParseState.readGuestNames
        elif state is ParseState.readGuestNames:
            # Searching for either (1) valid guest definition or (2) a line that
            # contains [io] to indicate the beginning of the vm's IO definitions
            if "[io]" in line:
                state = ParseState.readIO
            else:
                # Add the guest definition to the dictionary of guests
                guestdef = line.split(' ')
                guests[guestdef[0]] = dict()
                guests[guestdef[0]]['vectortable'] = guestdef[1]
                guests[guestdef[0]]['io'] = dict()
        elif state is ParseState.readIO:
            number,priority,guestassignment = line.split(' ')
            guests[guestassignment]['io'][number] = priority

    return guests


def writeCopyright(outfile):
    print """
/*

MIT License

Copyright (c) 2018 Neil Klingensmith

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
"""

def writeGlobals(guests,outfile):
    outfile.write("""
#include "hermes.h"

/////////////////////////////////////////////////////
// Global variables
extern struct interrupt intlist[NUM_PERIPHERAL_INTERRUPTS];

// List of guests running on Hermes
""")
    
    for guestname in guests:
        outfile.write("struct vm *__" + guestname +";\n")

    outfile.write("\n")


def writeIoInit(guests,outfile):

    outfile.write("""
/*
 * ioInit
 *
 * This is where we assign MCU peripherals to guests in Hermes. For each
 * peripheral, we are going to do three basic tasks:
 *
 *    1. Set the interrupt's priority in the ARM's Nonvolatile Interrupt
 *       Controller (NVIC) Interrupt Priority Register (IPR)
 *    2. Enable interrupt source in the Interrupt Set Enable Register (ISER).
 *    3. Assign the interrupt to a guest in the in Hermes's intlist[] array.
 */
void ioInit(void) {
    // Set interrupt priority in the IPR
""")
    for guestname,guest in guests.iteritems():
        for isrnum,isrprio in guest['io'].iteritems():
            outfile.write("    NVIC_IPR[" + str(isrnum) + "] = " + isrprio + ";\n")

    outfile.write("\n    // Set the interrupt enable bit in ISERx\n")
    for guestname,guest in guests.iteritems():
        for isrnum,isrprio in guest['io'].iteritems():
            iserNum = 0
            bitNum = int(isrnum)
            while bitNum >= 32:
                bitNum -= 32
                iserNum += 1

            outfile.write("    ISER" + str(iserNum)+" |= (1<<" + str(bitNum) + ");\n")
 
    outfile.write("\n    // Assign the interrupt source to the correct guest\n")
    for guestname,guest in guests.iteritems():
        for isrnum,isrprio in guest['io'].iteritems():
            outfile.write("    intlist[" + str(isrnum) + "].owner = __" + guestname + ";\n")
            outfile.write("    intlist[" + str(isrnum) + "].priority = " + isrprio + ";\n")


    outfile.write("}\n")

def writeGuestInit(guests,outfile):
    # Function preamble
    outfile.write("""
/*
 * guestInit
 *
 * Initialize guests 
 *
 */
void guestInit(void) {
""")
    # Define guest structures
    for guestname,guest in guests.iteritems():
        outfile.write("  extern void *" + guest['vectortable'] +";\n")

    outfile.write("\n")

    # Call createGuest() for each guest
    for guestname,guest in guests.iteritems():
        outfile.write("    __" + guestname + " = createGuest(" + guest['vectortable'] + ");\n")

    outfile.write("}\n")

def main():

    outfile = sys.stdout

    guests = parseGuestConfigFile(sys.argv[1])

    writeCopyright(outfile)

    writeGlobals(guests,outfile)

    writeIoInit(guests,outfile)

    writeGuestInit(guests,outfile)
    return

if __name__ == "__main__":
    main()
