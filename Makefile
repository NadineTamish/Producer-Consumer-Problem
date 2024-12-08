cc = gcc -g
CC = g++ -g

# Define object files
OBJECTS1 = producer.o shared_memory.o
OBJECTS2 = consumer.o shared_memory.o

all:producer

# Compile shared_memory.c into an object file
shared_memory.o: shared_memory.cpp shared_memory.h
	$(cc) -c shared_memory.cpp

# Compile producer.cpp into an object file
producer.o: producer.cpp shared_memory.h
	$(CC) -c producer.cpp

# Compile consumer.cpp into an object file
consumer.o: consumer.cpp shared_memory.h
	$(CC) -c consumer.cpp

# Link the object files into an executable
producer: $(OBJECTS1)
	$(CC) -o producer $(OBJECTS1) -lrt -pthread

consumer: $(OBJECTS2)
	$(CC) -o consumer $(OBJECTS2) -lrt -pthread

# Clean the generated files
clean:
	rm -f producer $(OBJECTS1)
	rm -f consumer $(OBJECTS2)