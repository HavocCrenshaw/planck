CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Wpedantic -Werror -pedantic-errors -O3
SRC := planck.c
OUT := planck

build:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -rf $(OUT)
