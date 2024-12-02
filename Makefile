cc = gcc -g
CC = g++ -g

# Define object files
OBJECTS = producer.o shared_memory.o

all:producer

# Compile shared_memory.c into an object file
shared_memory.o: shared_memory.cpp shared_memory.h
	$(cc) -c shared_memory.cpp

# Compile producer.cpp into an object file
producer.o: producer.cpp shared_memory.h
	$(CC) -c producer.cpp

# Link the object files into an executable
producer: $(OBJECTS)
	$(CC) -o producer $(OBJECTS) -lrt -pthread

# Clean the generated files
clean:
	rm -f producer $(OBJECTS)