HB_CFLAGS=$(shell pkg-config harfbuzz --cflags) $(shell freetype-config --cflags) $(shell pkg-config icu-uc --cflags)
HB_LDFLAGS=$(shell pkg-config harfbuzz --libs) $(shell freetype-config --libs) $(shell pkg-config icu-uc --libs)
CXXFLAGS := $(CXXFLAGS) # inherit from env
LDFLAGS := $(LDFLAGS) # inherit from env

all: symbolizer-expressions

symbolizer-expressions: main.cpp Makefile
	$(CXX) -o ./symbolizer-expressions main.cpp $(CXXFLAGS) $(LDFLAGS) `mapnik-config --all-flags`

test: symbolizer-expressions
	./symbolizer-expressions 1000000

clean:
	@rm -f ./symbolizer-expressions

.PHONY: test
