// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Tcl command completion for the web console.
// Provides tab-completion for commands, arguments (-flags), and variables ($).

const kBoundaryChars = ' \t\n\r[]{}';
const kMinPrefixLength = 2;
const kDebounceMs = 100;
const kMaxVisible = 15;

export class TclCompleter {
    constructor(inputEl, websocketManager) {
        this._input = inputEl;
        this._ws = websocketManager;
        this._popup = null;
        this._items = [];
        this._selectedIndex = -1;
        this._replaceStart = 0;
        this._replaceEnd = 0;
        this._debounceTimer = null;
        this._commandCache = null;
        this._history = [];
        this._historyIndex = -1;
        this._historyStash = '';

        this._createPopup();
        this._onInputBound = this._onInput.bind(this);
        this._onBlurBound = this._onBlur.bind(this);
        this._input.addEventListener('input', this._onInputBound);
        this._input.addEventListener('blur', this._onBlurBound);
    }

    _createPopup() {
        this._popup = document.createElement('div');
        this._popup.className = 'tcl-complete-popup';
        this._popup.style.display = 'none';
        // Insert into the input row so absolute positioning works
        this._input.parentElement.appendChild(this._popup);
    }

    // Returns true if the key was consumed by the completer.
    handleKeyDown(e) {
        const popupVisible = this._popup.style.display !== 'none';

        if (e.key === 'Tab') {
            e.preventDefault();
            if (popupVisible) {
                this._acceptCompletion();
            } else {
                this._requestCompletions(
                    this._input.value,
                    this._input.selectionStart
                );
            }
            return true;
        }

        if (e.key === 'Escape') {
            if (popupVisible) {
                this._hidePopup();
                return true;
            }
            return false;
        }

        if (e.key === 'ArrowDown') {
            if (popupVisible) {
                e.preventDefault();
                this._selectItem(this._selectedIndex + 1);
                return true;
            }
            // History: forward
            if (this._historyIndex >= 0) {
                e.preventDefault();
                this._historyIndex--;
                if (this._historyIndex < 0) {
                    this._input.value = this._historyStash;
                } else {
                    this._input.value =
                        this._history[this._historyIndex];
                }
                return true;
            }
            return false;
        }

        if (e.key === 'ArrowUp') {
            if (popupVisible) {
                e.preventDefault();
                this._selectItem(this._selectedIndex - 1);
                return true;
            }
            // History: backward
            if (this._historyIndex < this._history.length - 1) {
                e.preventDefault();
                if (this._historyIndex < 0) {
                    this._historyStash = this._input.value;
                }
                this._historyIndex++;
                this._input.value = this._history[this._historyIndex];
                return true;
            }
            return false;
        }

        if (e.key === 'Enter') {
            if (popupVisible) {
                e.preventDefault();
                this._acceptCompletion();
                return true;
            }
            this._historyIndex = -1;
            return false;
        }

        return false;
    }

    addToHistory(cmd) {
        if (cmd && (this._history.length === 0 || this._history[0] !== cmd)) {
            this._history.unshift(cmd);
        }
        this._historyIndex = -1;
    }

    _onInput() {
        clearTimeout(this._debounceTimer);
        this._debounceTimer = setTimeout(() => {
            const line = this._input.value;
            const cursor = this._input.selectionStart;
            const prefix = this._extractPrefix(line, cursor);

            const shouldTrigger =
                prefix.length >= kMinPrefixLength ||
                (prefix.length >= 1 && (prefix[0] === '$' || prefix[0] === '-'));

            if (shouldTrigger) {
                this._requestCompletions(line, cursor);
            } else {
                this._hidePopup();
            }
        }, kDebounceMs);
    }

    _onBlur() {
        // Small delay so click on popup item fires first
        setTimeout(() => this._hidePopup(), 150);
    }

    _extractPrefix(line, cursorPos) {
        let pos = cursorPos - 1;
        while (pos >= 0 && kBoundaryChars.indexOf(line[pos]) === -1) {
            pos--;
        }
        return line.substring(pos + 1, cursorPos);
    }

    _requestCompletions(line, cursorPos) {
        const prefix = this._extractPrefix(line, cursorPos);
        const isVariable = prefix.length > 0 && prefix[0] === '$';
        const isArgument = prefix.length > 0 && prefix[0] === '-';
        const isCommand = !isVariable && !isArgument;

        // Use cached command list for client-side filtering
        if (isCommand && this._commandCache) {
            const lowerPrefix = prefix.toLowerCase();
            const filtered = this._commandCache.filter(
                c => c.toLowerCase().startsWith(lowerPrefix)
            );
            const wordStart = cursorPos - prefix.length;
            this._showPopup(filtered, wordStart, cursorPos);
            return;
        }

        this._ws.request({
            type: 'tcl_complete',
            line: line,
            cursor_pos: cursorPos,
        }).then(data => {
            // Cache command completions (commands don't change at runtime)
            if (data.mode === 'commands' && !this._commandCache && prefix === '') {
                this._commandCache = data.completions;
            }
            if (data.mode === 'commands' && !this._commandCache) {
                // Request full list for caching on first non-empty prefix
                this._ws.request({
                    type: 'tcl_complete',
                    line: '',
                    cursor_pos: 0,
                }).then(fullData => {
                    if (fullData.mode === 'commands') {
                        this._commandCache = fullData.completions;
                    }
                }).catch(() => {});
            }
            this._showPopup(
                data.completions,
                data.replace_start,
                data.replace_end
            );
        }).catch(() => {
            this._hidePopup();
        });
    }

    _showPopup(completions, replaceStart, replaceEnd) {
        if (!completions || completions.length === 0) {
            this._hidePopup();
            return;
        }

        // If only one match and it equals the prefix exactly, hide
        const prefix = this._input.value.substring(replaceStart, replaceEnd);
        if (completions.length === 1 && completions[0] === prefix) {
            this._hidePopup();
            return;
        }

        this._items = completions;
        this._replaceStart = replaceStart;
        this._replaceEnd = replaceEnd;

        this._popup.innerHTML = '';
        const shown = completions.slice(0, kMaxVisible);
        shown.forEach((text, i) => {
            const item = document.createElement('div');
            item.className = 'tcl-complete-item';
            item.textContent = text;
            item.addEventListener('mousedown', (e) => {
                e.preventDefault(); // prevent blur
                this._selectedIndex = i;
                this._acceptCompletion();
            });
            this._popup.appendChild(item);
        });
        if (completions.length > kMaxVisible) {
            const more = document.createElement('div');
            more.className = 'tcl-complete-more';
            more.textContent = `... ${completions.length - kMaxVisible} more`;
            this._popup.appendChild(more);
        }

        this._selectedIndex = 0;
        this._highlightSelected();
        this._popup.style.display = 'block';
    }

    _hidePopup() {
        this._popup.style.display = 'none';
        this._items = [];
        this._selectedIndex = -1;
    }

    _selectItem(index) {
        if (this._items.length === 0) return;
        const maxIndex = Math.min(this._items.length, kMaxVisible) - 1;
        this._selectedIndex = Math.max(0, Math.min(index, maxIndex));
        this._highlightSelected();
    }

    _highlightSelected() {
        const children = this._popup.querySelectorAll('.tcl-complete-item');
        children.forEach((el, i) => {
            el.classList.toggle('selected', i === this._selectedIndex);
        });
        if (children[this._selectedIndex] && children[this._selectedIndex].scrollIntoView) {
            children[this._selectedIndex].scrollIntoView({ block: 'nearest' });
        }
    }

    _acceptCompletion() {
        if (this._selectedIndex < 0 || this._selectedIndex >= this._items.length) {
            this._hidePopup();
            return;
        }

        const completion = this._items[this._selectedIndex];
        const before = this._input.value.substring(0, this._replaceStart);
        const after = this._input.value.substring(this._replaceEnd);
        this._input.value = before + completion + after;

        const newCursor = this._replaceStart + completion.length;
        this._input.setSelectionRange(newCursor, newCursor);
        this._input.focus();
        this._hidePopup();
    }
}
