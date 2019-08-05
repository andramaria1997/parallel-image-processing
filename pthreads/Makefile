all: task1 task2

task1: task1.c task1.h main.c
	gcc -o task1 task1.c main.c -lpthread -Wall -lm

task2: task2.c task2.h main1.c
	gcc -o task2 task2.c main1.c -lpthread -Wall -lm

.PHONY: clean
clean:
	rm -f task1
	rm -f task2
	rm -rf out/*
