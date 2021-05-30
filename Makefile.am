ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

AM_COLOR_TESTS = always

Makefile.dep:
	-$(CXX) -MM ./src/*.cpp > $@

-include Makefile.dep

noinst_LIBRARIES =
bin_PROGRAMS =
noinst_PROGRAMS =

noinst_LIBRARIES += libcrypt.a
libcrypt_a_SOURCES =
libcrypt_a_SOURCES += $(top_srcdir)/src/crypto.hpp
libcrypt_a_SOURCES += $(top_srcdir)/src/crypto.cpp

noinst_LIBRARIES += libutils.a
libutils_a_SOURCES =
libutils_a_SOURCES += $(top_srcdir)/src/utils.hpp
libutils_a_SOURCES += $(top_srcdir)/src/utils.cpp

bin_PROGRAMS += smms
smms_SOURCES =
smms_SOURCES += $(top_srcdir)/src/acceptor.cpp
smms_SOURCES += $(top_srcdir)/src/acceptor.hpp
smms_SOURCES += $(top_srcdir)/src/logger.cpp
smms_SOURCES += $(top_srcdir)/src/logger.hpp
smms_SOURCES += $(top_srcdir)/src/net.cpp
smms_SOURCES += $(top_srcdir)/src/net.hpp
smms_SOURCES += $(top_srcdir)/src/session.hpp
smms_SOURCES += $(top_srcdir)/src/session_impl.hpp
smms_SOURCES += $(top_srcdir)/src/session_impl.cpp
smms_SOURCES += $(top_srcdir)/src/smms.cpp
smms_SOURCES += $(top_srcdir)/src/utils.cpp
smms_SOURCES += $(top_srcdir)/src/utils.hpp
smms_CPPFLAGS =
smms_CPPFLAGS += $(BOOST_CPPFLAGS)
smms_CPPFLAGS += -I$(top_srcdir)/src
smms_LDFLAGS =
smms_LDFLAGS += $(BOOST_LDFLAGS)
smms_LDADD =
smms_LDADD += -lfmt 
smms_LDADD += libcrypt.a
smms_LDADD += libutils.a
smms_LDADD += -l:libboost_program_options.a
smms_LDADD += -l:libboost_filesystem.a
smms_LDADD += -lsodium
smms_LDADD += -ljpeg
smms_LDADD += -lpthread
smms_LDADD += -lssl
smms_LDADD += -lcrypto

bin_PROGRAMS += smms-keygen
smms_keygen_SOURCES =
smms_keygen_SOURCES += $(top_srcdir)/src/smms-keygen.cpp
smms_keygen_CPPFLAGS =
smms_keygen_CPPFLAGS += $(BOOST_CPPFLAGS)
smms_keygen_CPPFLAGS += -I$(top_srcdir)/src
smms_keygen_LDADD =
smms_keygen_LDADD += -l:libboost_program_options.a
smms_keygen_LDADD += libcrypt.a
smms_keygen_LDADD += libutils.a
smms_keygen_LDADD += -lfmt 
smms_keygen_LDADD += -lsodium 
smms_keygen_LDADD += -lssl
smms_keygen_LDADD += -lcrypto

noinst_PROGRAMS += test
test_SOURCES =
test_SOURCES += $(top_srcdir)/src/test.cpp
test_CPPFLAGS =
test_CPPFLAGS += $(BOOST_CPPFLAGS)
test_CPPFLAGS += -I$(top_srcdir)/src
test_LDADD =
test_LDADD += libcrypt.a
test_LDADD += libutils.a
test_LDADD += -lsodium 
test_LDADD += -lssl
test_LDADD += -lcrypto

TESTS = test

EXTRA_DIST =
EXTRA_DIST += $(top_srcdir)/debian
EXTRA_DIST += $(top_srcdir)/README.md

dist_sysconf_DATA =
dist_sysconf_DATA += $(top_srcdir)/config/smms.conf

dist_man_MANS = $(top_srcdir)/doc/smms.1

CLEANFILES =
CLEANFILES += Makefile.dep

.PHONY: deb
deb: dist
	export CPPFLAGS="$(BOOST_CPPFLAGS) $(CPPFLAGS)" &&\
	export LDFLAGS="$(BOOST_LDFLAGS)" &&\
	rm -rf tmp &&\
	mkdir tmp &&\
	mv $(distdir).tar.gz $(distdir)-1.tar.gz &&\
	mv $(distdir)-1.tar.gz tmp &&\
	cd tmp &&\
	ln $(distdir)-1.tar.gz $(PACKAGE)_$(VERSION).orig.tar.gz &&\
	tar -xvvzf $(distdir)-1.tar.gz &&\
	cd $(distdir)/debian; debuild --no-sign -j1

