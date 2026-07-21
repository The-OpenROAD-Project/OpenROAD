# Makefile is divided into two parts:
# Pandoc compilation - Manpages
# Sphinx compilation - ReadTheDocs

#  _____        _   _ _____   ____   _____ 
# |  __ \ /\   | \ | |  __ \ / __ \ / ____|
# | |__) /  \  |  \| | |  | | |  | | |     
# |  ___/ /\ \ | . ` | |  | | |  | | |     
# | |  / ____ \| |\  | |__| | |__| | |____ 
# |_| /_/    \_\_| \_|_____/ \____/ \_____|

# Render the man pages in a UTF-8 locale so groff/preconv and col treat page
# bytes as UTF-8 rather than Latin-1. Without this, non-ASCII characters in the
# READMEs (e.g. accented author names) render as mojibake or trigger troff
# "character with input code N not defined" warnings on the 7-bit ascii device.
# Detect an installed UTF-8 locale instead of hard-coding one (C.UTF-8 is not
# present on every host, e.g. stock macOS); fall back to the user's locale if
# none is found rather than exporting an invalid LC_ALL.
UTF8_LOCALE := $(shell locale -a 2>/dev/null | grep -iE '^(C|en_US)\.utf-?8$$' | head -1)
ifeq ($(UTF8_LOCALE),)
UTF8_LOCALE := $(shell locale -a 2>/dev/null | grep -iE 'utf-?8$$' | head -1)
endif
ifneq ($(UTF8_LOCALE),)
export LC_ALL := $(UTF8_LOCALE)
endif

# Define variables
PANDOC = pandoc
GROFF ?= groff
# Roff snippet injected into every man page to remap Pandoc's Courier-based
# code/verbatim fonts on devices that lack them, silencing troff "cannot
# select font" warnings (see the file for details).
FONTFIX = src/man_fontfix.tmac
SRC_DIR = md
MAN_ROOT_DIR = man
HTML_ROOT_DIR = html
CAT_ROOT_DIR = cat
MAN1 = man1
MAN2 = man2
MAN3 = man3

# Exclude these set of keywords
EXCLUDE_KEYWORDS = ant cts dft dpl drt fin gpl grt gui ifp mpl \
				   odb pad par pdn ppl psm rcx rmp rsz sta \
				   stt tap upf utl
EXCLUDE_FILES := $(addsuffix .md,$(EXCLUDE_KEYWORDS))

MAN1_DIR = $(MAN_ROOT_DIR)/$(MAN1)
SRC1_DIR = $(SRC_DIR)/$(MAN1)
MAN1_FILES = $(wildcard $(SRC1_DIR)/*.md)
MAN1_PAGES = $(patsubst $(SRC1_DIR)/%.md,$(MAN1_DIR)/%.1,$(MAN1_FILES))
HTML1_DIR = $(HTML_ROOT_DIR)/html1
HTML1_PAGES = $(patsubst $(SRC1_DIR)/%.md,$(HTML1_DIR)/%.html,$(MAN1_FILES))
CAT1_DIR = $(CAT_ROOT_DIR)/cat1
CAT1_PAGES = $(patsubst $(SRC1_DIR)/%.md,$(CAT1_DIR)/%.1,$(MAN1_FILES))

MAN2_DIR = $(MAN_ROOT_DIR)/$(MAN2)
SRC2_DIR = $(SRC_DIR)/$(MAN2)
MAN2_FILES = $(wildcard $(SRC2_DIR)/*.md)
MAN2_FILES := $(filter-out $(foreach keyword, $(EXCLUDE_FILES), %$(keyword)), $(MAN2_FILES))
MAN2_PAGES = $(patsubst $(SRC2_DIR)/%.md,$(MAN2_DIR)/%.2,$(MAN2_FILES))
HTML2_DIR = $(HTML_ROOT_DIR)/html2
HTML2_PAGES = $(patsubst $(SRC2_DIR)/%.md,$(HTML2_DIR)/%.html,$(MAN2_FILES))
CAT2_DIR = $(CAT_ROOT_DIR)/cat2
CAT2_PAGES = $(patsubst $(SRC2_DIR)/%.md,$(CAT2_DIR)/%.2,$(MAN2_FILES))

MAN3_DIR = $(MAN_ROOT_DIR)/$(MAN3)
SRC3_DIR = $(SRC_DIR)/$(MAN3)
MAN3_FILES = $(wildcard $(SRC3_DIR)/*.md)
MAN3_PAGES = $(patsubst $(SRC3_DIR)/%.md,$(MAN3_DIR)/%.3,$(MAN3_FILES))
HTML3_DIR = $(HTML_ROOT_DIR)/html3
HTML3_PAGES = $(patsubst $(SRC3_DIR)/%.md,$(HTML3_DIR)/%.html,$(MAN3_FILES))
CAT3_DIR = $(CAT_ROOT_DIR)/cat3
CAT3_PAGES = $(patsubst $(SRC3_DIR)/%.md,$(CAT3_DIR)/%.3,$(MAN3_FILES))

# Default target
all: doc web cat

# Target to do symlinks and pandoc-compatible conversion
preprocess:
	./src/scripts/link_readmes.sh && python3 src/scripts/md_roff_compat.py

# Target to generate all man pages
doc: $(MAN1_PAGES) $(MAN2_PAGES) $(MAN3_PAGES)

# Target to generate all web pages (changed name to disambiguate from sphinx)
web: $(HTML1_PAGES) $(HTML2_PAGES) $(HTML3_PAGES) 

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
$(HTML1_DIR): 
	mkdir -p $(HTML1_DIR)
$(HTML2_DIR): 
	mkdir -p $(HTML2_DIR)
$(HTML3_DIR): 
	mkdir -p $(HTML3_DIR)
$(CAT1_DIR): 
	mkdir -p $(CAT1_DIR)
$(CAT2_DIR): 
	mkdir -p $(CAT2_DIR)
$(CAT3_DIR): 
	mkdir -p $(CAT3_DIR)

# Rule to generate a roff file from a corresponding Markdown file
$(MAN1_DIR)/%.1: $(SRC1_DIR)/%.md $(FONTFIX) | $(MAN1_DIR)
	$(PANDOC) -s -t man --include-in-header=$(FONTFIX) $< -o $@ --quiet
$(MAN2_DIR)/%.2: $(SRC2_DIR)/%.md $(FONTFIX) | $(MAN2_DIR)
	$(PANDOC) -s -t man --include-in-header=$(FONTFIX) $< -o $@ --quiet
$(MAN3_DIR)/%.3: $(SRC3_DIR)/%.md $(FONTFIX) | $(MAN3_DIR)
	$(PANDOC) -s -t man --include-in-header=$(FONTFIX) $< -o $@ --quiet

# Rule to generate a html file from a corresponding roff file
$(HTML1_DIR)/%.html: $(MAN1_DIR)/%.1 | $(HTML1_DIR)
	$(PANDOC) -s -o html $< -o $@ --quiet
$(HTML2_DIR)/%.html: $(MAN2_DIR)/%.2 | $(HTML2_DIR)
	$(PANDOC) -s -o html $< -o $@ --quiet
$(HTML3_DIR)/%.html: $(MAN3_DIR)/%.3 | $(HTML3_DIR)
	$(PANDOC) -s -o html $< -o $@ --quiet

# Rule to generate a cat file from a corresponding roff file.
# -t runs the tbl preprocessor so the OPTIONS tables (the '\" t directive in
# the Pandoc output) render instead of warning about missing TW registers.
# -Kutf8 runs preconv to decode UTF-8 input and -Tutf8 emits UTF-8, so non-ASCII
# characters render correctly instead of warning/mangling on the ascii device.
# -rIN=4n narrows the body indent (default 7n) so Pandoc's full-width tables fit
# within the line length, avoiding "table wider than line length" warnings.
$(CAT1_DIR)/%.md: $(MAN1_DIR)/%.1 | $(CAT1_DIR)
	$(GROFF) -Kutf8 -t -man -Tutf8 -rIN=4n $< | col -b > $@
$(CAT2_DIR)/%.md: $(MAN2_DIR)/%.2 | $(CAT2_DIR)
	$(GROFF) -Kutf8 -t -man -Tutf8 -rIN=4n $< | col -b > $@
$(CAT3_DIR)/%.md: $(MAN3_DIR)/%.3 | $(CAT3_DIR)
	$(GROFF) -Kutf8 -t -man -Tutf8 -rIN=4n $< | col -b > $@
#$(PANDOC) -s -o markdown $< -o $@
#sed -i 's/\\\[/\[/g; s/\\]/\]/g; s/\\_/_/g' $@

$(CAT1_DIR)/%.1: $(CAT1_DIR)/%.md
	mv $< $@
$(CAT2_DIR)/%.2: $(CAT2_DIR)/%.md
	mv $< $@
$(CAT3_DIR)/%.3: $(CAT3_DIR)/%.md
	mv $< $@

# Single-invocation target used by //docs:man_pages (Bazel). Chains the
# preprocessing step (README symlinks + roff-compat conversion) with the
# cat and web outputs that the Bazel rule copies into its declared outputs.
bazel-manpages: preprocess cat web

# Phony targets
.PHONY: all bazel-manpages preprocess

#   _____ _____  _    _ _____ _   ___   __
#  / ____|  __ \| |  | |_   _| \ | \ \ / /
# | (___ | |__) | |__| | | | |  \| |\ V / 
#  \___ \|  ___/|  __  | | | | . ` | > <  
#  ____) | |    | |  | |_| |_| |\  |/ . \ 
# |_____/|_|    |_|  |_|_____|_| \_/_/ \_\

SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     =
BUILDDIR      = build

help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

html: Makefile
	@$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	rm -f main
	./revert-links.py

checklinks:
	$(SPHINXBUILD) -b linkcheck "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O) || echo
	@echo
	@echo "Check finished. Report is in $(BUILDDIR)."
	rm -f main
	./revert-links.py

# Clean target to remove all man pages/Sphinx docs
clean:
	rm -rf $(MAN1_DIR) $(MAN2_DIR) $(MAN3_DIR)
	rm -rf $(HTML1_DIR) $(HTML2_DIR) $(HTML3_DIR)
	rm -rf $(CAT1_DIR) $(CAT2_DIR) $(CAT3_DIR)
	rm -rf ./md/man2/*md
	rm -rf ./md/man3/*md
	rm -rf $(BUILDDIR)
