all: UnixLs.c
	gcc -g -Wall -o UnixLs UnixLs.c
clean:
	$(RM) UnixLs
