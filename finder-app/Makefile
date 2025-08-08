# Define variables
CROSS_COMPILE ?=
CC=$(CROSS_COMPILE)gcc
CFLAGS= -Wall
TARGET= writer
SOURCES= writer.c
OBJ= $(SOURCES:.c=.o)

#Default target
all: $(TARGET)

#Build target from object files
# $^ represents the right side of the :, in this case all the object files
# $@ represents the name of the target, int this case writer
$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

#Build object files from source code
# $< represents the prerequisites of the target, in this case, the source files per object file
$(OBJ) : %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

#Clean target
clean:
	rm -f $(TARGET) $(OBJ)