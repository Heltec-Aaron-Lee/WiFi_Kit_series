This repo contains BearSSL (https://bearssl.org) ported for use on the
ESP8266 by Earle F. Philhower, III <earlephilhower@yahoo.com>.

Due to stack limitations on the ESP8266, most functions that have large
stack frames have been ported via macros to use a secondary stack,
managed in the heap.

Many constants and the output of the Forth compiler T0 now is stored in
PROGMEM, and patches have been applied to seamlessly run the interpreter
from that space.

Additional functions for parsing public keys and for a dynamic trust
anchor repository (since ESP8266 PROGMEM or RAM too small) were added.

To build:
. Ensure the SDK gcc is present in your path
. make clean
. make CONF=esp8266 T0   # Compiles the *.t0 -> *.c forth interpreters
. make CONF=esp8266
. Copy the resulting lib in esp8266/libbearssl.a to the Arduino path

May 15, 2018
-EFP3

