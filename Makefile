##============================================================================
##
##   SSSS    tt          lll  lll
##  SS  SS   tt           ll   ll
##  SS     tttttt  eeee   ll   ll   aaaa
##   SSSS    tt   ee  ee  ll   ll      aa
##      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
##  SS  SS   tt   ee      ll   ll  aa  aa
##   SSSS     ttt  eeeee llll llll  aaaaa
##
## Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
##
## See the file "license" for information on usage and redistribution of
## this file, and for a DISCLAIMER OF ALL WARRANTIES.
##
## $Id: Makefile,v 1.30 2006-12-20 20:36:25 stephena Exp $
##
##   Based on code from ScummVM - Scumm Interpreter
##   Copyright (C) 2002-2004 The ScummVM project
##============================================================================

#######################################################################
# Default compilation parameters. Normally don't edit these           #
#######################################################################

srcdir      ?= .

DEFINES     :=
LDFLAGS     :=
INCLUDES    :=
LIBS	    :=
OBJS	    :=
PROF        :=

MODULES     :=
MODULE_DIRS :=

DISTNAME    := stella-snapshot

# Load the make rules generated by configure
include config.mak

# Uncomment this for stricter compile time code verification
# CXXFLAGS+= -Werror

ifdef CXXFLAGS
  CXXFLAGS:= $(CXXFLAGS)
else
  CXXFLAGS:= -O2
endif
CXXFLAGS+= -Wall -Wno-multichar -Wunused -fno-rtti

ifdef PROFILE
  PROF:= -g -pg -fprofile-arcs -ftest-coverage
  CXXFLAGS+= $(PROF)
else
  CXXFLAGS+= -fomit-frame-pointer
endif

# Even more warnings...
#CXXFLAGS+= -pedantic -Wpointer-arith -Wcast-qual -Wconversion
#CXXFLAGS+= -Wshadow -Wimplicit -Wundef -Wnon-virtual-dtor
#CXXFLAGS+= -Wno-reorder -Wwrite-strings -fcheck-new -Wctor-dtor-privacy 

#######################################################################
# Misc stuff - you should never have to edit this                     #
#######################################################################

EXECUTABLE  := stella$(EXEEXT)

all: tags $(EXECUTABLE)


######################################################################
# Various minor settings
######################################################################

# The name for the directory used for dependency tracking
DEPDIR := .deps


######################################################################
# Module settings
######################################################################

MODULES := $(MODULES)

# After the game specific modules follow the shared modules
MODULES += \
	src/emucore \
	src/emucore/m6502 \
	src/gui \
	src/common

######################################################################
# The build rules follow - normally you should have no need to
# touch whatever comes after here.
######################################################################

# Concat DEFINES and INCLUDES to form the CPPFLAGS
CPPFLAGS:= $(DEFINES) $(INCLUDES)

# Include the build instructions for all modules
-include $(addprefix $(srcdir)/, $(addsuffix /module.mk,$(MODULES)))

# Depdir information
DEPDIRS = $(addsuffix /$(DEPDIR),$(MODULE_DIRS))
DEPFILES = 

# The build rule for the Stella executable
$(EXECUTABLE):  $(OBJS)
	$(LD) $(LDFLAGS) $(PRE_OBJS_FLAGS) $+ $(POST_OBJS_FLAGS) $(LIBS) $(PROF) -o $@

distclean: clean
	$(RM_REC) $(DEPDIRS)
	$(RM) build.rules config.h config.mak config.log

clean:
	$(RM) $(OBJS) $(EXECUTABLE)

.PHONY: all clean dist distclean

.SUFFIXES: .cxx
ifndef HAVE_GCC3
# If you use GCC, disable the above and enable this for intelligent
# dependency tracking. 
.cxx.o:
	$(MKDIR) $(*D)/$(DEPDIR)
	$(CXX) -Wp,-MMD,"$(*D)/$(DEPDIR)/$(*F).d2" $(CXXFLAGS) $(CPPFLAGS) -c $(<) -o $*.o
	$(ECHO) "$(*D)/" > $(*D)/$(DEPDIR)/$(*F).d
	$(CAT) "$(*D)/$(DEPDIR)/$(*F).d2" >> "$(*D)/$(DEPDIR)/$(*F).d"
	$(RM) "$(*D)/$(DEPDIR)/$(*F).d2"

.c.o:
	$(MKDIR) $(*D)/$(DEPDIR)
	$(CXX) -Wp,-MMD,"$(*D)/$(DEPDIR)/$(*F).d2" $(CXXFLAGS) $(CPPFLAGS) -c $(<) -o $*.o
	$(ECHO) "$(*D)/" > $(*D)/$(DEPDIR)/$(*F).d
	$(CAT) "$(*D)/$(DEPDIR)/$(*F).d2" >> "$(*D)/$(DEPDIR)/$(*F).d"
	$(RM) "$(*D)/$(DEPDIR)/$(*F).d2"
else
# If you even have GCC 3.x, you can use this build rule, which is safer; the above
# rule can get you into a bad state if you Ctrl-C at the wrong moment.
# Also, with this GCC inserts additional dummy rules for the involved headers,
# which ensures a smooth compilation even if said headers become obsolete.
.cxx.o:
	$(MKDIR) $(*D)/$(DEPDIR)
	$(CXX) -Wp,-MMD,"$(*D)/$(DEPDIR)/$(*F).d",-MQ,"$@",-MP $(CXXFLAGS) $(CPPFLAGS) -c $(<) -o $*.o

.c.o:
	$(MKDIR) $(*D)/$(DEPDIR)
	$(CXX) -Wp,-MMD,"$(*D)/$(DEPDIR)/$(*F).d",-MQ,"$@",-MP $(CXXFLAGS) $(CPPFLAGS) -c $(<) -o $*.o
endif

ifdef HAVE_NASM
.SUFFIXES: .asm
.asm.o:
	$(NASM) -O1 $(NASMFLAGS) -g -o $*.o $(<)
endif

# Include the dependency tracking files. We add /dev/null at the end
# of the list to avoid a warning/error if no .d file exist
-include $(wildcard $(addsuffix /*.d,$(DEPDIRS))) /dev/null

# check if configure has been run or has been changed since last run
config.mak: $(srcdir)/configure
	@echo "You need to run ./configure before you can run make"
	@echo "Either you haven't run it before or it has changed."
	@exit 1

install: all
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -c -s -m 755 "$(srcdir)/stella$(EXEEXT)" "$(DESTDIR)$(BINDIR)/stella$(EXEEXT)"
	$(INSTALL) -d "$(DESTDIR)$(DOCDIR)"
	$(INSTALL) -c -m 644 "$(srcdir)/Announce.txt" "$(srcdir)/Changes.txt" "$(srcdir)/Copyright.txt" "$(srcdir)/License.txt" "$(srcdir)/README-SDL.txt" "$(srcdir)/Readme.txt" "$(srcdir)/Todo.txt" "$(srcdir)/docs/index.html" "$(srcdir)/docs/debugger.html" "$(DESTDIR)$(DOCDIR)/"
	$(INSTALL) -d "$(DESTDIR)$(DOCDIR)/graphics"
	$(INSTALL) -c -m 644 "$(srcdir)/docs/graphics/chucky_cheese.png" "$(srcdir)/docs/graphics/circuit.png" "$(srcdir)/docs/graphics/console.png" "$(srcdir)/docs/graphics/eventmapping.png" "$(srcdir)/docs/graphics/eventmapping_remap.png" "$(srcdir)/docs/graphics/jr_pacman.png" "$(srcdir)/docs/graphics/launcher_options_rom.png" "$(srcdir)/docs/graphics/launcher_options_snap.png" "$(srcdir)/docs/graphics/launcher.png" "$(srcdir)/docs/graphics/options_about.png" "$(srcdir)/docs/graphics/options_audio.png" "$(srcdir)/docs/graphics/options_gameinfo.png" "$(srcdir)/docs/graphics/options_help.png" "$(srcdir)/docs/graphics/options_video.png" "$(srcdir)/docs/graphics/pacman.png" "$(srcdir)/docs/graphics/rom_browser.png" "$(srcdir)/docs/graphics/secret_quest.png" "$(srcdir)/docs/graphics/space_invaders.png" "$(DESTDIR)$(DOCDIR)/graphics"
	$(INSTALL) -d "$(DESTDIR)$(DATADIR)/applications"
	$(INSTALL) -c -m 644 "$(srcdir)/src/unix/stella.desktop" "$(DESTDIR)$(DATADIR)/applications"
	$(INSTALL) -d "$(DESTDIR)$(DATADIR)/icons"
	$(INSTALL) -d "$(DESTDIR)$(DATADIR)/icons/mini"
	$(INSTALL) -d "$(DESTDIR)$(DATADIR)/icons/large"
	$(INSTALL) -c -m 644 "$(srcdir)/src/common/stella.xpm" "$(DESTDIR)$(DATADIR)/icons"
	$(INSTALL) -c -m 644 "$(srcdir)/src/common/stella.xpm" "$(DESTDIR)$(DATADIR)/icons/mini"
	$(INSTALL) -c -m 644 "$(srcdir)/src/common/stella.xpm" "$(DESTDIR)$(DATADIR)/icons/large"

install-strip: install
	strip stella$(EXEEXT)

uninstall:
	rm -f  "$(DESTDIR)$(BINDIR)/stella$(EXEEXT)"
	rm -rf "$(DESTDIR)$(DOCDIR)/"
	rm -f  "$(DESTDIR)$(DATADIR)/applications/stella.desktop"
	rm -f  "$(DESTDIR)$(DATADIR)/icons/stella.xpm"
	rm -f  "$(DESTDIR)$(DATADIR)/icons/mini/stella.xpm"
	rm -f  "$(DESTDIR)$(DATADIR)/icons/large/stella.xpm"

# Special rule for Win32 icon stuff (there's probably a better way to do this ...)
src/win32/stella_icon.o: src/win32/stella.ico src/win32/stella.rc
	windres --include-dir src/win32 src/win32/stella.rc src/win32/stella_icon.o 
		
# Special target to create a Win32 snapshot package
win32dist: stella$(EXEEXT)
	rm -rf $(DISTNAME)
	mkdir -p $(DISTNAME)/docs/graphics
	strip stella$(EXEEXT) -o $(DISTNAME)/stella$(EXEEXT)
	cp Announce.txt Changes.txt Copyright.txt License.txt README-SDL.txt Readme.txt Todo.txt $(DISTNAME)/docs
	cp -r docs/*.html $(DISTNAME)/docs
	cp -r docs/graphics/*.png $(DISTNAME)/docs/graphics
	cp /mingw/bin/SDL.dll /mingw/bin/zlib1.dll $(DISTNAME)
#	u2d $(DISTNAME)/*.txt
#	zip $(DISTNAME)-win32.zip $(DISTNAME)

# Special target to create psp stuff
psp-package: $(EXECUTABLE)
	$(MKSFO) 'Stella' PARAM.SFO
	mkdir -p "psp/stella"
	$(PSP-FIX) stella.elf
	$(PSP-STRIP) stella.elf -o psp/stella/EBOOT.PBP
	mkdir -p "psp/stella%"
	$(PACK-PBP) "psp/stella%/EBOOT.PBP" PARAM.SFO src/psp/data/ICON0.PNG NULL NULL src/psp/data/PIC1.PNG  NULL NULL NULL
	rm -f PARAM.SFO
	
psp-upload: psp-package
	mount | grep  $(PSP-MOUNTPOINT) || mount $(PSP-MOUNTPOINT)
	cp -rv "psp/stella" "psp/stella%" $(PSP-MOUNTPOINT)/psp/game/
	umount $(PSP-MOUNTPOINT)

psp-debug:
	mount | grep \$(PSP-MOUNTPOINT) || mount $(PSP-MOUNTPOINT)
	@cat $(PSP-MOUNTPOINT)/stderr.txt
	@echo -e "\n\n------------------------------------------------------------"
	@cat $(PSP-MOUNTPOINT)/stdout.txt
	umount $(PSP-MOUNTPOINT)

psp-cleanup:
	rm -rf psp
	mount | grep $(PSP-MOUNTPOINT) || mount $(PSP-MOUNTPOINT)
	rm -rf  $(PSP-MOUNTPOINT)/std*
	umount $(PSP-MOUNTPOINT)
	
psp-layout:
	mount | grep $(PSP-MOUNTPOINT) || mount $(PSP-MOUNTPOINT)
	mkdir -p $(PSP-MOUNTPOINT)/stella/roms
	mkdir -p $(PSP-MOUNTPOINT)/stella/snapshots
	cp -v src/psp/data/stellarc.psp $(PSP-MOUNTPOINT)/stella/stellarc
	umount $(PSP-MOUNTPOINT)

# GP2X exec needs strip
gp2x-strip: $(EXECUTABLE)
	$(GP2X-STRIP) stella

# GP2X organize: groups necessary files into a gp2x folder for easy access.
gp2x-organize:
	mkdir -p "$(srcdir)/gp2x"
	mkdir -p "$(srcdir)/gp2x/docs"
	cp -v $(srcdir)/stella  $(srcdir)/gp2x
	cp -v $(srcdir)/src/gp2x/stella.gpe $(srcdir)/gp2x
	cp -v $(srcdir)/README-GP2X.txt $(srcdir)/gp2x
	cp -v -r $(srcdir)/docs/* $(srcdir)/gp2x/docs
	
.PHONY: deb bundle test win32dist install uninstall

# Use Exuberant ctags (the one from Slackware's vim package, for instance),
# not the one from emacs!
tags:
	ctags `find . -name '*.[ch]xx' -o -name '*.c' -o -name '*.y'` || true
