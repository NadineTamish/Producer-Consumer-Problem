cc = gcc -g
CC = g++ -g

all:producer

producer: producer.cpp
	$(CC) -o producer producer.cpp -lrt


clean:
	rm -f producer
