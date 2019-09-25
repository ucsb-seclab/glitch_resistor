src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = 

example: $(obj)
	$(CC) $(CC_FLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) example
