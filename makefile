
CC=gcc 
STANDARD= -std=c99
CFLAGS= $(STANDARD) -g -D_POSIX_SOURCE -Wall -Werror -pedantic -I.

VAL_OPTS= -v --leak-check=full --log-file=log

EXEC=lab02
DEPS= vash.h va_utils.h list.h context.h command.h
OBJ= $(EXEC).o vash.o va_utils.o list.o context.o command.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 

.PHONY: 
	run weak standard clean

run: $(EXEC)
	./$(EXEC)

weak: $(EXEC)
	splint -weak *.c | less

standard: $(EXEC)
	splint -standard *.c | less

grind: $(EXEC)
	valgrind $(VAL_OPTS) ./$(EXEC)

clean:
	rm *.o $(EXEC)
