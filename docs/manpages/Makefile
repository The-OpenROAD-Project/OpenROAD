# Makefile for generating man pages using pandoc

# Define variables
PANDOC = pandoc
SRC_DIR = src

MAN1_DIR = man1
MAN1_FILES = $(wildcard $(SRC_DIR)/$(MAN1_DIR)/*.md)
MAN1_PAGES = $(patsubst $(SRC_DIR)/$(MAN1_DIR)/%.md,$(MAN1_DIR)/%.1,$(MAN1_FILES))
MAN1_HTML = $(patsubst $(SRC_DIR)/$(MAN1_DIR)/%.md,$(MAN1_DIR)/%.html,$(MAN1_FILES))

MAN2_DIR = man2
MAN2_FILES = $(wildcard $(SRC_DIR)/$(MAN2_DIR)/*.md)
MAN2_PAGES = $(patsubst $(SRC_DIR)/$(MAN2_DIR)/%.md,$(MAN2_DIR)/%.2,$(MAN2_FILES))
MAN2_HTML = $(patsubst $(SRC_DIR)/$(MAN2_DIR)/%.md,$(MAN2_DIR)/%.html,$(MAN2_FILES))

MAN3_DIR = man3
MAN3_FILES = $(wildcard $(SRC_DIR)/$(MAN3_DIR)/*.md)
MAN3_PAGES = $(patsubst $(SRC_DIR)/$(MAN3_DIR)/%.md,$(MAN3_DIR)/%.3,$(MAN3_FILES))
MAN3_HTML = $(patsubst $(SRC_DIR)/$(MAN3_DIR)/%.md,$(MAN3_DIR)/%.html,$(MAN3_FILES))

# Default target
all: doc html

# Target to generate all man pages
doc: $(MAN1_PAGES) $(MAN2_PAGES) $(MAN3_PAGES)

# Target to generate all html pages
html: $(MAN1_HTML) $(MAN2_HTML) $(MAN3_HTML)

# Rule to create the man directory
$(MAN1_DIR):
	mkdir -p $(MAN1_DIR)
$(MAN2_DIR):
	mkdir -p $(MAN2_DIR)
$(MAN3_DIR):
	mkdir -p $(MAN3_DIR)

# Rule to generate a man page from a corresponding Markdown file
$(MAN1_DIR)/%.1: $(SRC_DIR)/$(MAN1_DIR)/%.md | $(MAN1_DIR)
	$(PANDOC) -s -t man $< -o $@
$(MAN2_DIR)/%.2: $(SRC_DIR)/$(MAN2_DIR)/%.md | $(MAN2_DIR)
	$(PANDOC) -s -t man $< -o $@
$(MAN3_DIR)/%.3: $(SRC_DIR)/$(MAN3_DIR)/%.md | $(MAN3_DIR)
	$(PANDOC) -s -t man $< -o $@

# Rule to generate a html from a corresponding Markdown file
$(MAN1_DIR)/%.html: $(SRC_DIR)/$(MAN1_DIR)/%.md | $(MAN1_DIR)
	$(PANDOC) -s -o html $< -o $@
$(MAN2_DIR)/%.html: $(SRC_DIR)/$(MAN2_DIR)/%.md | $(MAN2_DIR)
	$(PANDOC) -s -o html $< -o $@
$(MAN3_DIR)/%.html: $(SRC_DIR)/$(MAN3_DIR)/%.md | $(MAN3_DIR)
	$(PANDOC) -s -o html $< -o $@

# Clean target to remove all generated man pages
clean:
	rm -rf $(MAN1_DIR) $(MAN2_DIR) $(MAN3_DIR)

# Phony targets
.PHONY: all doc clean