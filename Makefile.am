AUTOMAKE_OPTIONS = foreign subdir-objects

mod_LTLIBRARIES = mod_morse.la
mod_morse_la_SOURCES  = mod_morse.c
mod_morse_la_CFLAGS   = $(FREESWITCH_CFLAGS) -Wall
mod_morse_la_LDFLAGS  = -avoid-version -module -no-undefined -shared $(FREESWITCH_LDFLAGS)
