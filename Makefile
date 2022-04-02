BASE_DIR := .
OUTPUT_DIR := $(BASE_DIR)/build

INCLUDES := $(INCLUDES) -I ./

CSRCS := $(wildcard *.c)
OBJS  := $(CSRCS:%.c=$(OUTPUT_DIR)/%.o)

stc8prog: $(OBJS)
	@ echo "\e[34mMKELF\e[0m	" $@
	@ gcc -o $@ $^ -g

install:
	@ cp stc8prog /usr/local/bin

clean:
	@ rm -f $(OBJS) stc8prog

$(OUTPUT_DIR)/%.o: %.c
	@ mkdir -p $(OUTPUT_DIR)
	@ echo "\e[32mCC\e[0m	" $@
	@ gcc $(INCLUDES) -o "$@" -c "$<" -g -Wall -Wextra -Werror

$(OUTPUT_DIR)/stc8prog.o: $(wildcard *.h)
