CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include -g
SRCS = src/main.c src/producer.c src/consumer.c src/utils.c src/profiling.c
OBJS = $(SRCS:src/%.c=obj/%.o)
TARGET = sports_analyzer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)