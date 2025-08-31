CC      := gcc
CFLAGS  := -Wall -pedantic -O2 -Iinclude
LDFLAGS := -lm -lSDL2 -lSDL2_image -lcairo -lcurl -lcjson

.PHONY: clean all

all:
	@if [ ! -d "bin" ]; then mkdir bin; fi
	make bin/gprop

bin/gprop: bin/gprop.o bin/chunk.o bin/search.o bin/tile.o
	${CC} $^ ${LDFLAGS} -o $@

bin/chunk.o: src/chunk.c include/chunk.h
	${CC} ${CFLAGS} -c $< -o $@

bin/search.o: src/search.c include/search.h include/chunk.h include/config.h
	${CC} ${CFLAGS} -c $< -o $@

bin/tile.o: src/tile.c include/tile.h include/chunk.h include/config.h
	${CC} ${CFLAGS} -c $< -o $@

bin/gprop.o: src/gprop.c
	${CC} ${CFLAGS} -c $< -o $@

clean:
	rm -rf bin