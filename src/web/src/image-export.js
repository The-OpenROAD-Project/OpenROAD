// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Client-side image/data export helpers for per-widget "Save" actions.
//
// The web GUI renders widgets in the browser (canvas / SVG / WebGL), so —
// unlike the Qt GUI, which re-renders widgets server-side to a file — export
// happens client-side and downloads directly in the browser.  These helpers
// are shared by schematic (SVG/PNG), 3D (PNG), and charts (CSV).

// Trigger a browser download of a data: or blob: URL.
export function downloadUrl(url, filename) {
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    a.remove();
}

// Download a Blob as `filename` (revokes the object URL afterwards).
export function downloadBlob(blob, filename) {
    const url = URL.createObjectURL(blob);
    downloadUrl(url, filename);
    // Defer revoke so the download has a chance to start.
    setTimeout(() => URL.revokeObjectURL(url), 1000);
}

// Download rows (array of arrays) as a CSV file.  Values containing commas,
// quotes, or newlines are quoted per RFC 4180.
export function downloadCsv(rows, filename) {
    const escape = (v) => {
        const s = v === null || v === undefined ? '' : String(v);
        return /[",\n]/.test(s) ? '"' + s.replace(/"/g, '""') + '"' : s;
    };
    const csv = rows.map(r => r.map(escape).join(',')).join('\r\n');
    downloadBlob(new Blob([csv], { type: 'text/csv;charset=utf-8' }), filename);
}

// Serialize an <svg> element to a standalone SVG string (ensures the xmlns and
// explicit width/height so the file opens outside the browser).
export function svgToString(svgEl) {
    const clone = svgEl.cloneNode(true);
    if (!clone.getAttribute('xmlns')) {
        clone.setAttribute('xmlns', 'http://www.w3.org/2000/svg');
    }
    if (!clone.getAttribute('xmlns:xlink')) {
        clone.setAttribute('xmlns:xlink', 'http://www.w3.org/1999/xlink');
    }
    // Prefer viewBox dimensions; fall back to the rendered size.
    if (!clone.getAttribute('width') || !clone.getAttribute('height')) {
        const box = svgEl.viewBox && svgEl.viewBox.baseVal;
        if (box && box.width && box.height) {
            clone.setAttribute('width', box.width);
            clone.setAttribute('height', box.height);
        }
    }
    return '<?xml version="1.0" encoding="UTF-8"?>\n'
        + new XMLSerializer().serializeToString(clone);
}

// Best-effort dimensions of an <svg> (viewBox, then attributes, then bbox).
function svgSize(svgEl) {
    const box = svgEl.viewBox && svgEl.viewBox.baseVal;
    if (box && box.width && box.height) {
        return { width: box.width, height: box.height };
    }
    const w = parseFloat(svgEl.getAttribute('width'));
    const h = parseFloat(svgEl.getAttribute('height'));
    if (w && h) return { width: w, height: h };
    const r = svgEl.getBoundingClientRect();
    return { width: r.width || 800, height: r.height || 600 };
}

// Rasterize an <svg> element to a PNG data URL at `scale`x its intrinsic size
// (scale is the web-only high-resolution export factor).  Returns a Promise.
export function rasterizeSvg(svgEl, scale = 2) {
    const { width, height } = svgSize(svgEl);
    const svgString = svgToString(svgEl);
    const blob = new Blob([svgString], { type: 'image/svg+xml;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    return new Promise((resolve, reject) => {
        const img = new Image();
        img.onload = () => {
            const canvas = document.createElement('canvas');
            canvas.width = Math.max(1, Math.round(width * scale));
            canvas.height = Math.max(1, Math.round(height * scale));
            const ctx = canvas.getContext('2d');
            ctx.fillStyle = '#ffffff';
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
            URL.revokeObjectURL(url);
            resolve(canvas.toDataURL('image/png'));
        };
        img.onerror = (e) => { URL.revokeObjectURL(url); reject(e); };
        img.src = url;
    });
}

// Copy a PNG data URL to the clipboard as an image (best-effort; resolves to
// true on success, false when unsupported/denied).
export async function copyPngToClipboard(dataUrl) {
    try {
        if (!navigator.clipboard || !window.ClipboardItem) return false;
        const blob = await (await fetch(dataUrl)).blob();
        await navigator.clipboard.write([
            new ClipboardItem({ 'image/png': blob }),
        ]);
        return true;
    } catch (e) {
        return false;
    }
}
