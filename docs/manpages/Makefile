# Makefile for generating man pages using pandoc

# Define variables
PANDOC = pandoc
SRC_DIR = src
MAN_DIR = man1
SRC_FILES = $(wildcard $(SRC_DIR)/*.md)
MAN_PAGES = $(patsubst $(SRC_DIR)/%.md,$(MAN_DIR)/%.1,$(SRC_FILES))

# Default target
all: doc

# Target to generate all man pages
doc: $(MAN_PAGES)

# Rule to create the man directory
$(MAN_DIR):
	mkdir -p $(MAN_DIR)

# Rule to generate a man page from a corresponding Markdown file
$(MAN_DIR)/%.1: $(SRC_DIR)/%.md | $(MAN_DIR)
	$(PANDOC) -s -t man $< -o $@

# Clean target to remove all generated man pages
clean:
	rm -rf $(MAN_DIR)

# Phony targets
.PHONY: all doc clean

