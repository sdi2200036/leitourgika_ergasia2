CC      = gcc
LDFLAGS =
TARGETS = heavy daemon assign_ncores_to_group assign_process_to_group

build: $(TARGETS)

all: build

heavy: LDFLAGS += -lpthread

%: %.c
	$(CC) -o $@ $< $(LDFLAGS)

test:
	@./script.sh

clean:
	rm -f $(TARGETS)

