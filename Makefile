# CC = gcc
# CFLAGS = -Wall -Werror -g
# TARGET = spchk
# SRCS = spchk.c
# OBJS = $(SRCS:.c=.o)

# all: $(TARGET)

# $(TARGET): $(OBJS)
# 	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# .c.o:
# 	$(CC) $(CFLAGS) -c $< -o $@

# clean:
# 	rm -f $(OBJS) $(TARGET)

# .PHONY: all clean

CC = gcc
CFLAGS = -Wall -g
OBJ = main.o
EXEC = spchk

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ)

main.o: main.c spchk.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(EXEC) $(OBJ)

.PHONY: all clean
