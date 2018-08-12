#!/bin/bash

#
# objrename.sh
#
#
# 
# MIT License
# 
# Copyright (c) 2017 Neil Klingensmith
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 
#
# Renames symbols in the src directory. Useful for having two copies of the same
# RTOS distribution compiled into the same binary image.
#

APPENDSTR="\_g1"

# List the C and H files to be updated
flist=($(find . -type f \( -name "*.h" -or -name "*.c" \)))

for objfile in `find ../ -iname '*.o'`
do
    # Strip .o extension
    bn=`echo $objfile | cut -d'.' -f1`

    # Strip Debug directory from beginning of the path
    cpath=`echo $bn | cut -d'/' -f2-`

    # Append .c to the end of the filename
    cfile="$cpath.c"

    # Check if the object file came from the hermes directory. If so, skip it.
    # We don't need to rename symbols in the hermes distribution.
    ishermescode=`echo $objfile | grep hermes`;
    if [ ! -z "$ishermescode" ]
    then
        continue
    fi

    # Search for function names in the output of objdump
    echo "Function names in $objfile:"
    while read line
    do
        echo $line

        for ((i=0; i<${#flist[@]}; i++)); do

            # Check if the object file came from the hermes directory. If so, skip it.
            # We don't need to rename symbols in the hermes distribution.
            ishermescode=`echo $objfile | grep hermes`;
            if [ ! -z "$ishermescode" ]
            then
                echo "Skipping"
                continue
            fi

            sed -i "s/\b$line\b/$line$APPENDSTR/g" ${flist[$i]}

        done

    done < <(objdump --syms $objfile | grep text | rev | cut -f1 -d' ' | rev | grep -v ".text" | grep -v -e '^$' | grep -v '\$')

    # Search for variables in the output of objdump
    echo "Variable names in $objfile:"
    while read line
    do
        echo $line

        for ((i=0; i<${#flist[@]}; i++)); do

            # Check if the object file came from the hermes directory. If so, skip it.
            # We don't need to rename symbols in the hermes distribution.
            ishermescode=`echo $objfile | grep hermes`;
            if [ ! -z "$ishermescode" ]
            then
                echo "Skipping"
                continue
            fi

            sed -i "s/\b$line\b/$line$APPENDSTR/g" ${flist[$i]}

        done

    done < <(objdump --syms $objfile | grep 'bss\|data\|rodata\|COM' | rev | cut -f1 -d' ' | rev | grep -v ".data\|.bss\|.rodata\|*COM*" | grep -v "." | grep -v -e '^$' | grep -v '\$')

done


