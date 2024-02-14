# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Werror

# Libraries
LIBS = -lpthread

# Source files
SRCS = stalk.c

# Object files
OBJS = list.o

# Executable
TARGET = stalk

all: $(TARGET)

$(TARGET): $(SRCS) $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) $(OBJS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)