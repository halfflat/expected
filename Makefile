# set coverage variable (e.g. with coverage=true on command line)
# to build with --coverage and to generate reports with test target

.PHONY: all test clean realclean
.SECONDARY:

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

all:: unit

test-src:=unit.cc test_expected.cc test_unexpected.cc

all-src:=$(test-src)
all-obj:=$(patsubst %.cc, %.o, $(all-src))

gtest-top:=$(top)test/googletest/googletest
gtest-inc:=$(gtest-top)/include
gtest-src:=$(gtest-top)/src/gtest-all.cc

vpath %.cc $(top)test

CXXSTD?=c++17
OPTFLAGS?=-O1
CXXFLAGS+=$(OPTFLAGS) -MMD -MP -std=$(CXXSTD) -pedantic -Wall -Wextra -g -pthread
CPPFLAGS+=-isystem $(gtest-inc) -I $(top)include

depends:=$(patsubst %.cc, %.d, $(all-src)) gtest.d
-include $(depends)

gtest.o: CPPFLAGS+=-I $(gtest-top)
gtest.o: ${gtest-src}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

test-obj:=$(patsubst %.cc, %.o, $(test-src))
test-gcno:=$(patsubst %.cc, %.gcno, $(test-src))
test-gcda:=$(patsubst %.cc, %.gcda, $(test-src))

ifdef coverage
test_%.o: test_%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) --keep-inline-functions --coverage --param=max-vartrack-size=100000000 -o $@ -c $<
else
test_%.o: test_%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<
endif

unit: $(test-obj) gtest.o
ifdef coverage
	$(CXX) $(CXXFLAGS) --coverage -o $@ $^ $(LDFLAGS) $(LDLIBS)
else
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
endif

ifdef coverage
test: unit
	@rm -f $(test-gcda)
	@./unit
	@if command -v gcovr &>/dev/null; then \
	    echo 'Generating gcovr detailed report: coverage.expected.h.html'; \
	    gcovr --html-self-contained --html-details --root '$(top)' --filter '$(top)include/backport/' -o _gcovr; \
	    rm -f _gcovr _gcovr.functions.html; \
	    mv _gcovr.expected.h.[a-f0-9]*.html coverage.expected.h.html; \
	fi
else
test: unit
	@./unit
endif

clean:
	rm -f $(all-obj) $(test-gcno) $(test-gcda)

realclean: clean
	rm -f unit $(examples) gtest.o $(depends) coverage.expected.h.html
