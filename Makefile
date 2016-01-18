CFLAGS = -g -Wall -std=c++11
EXEC = client server
OUTDIR = bin/release

###############
# Debug rules #
###############

ifdef DEBUG
OUTDIR = bin/debug
CFLAGS += -DDEBUG
endif

###############
# Libraries #
###############

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
LIBFLAGS = -lncurses
else
LIBFLAGS = -lncursesw -pthread
endif
LIBFLABS += -lcrypto -lssl

###############
# PHONY rules #
###############
.PHONY: all

all: client server

##############
# Executable #
##############
client: MAKEFLAGS = $(CFLAGS)
client: $(OUTDIR)/main_client.o
server: MAKEFLAGS = $(CFLAGS)
server: $(OUTDIR)/main_server.o
$(EXEC): $(OUTDIR)/util.o
	cd $(OUTDIR); g++ -o $@ $(MAKEFLAGS) main_$@.o util.o $(LIBFLAGS)
	strip $(OUTDIR)/$@

#server: $(OUTDIR)/main_server.o $(OUTDIR)/util.o
#	cd $(OUTDIR); g++ -o $@ $(MAKEFLAGS) main_server.o util.o
#	strip $(OUTDIR)/$@

################
# Object files #
################

$(OUTDIR)/%.o: %.cpp util.h
	@mkdir -p $(OUTDIR)
	g++ -c $(MAKEFLAGS) $< -o $@

clean:
	rm -rf bin/*

ctags:
	ctags *.cpp *.h
