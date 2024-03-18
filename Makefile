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
OBJ = spchk.o
EXEC = spchk

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ)

spchk.o: spchk.c spchk.h
	$(CC) $(CFLAGS) -c spchk.c

clean:
	rm -f $(EXEC) $(OBJ)

.PHONY: all clean
