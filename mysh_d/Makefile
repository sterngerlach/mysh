
# 情報工学科3年 学籍番号61610117 杉浦 圭祐
# Makefile

CC = gcc
CFLAGS = -MMD -MP -Os -Wall -Wextra
LDFLAGS =

TARGET = mysh
SOURCES = $(notdir $(wildcard ./*.c))
OBJECTS = $(notdir $(SOURCES:.c=.o))
DEPENDS = $(OBJECTS:.o=.d)

default: $(TARGET) clean

debug: CFLAGS += -ggdb
debug: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
.PHONY: debug

clean:
	rm -f $(OBJECTS) $(DEPENDS)

-include $(DEPENDS)

