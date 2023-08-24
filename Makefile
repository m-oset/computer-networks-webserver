# Marcelina Oset 313940

CXXFLAGS = -std=gnu++17 -Wall -Wextra
CC = g++

all: webserver

traceroute: webserver.o

clean:
	rm -f webserver.o

distclean:
	rm -f webserver.o webserver