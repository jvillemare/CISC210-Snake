snake: snake.o framebuffer.o
	cc -o snake snake.o -L ../Downloads/libsense_p/lib -lsense -lm

snake.o: snake.c
	cc -c snake.c framebuffer.h -I ../Downloads/libsense_p/include -lsense -lm -g

clean:
	rm snake.o
