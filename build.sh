#!/bin/sh

clear
echo Build started.
rm DEMO128C.PRG demo128c.list demo128c.o
cl65 -t cx16 -o DEMO128C.PRG -l demo128c.list demo128c.asm
echo Build ended.

