// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Navigation dialogs (issue #10619 table 2.8):
//  - Go to position: jump the view to x,y in microns (mirrors Qt GotoDialog).
// Reuses the .modal-overlay/.modal-dialog pattern from menu-bar.js; Find
// lives in menu-bar.js, which searches every descriptor type the server
// registers.

import { dbuToLatLng, dbuRectToBounds, latLngToDbu } from './coordinates.js';

// Build a modal shell and return handles.  Closes on backdrop click / Escape.
function buildModal(title, bodyHtml, okLabel) {
    const overlay = document.createElement('div');
    overlay.className = 'modal-overlay';
    overlay.innerHTML = `
        <div class="modal-dialog search-nav-dialog">
            <button class="modal-close" title="Close" aria-label="Close">&times;</button>
            <h3>${title}</h3>
            ${bodyHtml}
            <div class="modal-error" style="display:none"></div>
            <div class="modal-buttons">
                <button class="cancel">Cancel</button>
                <button class="primary ok">${okLabel}</button>
            </div>
        </div>`;
    document.body.appendChild(overlay);

    const close = () => overlay.remove();
    overlay.addEventListener('click', (e) => { if (e.target === overlay) close(); });
    overlay.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
    overlay.querySelector('.cancel').addEventListener('click', close);
    overlay.querySelector('.modal-close').addEventListener('click', close);

    const errorDiv = overlay.querySelector('.modal-error');
    const showError = (msg, isInfo) => {
        errorDiv.textContent = msg;
        errorDiv.style.display = msg ? 'block' : 'none';
        errorDiv.classList.toggle('info', !!isInfo);
    };
    return { overlay, close, okBtn: overlay.querySelector('.ok'), showError };
}

// ── Go to position ──────────────────────────────────────────────────────────

export function showGotoDialog(app) {
    if (!app.designScale || !app.map) {
        return;
    }
    // Every field is read and written in whichever unit the "Show DBU"
    // setting (Qt's MainWindow::useDBU) is displaying, so the numbers here
    // match the ones the Inspector, the rulers and the scale bar are showing.
    const fmt = (dbu) => app.formatDbu(dbu);
    const parse = (str) => app.parseDbu(str);
    const unit = app.unitLabel();

    // Prefill X/Y with the current view center, so "Go to" with no edits is
    // a no-op.
    const c = app.map.getCenter();
    const centerDbu = latLngToDbu(c.lat, c.lng, app.designScale,
        app.designMaxDXDY, app.designOriginX, app.designOriginY);

    const { overlay, close, okBtn, showError } = buildModal('Go to Position', `
        <div class="sn-row"><label>X (${unit})</label>
            <input type="text" class="fb-path-input sn-x" value="${fmt(centerDbu.dbuX)}"></div>
        <div class="sn-row"><label>Y (${unit})</label>
            <input type="text" class="fb-path-input sn-y" value="${fmt(centerDbu.dbuY)}"></div>
        <div class="sn-row"><label>Size (${unit})</label>
            <input type="text" class="fb-path-input sn-size" placeholder="optional zoom window"></div>`,
        'Go to');

    const xIn = overlay.querySelector('.sn-x');
    const yIn = overlay.querySelector('.sn-y');
    const sizeIn = overlay.querySelector('.sn-size');
    xIn.focus();
    xIn.select();

    function doGoto() {
        const dbuX = parse(xIn.value);
        const dbuY = parse(yIn.value);
        if (dbuX === null || dbuY === null) {
            showError(`X and Y must be numbers (${unit}).`);
            return;
        }
        const latlng = dbuToLatLng(dbuX, dbuY, app.designScale,
            app.designMaxDXDY, app.designOriginX, app.designOriginY);

        // A size of 0 or less is not a window; fall through to the recentre.
        const sizeVal = sizeIn.value.trim();
        const sizeDbu = sizeVal ? parse(sizeVal) : null;
        if (sizeDbu !== null && sizeDbu > 0) {
            const half = sizeDbu / 2;
            app.map.fitBounds(dbuRectToBounds(
                dbuX - half, dbuY - half, dbuX + half, dbuY + half,
                app.designScale, app.designMaxDXDY,
                app.designOriginX, app.designOriginY));
        } else {
            app.map.setView(latlng, app.map.getZoom());
        }
        close();
    }

    okBtn.addEventListener('click', doGoto);
    for (const el of [xIn, yIn, sizeIn]) {
        el.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') { e.preventDefault(); doGoto(); }
        });
    }
    return { close };
}
