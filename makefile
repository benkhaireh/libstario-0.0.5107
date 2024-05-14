VPATH = src:src/rpm-spec:bin

OBJS = stario.o stario-usb.o stario-parallel.o stario-serial.o
HEADERS = stario-error.h stario-structures.h stario-prvstructures.h

MAJOR=$(shell grep '^major' src/version | awk '{print $$2}')
MINOR=$(shell grep '^minor' src/version | awk '{print $$2}')
COMPILE=$(shell grep '^compile' src/version | awk '{print $$2}')

COMPILEOFFSET=0

ifdef STARMICRONICSBUILD
COMPILEOFFSET="5000"
endif

DEFS=
LIBS=-lc -lusb

ifdef RPMBUILD
DEFS=-DRPMBUILD
LIBS=-lc -ldl
endif

define dependencies
@if [ ! -e /usr/include/usb.h ]; then echo "libusb headers not available - exiting"; exit 1; fi
@if ! (ls /usr/lib | grep libusb > /dev/null); then echo "libusb not available - exiting"; exit 1; fi
endef

define init
@if [ ! -e bin ]; then echo "mkdir bin"; mkdir bin; fi
endef

define sweep
@if [ -e bin ]; then echo "rm -f bin/*"; rm -f bin/*; rmdir bin; fi
@if [ -e installer ]; then echo "rm -f installer"; rm -f installer; fi
@if [ -e uninstaller ]; then echo "rm -f uninstaller"; rm -f uninstaller; fi
@if [ -e src/rpm-spec/libstario.spec ]; then echo "rm -f src/rpm-spec/libstario.spec"; rm -f src/rpm-spec/libstario.spec; fi
endef

define incrementbuildversion
@rm -f version
@echo -e '# libstario version field definition file\n\nmajor\t$(MAJOR)\nminor\t$(MINOR)\ncompile\t$(shell expr $(COMPILE) + 1)\n' > src/version
endef

teststario: teststario.c libstario.so.$(MAJOR).$(MINOR).$(shell expr $(COMPILE) + $(COMPILEOFFSET)) installer uninstaller
	$(init)
	gcc -Wall -DLOCALSTARIOH -o bin/teststario $< -Lbin -lstario

libstario.so.$(MAJOR).$(MINOR).$(shell expr $(COMPILE) + $(COMPILEOFFSET)): $(OBJS)
	$(init)
	$(incrementbuildversion)
	@if [ -e bin/libstario.so.$(MAJOR).$(MINOR).$(shell expr $(COMPILE) + $(COMPILEOFFSET)) ]; then rm -f bin/libstario.so.$(MAJOR).$(MINOR).$(shell expr $(COMPILE) + $(COMPILEOFFSET)); fi
	gcc -shared -Wl,-soname,libstario.so.$(MAJOR) -o bin/libstario.so.$(MAJOR).$(MINOR).$(shell expr $(COMPILE) + $(COMPILEOFFSET) + 1) $(addprefix bin/,$(OBJS)) $(LIBS)
	cd bin; ln -sf libstario.so.$(MAJOR).$(MINOR).$(shell expr $(COMPILE) + $(COMPILEOFFSET) + 1) libstario.so

$(OBJS): %.o: %.c %.h $(HEADERS)
	$(dependencies)
	$(init)
	gcc -fPIC -g -c -Wall $(DEFS) -o bin/$@ $<

installer: installer.in
	$(init)
	@if [ -e installer ]; then rm -f installer; fi
	cat src/installer.in | sed -e 's/(MAJOR)/$(MAJOR)/g' | sed -e 's/(MINOR)/$(MINOR)/g' | sed -e 's/(COMPILE)/$(shell expr $(COMPILE) + $(COMPILEOFFSET))/g' > installer
	chmod +x installer

uninstaller: uninstaller.in
	$(init)
	@if [ -e uninstaller ]; then rm -f uninstaller; fi
	cp src/uninstaller.in uninstaller
	chmod +x uninstaller

libstario.spec: libstario.spec.in version
	@if [ -e src/rpm-spec/libstario.spec ]; then echo "rm -f src/rpm-spec/libstario.spec"; rm -f src/rpm-spec/libstario.spec; fi
	echo -e "%define major $(MAJOR)\n%define minor $(MINOR)\n%define compile $(shell expr $(COMPILE) + $(COMPILEOFFSET) + 1)\n" | cat - src/rpm-spec/libstario.spec.in > src/rpm-spec/libstario.spec

.PHONY:
install:
	#installing
	@if [ -z $(DESTDIR) ] && [ "$$UID" -ne "0" ]; then echo "Must be root user to install"; exit 1; fi
	@if [ ! -e $(DESTDIR)/usr/include/stario ]; then echo "mkdir -p $(DESTDIR)/usr/include/stario"; mkdir -p $(DESTDIR)/usr/include/stario; fi
	@if [ ! -e $(DESTDIR)/usr/include/stario/example ]; then echo "mkdir -p $(DESTDIR)/usr/include/stario/example"; mkdir -p $(DESTDIR)/usr/include/stario/example; fi
	cp -f src/stario.h $(DESTDIR)/usr/include/stario
	cp -f src/stario-structures.h $(DESTDIR)/usr/include/stario
	cp -f src/stario-error.h $(DESTDIR)/usr/include/stario
	cp -f bin/teststario $(DESTDIR)/usr/include/stario/example
	cp -f src/teststario.c $(DESTDIR)/usr/include/stario/example
	@if [ ! -e $(DESTDIR)/usr/lib ]; then echo "mkdir -p $(DESTDIR)/usr/lib"; mkdir -p $(DESTDIR)/usr/lib; fi
	cp -f bin/libstario.so.$(MAJOR).$(MINOR).* $(DESTDIR)/usr/lib
	@if [ -z $(RPMBUILD) ]; then echo "ldconfig"; ldconfig; fi
	@if [ -z $(RPMBUILD) ]; then echo "ln -sf /usr/lib/libstario.so.$(MAJOR) /usr/lib/libstario.so"; ln -sf /usr/lib/libstario.so.$(MAJOR) /usr/lib/libstario.so; fi

.PHONY:
remove:
	#removing
	@if [ -z $(DESTDIR) ] && [ "$$UID" -ne "0" ]; then echo "Must be root user to uninstall"; exit 1; fi
	rm -f $(DESTDIR)/usr/lib/libstario.so*
	ldconfig
	rm -rf $(DESTDIR)/usr/include/stario

.PHONY: clean
clean:
	# cleaning
	$(sweep)

.PHONY: help
help:
	# Help for libstario make file usage
	#
	# command          purpose
	# ------------------------------------
	# make              compile all sources and create libstario.so library
	#                   (version number incremented automatically)
	#
	# make install      install libstario.so and its development header files
	#                   [require root user permissions]
	#
	# make remove       removes installed files from your system
	#                   [requires root user permissions]
	#
	# make installer     create installer shell script
	# make uninstaller   create uninstaller shell script
	#
	# make clean        deletes all compiled files and their folders

