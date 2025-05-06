all:
	gcc main.c src/kernel_loader.c src/errors.c src/wav.c -o main.exe -Iinclude -lOpenCl -w -g
	gcc runner.c -o runner.exe
graph:
	gcc graph.c -o graph.exe -Iinclude -lPlplot