#
# Makefile for the Stalker Client add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = Kodi_Stalker_Client
ADDONNAME       = pvr.stalker.client
LIBNAME         = libstalkerclient-addon
lib_LTLIBRARIES = libstalkerclient-addon.la

LIBS            = @abs_top_srcdir@/lib/tinyxml/libtinyxml.la

include ../Makefile.include.am

libpvrdemo_addon_la_SOURCES = src/client.cpp \
                              src/PVRDemoData.cpp
libpvrdemo_addon_la_LDFLAGS = @TARGET_LDFLAGS@

