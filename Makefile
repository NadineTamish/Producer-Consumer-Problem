cc = gcc -g
CC = g++ -g

all:producer consumer

# Compile producer.cpp into an object file
producer.o: producer.cpp
	$(CC) -c producer.cpp

# Compile consumer.cpp into an object file
consumer.o: consumer.cpp
	$(CC) -c consumer.cpp

# Link the object files into an executable
producer: producer.o
	$(CC) -o producer producer.o

consumer: consumer.o
	$(CC) -o consumer consumer.o

# Clean the generated files
clean:
	rm -f producer producer.o
	rm -f consumer consumer.o