all: pixelDrawer.c
	gcc -Wall pixelDrawer.c -o pixelDrawer.exe -lgdi32