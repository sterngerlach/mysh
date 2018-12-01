
# 情報工学科3年 学籍番号61610117 杉浦 圭祐
# Makefile

CC = gcc
CFLAGS = -MMD -MP -Os -Wall -Wextra
LDFLAGS =

OBJDIR = ./obj
OUTDIR = .

TARGET = $(OUTDIR)/mysh
SOURCES = $(notdir $(wildcard ./*.c))
OBJECTS = $(addprefix $(OBJDIR)/, $(notdir $(SOURCES:.c=.o)))
DEPENDS = $(OBJECTS:.o=.d)

default: $(TARGET)

debug: CFLAGS += -ggdb
debug: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJECTS) $(DEPENDS)

.PHONY: clean
.PHONY: debug

-include $(DEPENDS)

