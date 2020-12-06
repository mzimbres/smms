pkg_name = smms
pkg_version = 1.0.0
pkg_revision = 1
tarball_name = $(pkg_name)-$(pkg_version)-$(pkg_revision)
tarball_dir = $(pkg_name)-$(pkg_version)
prefix = /usr
datarootdir = $(prefix)/share
datadir = $(datarootdir)
docdir = $(datadir)/doc/$(pkg_name)
bindir = $(prefix)/bin
srcdir = .
confdir = /etc/$(pkg_name)
systemddir = /lib/systemd/system

bin_final_dir = $(DESTDIR)$(bindir)
doc_final_dir = $(DESTDIR)$(docdir)
conf_final_dir = $(DESTDIR)$(confdir)
service_final_dir = $(DESTDIR)$(systemddir)

boost_dir = /opt/boost_1_74_0

ext_libs =
ext_libs += $(boost_dir)/lib/libboost_program_options.a
ext_libs += $(boost_dir)/lib/libboost_filesystem.a

LDFLAGS += -lpthread
LDFLAGS += -lfmt
LDFLAGS += -lsodium
LDFLAGS += -lssl
LDFLAGS += -lcrypto

CPPFLAGS += -std=c++20
CPPFLAGS += -I$./src -I$(boost_dir)/include
CPPFLAGS += $(pkg-config --cflags libsodium)
CPPFLAGS += $(CXXFLAGS)
CPPFLAGS += -g #-O2
CPPFLAGS += -D BOOST_ASIO_NO_DEPRECATED 

VPATH = ./src

exes += smms
exes += smms-keygen
exes += test
exes += compress_test

common_objs += crypto.o

smms_objs += acceptor.o
smms_objs += logger.o
smms_objs += net.o
smms_objs += utils.o

exe_objs = $(addsuffix .o, $(exes))

lib_objs =
lib_objs += $(smms_objs)
lib_objs += $(common_objs)

aux = Makefile

all: $(exes)

Makefile.dep:
	-$(CXX) -MM ./src/*.cpp > $@

-include Makefile.dep

smms: % : %.o $(smms_objs) $(common_objs)
	$(CXX) -o $@ $^ $(CPPFLAGS) $(LDFLAGS) $(ext_libs) -DBOOST_ASIO_DISABLE_THREADS

smms-keygen: % : %.o  $(common_objs)
	$(CXX) -o $@ $^ $(CPPFLAGS) $(LDFLAGS) $(ext_libs) -lfmt -lsodium

compress_test: % : %.o
	$(CXX) -o $@ $^ $(CPPFLAGS) -lz

test: % : %.o  $(common_objs)
	$(CXX) -o $@ $^ $(CPPFLAGS) $(LDFLAGS) -lfmt -lsodium

install: all
	install -D smms --target-directory $(bin_final_dir)
	install -D smms-keygen --target-directory $(bin_final_dir)
	install -D config/smms.conf $(conf_final_dir)/smms.conf

uninstall:
	rm -f $(bin_final_dir)/smms
	rm -f $(bin_final_dir)/smms-keygen
	rm -f $(conf_final_dir)/smms.conf
	rmdir $(DESDIR)$(docdir)

.PHONY: clean
clean:
	rm -f $(exes) $(exe_objs) $(lib_objs) $(tarball_name).tar.gz Makefile.dep Makefile.dep
	rm -rf tmp

$(tarball_name).tar.gz:
	git archive --format=tar.gz --prefix=$(tarball_dir)/ HEAD > $(tarball_name).tar.gz

.PHONY: dist
dist: $(tarball_name).tar.gz

.PHONY: deb
deb: dist
	rm -rf tmp; mkdir tmp; mv $(tarball_name).tar.gz tmp; cd tmp; \
	ln $(tarball_name).tar.gz $(pkg_name)_$(pkg_version).orig.tar.gz; \
	tar -xvvzf $(tarball_name).tar.gz; \
	cd $(tarball_dir)/debian; debuild --no-sign -j1

backup_emails = foo@bar.com

.PHONY: backup
backup: $(tarball_name).tar.gz
	echo "Backup" | mutt -s "Backup" -a $< -- $(backup_emails)

