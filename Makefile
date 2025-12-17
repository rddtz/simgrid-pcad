CPPC=g++
CPPFLAGS=-shared -Wall -Werror -fPIC
LIBS=simgrid

.PHONY: clean platform

export PKG_CONFIG_PATH := /opt/simgrid/lib/pkgconfig:$(PKG_CONFIG_PATH) # export installation path to simgrid

pcad:
	g++ -shared -fPIC -o pcad.so create-pcad.cpp `pkg-config --cflags --libs $(LIBS)`

base:
	g++ -shared -fPIC -o libgriffon_platform.so base.cpp `pkg-config --cflags --libs $(LIBS)`

clean:
	rm -f *.so
