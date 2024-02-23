# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Werror

# Libraries
LIBS = -lpthread

# Source files
SRCS = s-talk.c

# Object files
OBJS = list.o

# Executable
TARGET = s-talk

all: $(TARGET)

$(TARGET): $(SRCS) $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) $(OBJS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)