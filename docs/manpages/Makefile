# Makefile for generating man pages using pandoc

# Define variables
PANDOC = pandoc
SRC_DIR = src

MAN1_DIR = man1
MAN1_FILES = $(wildcard $(SRC_DIR)/$(MAN1_DIR)/*.md)
MAN1_PAGES = $(patsubst $(SRC_DIR)/$(MAN1_DIR)/%.md,$(MAN1_DIR)/%.1,$(MAN1_FILES))
MAN1_HTML = $(patsubst $(SRC_DIR)/$(MAN1_DIR)/%.md,$(MAN1_DIR)/%.html,$(MAN1_FILES))
CAT1_DIR = cat1
CAT1_PAGES = $(patsubst $(SRC_DIR)/$(MAN1_DIR)/%.md,$(CAT1_DIR)/%.cat,$(MAN1_FILES))

MAN2_DIR = man2
MAN2_FILES = $(wildcard $(SRC_DIR)/$(MAN2_DIR)/*.md)
MAN2_PAGES = $(patsubst $(SRC_DIR)/$(MAN2_DIR)/%.md,$(MAN2_DIR)/%.2,$(MAN2_FILES))
MAN2_HTML = $(patsubst $(SRC_DIR)/$(MAN2_DIR)/%.md,$(MAN2_DIR)/%.html,$(MAN2_FILES))
CAT2_DIR = cat2
CAT2_PAGES = $(patsubst $(SRC_DIR)/$(MAN2_DIR)/%.md,$(CAT2_DIR)/%.cat,$(MAN2_FILES))

MAN3_DIR = man3
MAN3_FILES = $(wildcard $(SRC_DIR)/$(MAN3_DIR)/*.md)
MAN3_PAGES = $(patsubst $(SRC_DIR)/$(MAN3_DIR)/%.md,$(MAN3_DIR)/%.3,$(MAN3_FILES))
MAN3_HTML = $(patsubst $(SRC_DIR)/$(MAN3_DIR)/%.md,$(MAN3_DIR)/%.html,$(MAN3_FILES))
CAT3_DIR = cat3
CAT3_PAGES = $(patsubst $(SRC_DIR)/$(MAN3_DIR)/%.md,$(CAT3_DIR)/%.cat,$(MAN3_FILES))

# Default target
all: doc html cat

# Target to generate all man pages
doc: $(MAN1_PAGES) $(MAN2_PAGES) $(MAN3_PAGES)

# Target to generate all html pages
html: $(MAN1_HTML) $(MAN2_HTML) $(MAN3_HTML)

# Target to generate all cat pages
cat: $(CAT1_PAGES) $(CAT2_PAGES) $(CAT3_PAGES)
	@echo $(CAT1_PAGES)

# Rule to create the man directory
$(MAN1_DIR): 
	mkdir -p $(MAN1_DIR) 
$(MAN2_DIR): 
	mkdir -p $(MAN2_DIR) 
$(MAN3_DIR): 
	mkdir -p $(MAN3_DIR) 
$(CAT1_DIR): 
	mkdir -p $(CAT1_DIR)
$(CAT2_DIR): 
	mkdir -p $(CAT2_DIR)
$(CAT3_DIR): 
	mkdir -p $(CAT3_DIR)

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

# Rule to generate a cat file
$(CAT1_DIR)/%.cat: $(SRC_DIR)/$(MAN1_DIR)/%.md | $(CAT1_DIR)
	cp $< $@
$(CAT2_DIR)/%.cat: $(SRC_DIR)/$(MAN2_DIR)/%.md | $(CAT2_DIR)
	cp $< $@
$(CAT3_DIR)/%.cat: $(SRC_DIR)/$(MAN3_DIR)/%.md | $(CAT3_DIR)
	cp $< $@

# Clean target to remove all generated man pages
clean:
	rm -rf $(MAN1_DIR) $(MAN2_DIR) $(MAN3_DIR) $(CAT1_DIR) $(CAT2_DIR) $(CAT3_DIR)

# Phony targets
.PHONY: all 