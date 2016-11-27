include Makefile.inc

all: vtpin libxfamily vtpin_test inheritance_test inheritance_test_so

vtpin: $(SRCDIR)/vtpin.cxx
	$(CXX) $(SRCDIR)/vtpin.cxx $(CXXEXTRAS) $(COMMONFLAGS) -I $(SRCDIR) $(DEBUG) -fPIC -shared \
					-o $(BINDIR)/$(OUT_LIBTVPIN) \
					$(LDFLAGS) -ldl #-fnon-call-exceptions

libxfamily: $(SRCDIR)/xfamily.cxx
	$(CXX) $(SRCDIR)/xfamily.cxx $(CXXEXTRAS) $(COMMONFLAGS) $(DEBUG) -fPIC -shared \
					-o $(BINDIR)/libxfamily.so \
					$(LDFLAGS) -ldl

vtpin_test: $(SRCDIR)/vtpin_test.cxx
	$(CXX) $(SRCDIR)/vtpin_test.cxx $(CXXEXTRAS) $(COMMONFLAGS) $(DEBUG) -L./$(BINDIR) \
					-o $(BINDIR)/vtpin_test -lxfamily \
					$(LDFLAGS) #-fnon-call-exceptions

inheritance_test: $(SRCDIR)/inheritance_test.cxx
	$(CXX) $(SRCDIR)/inheritance_test.cxx $(CXXEXTRAS) $(COMMONFLAGS) $(DEBUG) -L./$(BINDIR) \
					-o $(BINDIR)/inheritance_test \
					$(LDFLAGS) #-fnon-call-exceptions

inheritance_test_so: $(SRCDIR)/inheritance_test.cxx
	$(CXX) $(SRCDIR)/inheritance_test.cxx $(CXXEXTRAS) $(COMMONFLAGS) $(DEBUG) -L./$(BINDIR) -fPIC -shared \
					-o $(BINDIR)/libinheritance_test.so -lxfamily \
					$(LDFLAGS) -ldl #-fnon-call-exceptions

clean:
	rm -f $(BINDIR)/libvtpin.so         \
	      $(BINDIR)/libvtpin.dylib      \
	      $(BINDIR)/vtpin_test          \
	      $(BINDIR)/libxfamily.so       \
	      $(BINDIR)/inheritance_test    \
	      $(BINDIR)/libinheritance_test.so
