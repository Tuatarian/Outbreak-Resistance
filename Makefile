all:
	gcc main.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
clean:
	rm ./a.out
