src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = 

example: $(obj)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) example
