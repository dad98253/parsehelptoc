## Process this file with automake to produce Makefile.in

#SUBDIRS = build-aux
#AC_CONFIG_AUX_DIR = build-aux

ACLOCAL_AMFLAGS = -I m4 --install

EXTRA_DIST = m4/Makefile.in

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

@GENERATE_CHANGELOG_RULES@
  dist-hook: dist-ChangeLog

distclean-local:
	@rm config.status configure config.log
	@rm Makefile
	@rm -r autom4te.cache/
	@rm aclocal.m4
	@rm Makefile.in

#build parsehelptoc
bin_PROGRAMS = $(top_builddir)/bin/parsehelptoc
__top_builddir__bin_parsehelptoc_SOURCES = parsehelptoc.c zor.c
__top_builddir__bin_parsehelptoc_LDFLAGS = -Wl,-Map,loadmap.txt -lz -lxml2

#add header files
noinst_HEADERS = 

help:
	-@echo "make                    --> build passwin"
	-@echo "make clean              --> delete what was built"
	-@echo "make distclean          --> delete auto tools products"
	-@echo "make dist               --> build distribution package"

AM_CPPFLAGS = $(DEPS_CFLAGS) $(MY_CPPFLAGS)
AM_CXXFLAGS = -W -Wall -fmessage-length=0
AM_CFLAGS = -W -Wall -fmessage-length=0

