
ARFLAGS = cr
LDFLAGS = -L. -lloot_library -Lcubiomes/ -lcubiomes -lm
CFLAGS = -Wall -Wno-unused -Wextra -fwrapv

.PHONY : all clean

all: main

main: main.o libloot_library.a
	gcc $(LDFLAGS) $^ -o main

main.o: main.c
	$(CC) -c $(CFLAGS) $<

libloot_library.a: cJSON.o loot_functions.o loot_table_context.o loot_table_parser.o
	$(AR) $(ARFLAGS) libloot_library.a $^

cJSON.o: cjson/cJSON.c cjson/cJSON.h
	$(CC) -c $(CFLAGS) $<

loot_functions.o: lib/loot_functions.c lib/loot_functions.h
	$(CC) -c $(CFLAGS) $<

loot_table_context.o: lib/loot_table_context.c lib/loot_table_context.h
	$(CC) -c $(CFLAGS) $<

loot_table_parser.o: lib/loot_table_parser.c
	$(CC) -c $(CFLAGS) $<

clean:
	$(RM) *.o *.a
