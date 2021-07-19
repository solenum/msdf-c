CC =gcc
FLAGS 	=-g -lm -std=c99 -O2

all: msdf

msdf:
	gcc main.c msdf.c msdf.h -o msdf_gen $(FLAGS)