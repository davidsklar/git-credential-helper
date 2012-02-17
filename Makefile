CC=gcc
CFLAGS=-Wall -Wextra -Werror -O2

INCS:=$(shell pkg-config --cflags gnome-keyring-1)
LIBS:=$(shell pkg-config --libs gnome-keyring-1)

SRCS:=credential-keyring.c
OBJS:=$(SRCS:.c=.o)

EXE=git-credential-keyring

all: $(EXE)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCS) -o $@ -c $<

$(EXE): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

clean:
	@rm -vf *.o
	@rm -vf $(EXE)
