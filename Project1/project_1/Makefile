SOURCE	:= Main.c Parser.c Registers.c
CC	:= gcc
TARGET	:= RVSim

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(SOURCE) -lm

clean:
	rm -f $(TARGET)
