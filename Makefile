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
$(EXEC): $(OUTDIR)/socket.o
	cd $(OUTDIR); g++ -o $@ $(MAKEFLAGS) main_$@.o socket.o
	strip $(OUTDIR)/$@

#server: $(OUTDIR)/main_server.o $(OUTDIR)/socket.o
#	cd $(OUTDIR); g++ -o $@ $(MAKEFLAGS) main_server.o socket.o
#	strip $(OUTDIR)/$@

################
# Object files #
################

$(OUTDIR)/%.o: %.cpp socket.h
	@mkdir -p $(OUTDIR)
	g++ -c $(MAKEFLAGS) $< -o $@

clean:
	rm -rf bin/*

ctags:
	ctags *.cpp *.h
