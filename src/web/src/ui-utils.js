// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared UI utilities.

// Make table column headers resizable by dragging.
export function makeResizableHeaders(table) {
    // Reset to auto layout so browser computes natural column widths
    table.style.tableLayout = 'auto';
    const headers = table.querySelectorAll('thead th');
    headers.forEach((th) => th.style.width = '');
    // Force reflow to get natural widths
    const widths = Array.from(headers, (th) => th.offsetWidth);
    // Now lock in widths and switch to fixed layout
    headers.forEach((th, i) => th.style.width = widths[i] + 'px');
    table.style.tableLayout = 'fixed';

    headers.forEach((th, idx) => {
        if (idx === headers.length - 1) return; // skip last column
        const grip = document.createElement('div');
        grip.className = 'col-resize-grip';
        th.style.position = 'relative';
        th.appendChild(grip);

        let startX, startW;
        const onMouseMove = (e) => {
            th.style.width = Math.max(30, startW + e.clientX - startX) + 'px';
        };
        const onMouseUp = () => {
            document.removeEventListener('mousemove', onMouseMove);
            document.removeEventListener('mouseup', onMouseUp);
        };
        grip.addEventListener('mousedown', (e) => {
            e.preventDefault();
            startX = e.clientX;
            startW = th.offsetWidth;
            document.addEventListener('mousemove', onMouseMove);
            document.addEventListener('mouseup', onMouseUp);
        });
    });
}
