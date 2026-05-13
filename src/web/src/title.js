// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Mirrors the GUI's window title (mainWindow.cpp::updateTitle):
//   "OpenROAD - <block name>"   when a design is loaded
//   "OpenROAD"                  when no design is loaded
export function updateDocumentTitle(blockName, doc = (typeof document !== 'undefined' ? document : null)) {
    const base = 'OpenROAD';
    const title = blockName ? `${base} - ${blockName}` : base;
    if (doc) doc.title = title;
    return title;
}
