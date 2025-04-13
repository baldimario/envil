# Compiler
CC = gcc

# Docker
DC = docker compose -f docker-compose.y*ml

# Compiler flags
CFLAGS = -Wall -Wextra -Wsign-compare -Iinclude -Iconfig -I/usr/include/json-c -I/usr/include
LDFLAGS = -lm -lyaml -ljson-c

# Directories
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include
BIN_DIR = bin
CONFIG_DIR = config
TEST_DIR = test

# Source Files
SRCS = $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)

# Object Files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)

# Excecutable name
EXEC = $(BIN_DIR)/envil
TEST_EXEC = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BIN_DIR)/test_%)

# Default target
all: $(EXEC)

# Link object files to create the executable
$(EXEC): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

# Compile test source files into object files
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile and run tests
test: $(TEST_EXEC)
	for test in $(TEST_EXEC); do ./$$test; done

$(BIN_DIR)/test_%: $(OBJ_DIR)/test_%.o $(filter-out $(OBJ_DIR)/envil.o, $(OBJS)) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(filter-out $(OBJ_DIR)/envil.o, $(OBJS)) -o $@ $(LDFLAGS)

# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(EXEC)

up:
	$(DC) --profile all up -d

env:
	$(DC) --profile env up -d

build:
	$(DC) build $(SERVICE)

down:
	$(DC) --profile all down

ps:
	$(DC) ps

shell:
	$(DC) exec $(SERVICE) /bin/sh

run-shell:
	$(DC) run --rm --entrypoint "/bin/sh -c" $(SERVICE) sh

logs:
	$(DC) logs -f $(SERVICE)

run:
	./bin/envil

repl: clean all run

install:
	install -m 755 $(EXEC) /usr/local/bin/
	@echo "Installed envil to /usr/local/bin/"
	@mkdir -p /etc/bash_completion.d
	./bin/envil -C bash > /etc/bash_completion.d/envil
	@echo "Installed bash completion to /etc/bash_completion.d/envil"
	@mkdir -p /usr/share/zsh/site-functions
	./bin/envil -C zsh > /usr/share/zsh/site-functions/_envil
	@echo "Installed zsh completion to /usr/share/zsh/site-functions/_envil"
	@mkdir -p /usr/share/man/man1
	gzip -c man/man1/envil.1 > /usr/share/man/man1/envil.1.gz
	@echo "Installed man page to /usr/share/man/man1/envil.1.gz"
	@mandb -q

install-user:
	install -m 755 $(EXEC) ~/.local/bin/
	@echo "Installed envil to ~/.local/bin/"
	@mkdir -p ~/.bash_completion.d
	./bin/envil -C bash > ~/.bash_completion.d/envil
	@echo "Installed bash completion to ~/.bash_completion.d/envil"
	@mkdir -p ~/.zsh/completions
	./bin/envil -C zsh > ~/.zsh/completions/_envil
	@echo "Installed zsh completion to ~/.zsh/completions/_envil"
	@mkdir -p ~/.local/share/man/man1
	gzip -c man/man1/envil.1 > ~/.local/share/man/man1/envil.1.gz
	@echo "Installed man page to ~/.local/share/man/man1/envil.1.gz"
	@echo "Add this to your .zshrc to enable user completions:"
	@echo "fpath=(~/.zsh/completions \$$fpath)"
	@echo "Add this to your shell config to enable user man pages:"
	@echo "export MANPATH=\$$MANPATH:~/.local/share/man"

uninstall:
	rm -f /usr/local/bin/envil
	rm -f /etc/bash_completion.d/envil
	rm -f /usr/share/zsh/site-functions/_envil
	rm -f /usr/share/man/man1/envil.1.gz
	@mandb -q
	@echo "Uninstalled envil, completion scripts, and man page"

uninstall-user:
	rm -f ~/.local/bin/envil
	rm -f ~/.bash_completion.d/envil
	rm -f ~/.zsh/completions/_envil
	rm -f ~/.local/share/man/man1/envil.1.gz
	@echo "Uninstalled envil, completion scripts, and man page from user directories"

# Phony targets
.PHONY: all clean up env down build logs test repl install install-user uninstall uninstall-user