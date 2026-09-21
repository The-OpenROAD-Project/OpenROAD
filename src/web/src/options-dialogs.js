// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Dialogs behind the Options menu (issue #10619 table 2.15):
//  - Arrow keys scroll step: mirrors Qt MainWindow::showArrowKeysScrollStep.
//  - Application font: the browser counterpart of Qt's QFontDialog +
//    QApplication::setFont.
// Both write through the app setters, which own persistence.

import { buildModal, clampArrowStep, kArrowStepDefault, kArrowStepMax,
         kArrowStepMin } from './ui-utils.js';

const kDialogClass = 'options-dialog';

// Font stacks offered by the Application font dialog.  A browser cannot
// enumerate installed fonts without the Local Font Access permission prompt,
// so this is a curated list of stacks that resolve on any platform, plus a
// free-text box for anything else the user has.  "System default" is the empty
// string, which app.setAppFont treats as "remove the override" so the
// stylesheet's own --or-font-family applies — copying the stack here would
// pin whatever it said the day the dialog was written.
const kFontFamilies = [
    { label: 'System default', value: '' },
    { label: 'Sans-serif', value: 'sans-serif' },
    { label: 'Serif', value: 'serif' },
    { label: 'Monospace', value: 'monospace' },
    { label: 'Helvetica / Arial', value: "Helvetica, Arial, sans-serif" },
    { label: 'Georgia', value: "Georgia, 'Times New Roman', serif" },
    { label: 'Courier', value: "'Courier New', Courier, monospace" },
];

export const kFontScaleDefault = 100;
export const kFontScaleMin = 70;
export const kFontScaleMax = 200;

// Percent -> the unitless multiplier --or-font-scale wants.  Out-of-range and
// unparseable values clamp rather than propagate: a NaN would collapse every
// calc() in the stylesheet to nothing and leave the UI without font sizes.
export function clampFontScale(percent) {
    // Number('') and Number(null) are both 0, which would clamp to the
    // minimum; an unset cookie has to read as absent, not as "70 %".
    const value = (percent === '' || percent === null)
        ? NaN : Math.round(Number(percent));
    if (!Number.isFinite(value)) {
        return kFontScaleDefault;
    }
    return Math.min(kFontScaleMax, Math.max(kFontScaleMin, value));
}

export function showArrowStepDialog(app) {
    const { overlay, close, okBtn, showError } = buildModal(
        'Arrow keys scroll step', `
        <div class="sn-row">
            <label>Step (px)</label>
            <input type="number" class="fb-path-input opt-step"
                   min="${kArrowStepMin}" max="${kArrowStepMax}" step="1"
                   value="${clampArrowStep(app.arrowStep)}">
        </div>
        <div class="opt-hint">How far one arrow-key press pans the layout.
            Default ${kArrowStepDefault}.</div>`,
        'Apply', kDialogClass);

    const stepIn = overlay.querySelector('.opt-step');
    stepIn.focus();
    stepIn.select();

    function apply() {
        const raw = stepIn.value.trim();
        const step = Number(raw);
        if (raw === '' || !Number.isFinite(step)) {
            showError('Step must be a number.');
            return;
        }
        if (step < kArrowStepMin || step > kArrowStepMax) {
            showError(`Step must be between ${kArrowStepMin} and `
                      + `${kArrowStepMax} px.`);
            return;
        }
        app.setArrowStep(step);
        close();
    }

    okBtn.addEventListener('click', apply);
    stepIn.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') { e.preventDefault(); apply(); }
    });
    return { close };
}

export function showAppFontDialog(app) {
    // A stack the user typed by hand is not in the list, so it gets its own
    // option rather than silently snapping the dropdown to "System default".
    const current = app.fontFamily || '';
    const known = kFontFamilies.some((f) => f.value === current);
    const options = kFontFamilies
        .map((f) => `<option value="${escapeAttr(f.value)}"`
                    + `${f.value === current ? ' selected' : ''}>`
                    + `${f.label}</option>`)
        .join('')
        + (known ? ''
                 : `<option value="${escapeAttr(current)}" selected>Custom`
                   + `</option>`);

    const { overlay, close, okBtn, showError } = buildModal(
        'Application font', `
        <div class="sn-row">
            <label>Family</label>
            <select class="opt-family">${options}</select>
        </div>
        <div class="sn-row">
            <label>Custom family</label>
            <input type="text" class="fb-path-input opt-custom"
                   placeholder="optional CSS font-family"
                   value="${known ? '' : escapeAttr(current)}">
        </div>
        <div class="sn-row">
            <label>Size (%)</label>
            <input type="number" class="fb-path-input opt-scale"
                   min="${kFontScaleMin}" max="${kFontScaleMax}" step="5"
                   value="${clampFontScale(app.fontScale)}">
        </div>
        <div class="opt-preview">The quick brown fox jumps over the lazy dog
            0123456789</div>
        <div class="opt-hint">Applies to the panels, menus and console. Text
            drawn into the layout is rendered on the server and is not
            affected.</div>`,
        'Apply', kDialogClass);

    const familySel = overlay.querySelector('.opt-family');
    const customIn = overlay.querySelector('.opt-custom');
    const scaleIn = overlay.querySelector('.opt-scale');
    const preview = overlay.querySelector('.opt-preview');

    // Preview the pending choice without committing it, so a bad size can be
    // seen before it is applied to the whole window.
    function refreshPreview() {
        preview.style.fontFamily = customIn.value.trim() || familySel.value;
        preview.style.fontSize =
            `calc(13px * ${clampFontScale(scaleIn.value) / 100})`;
    }
    for (const el of [familySel, customIn, scaleIn]) {
        el.addEventListener('input', refreshPreview);
    }
    refreshPreview();

    function apply() {
        const raw = scaleIn.value.trim();
        const scale = Number(raw);
        if (raw === '' || !Number.isFinite(scale)) {
            showError('Size must be a number.');
            return;
        }
        if (scale < kFontScaleMin || scale > kFontScaleMax) {
            showError(`Size must be between ${kFontScaleMin} and `
                      + `${kFontScaleMax} %.`);
            return;
        }
        app.setAppFont({
            family: customIn.value.trim() || familySel.value,
            scale,
        });
        close();
    }

    okBtn.addEventListener('click', apply);
    for (const el of [customIn, scaleIn]) {
        el.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') { e.preventDefault(); apply(); }
        });
    }
    return { close };
}

// Font stacks carry quotes, which would close the value attribute early.
function escapeAttr(value) {
    return String(value).replace(/&/g, '&amp;').replace(/"/g, '&quot;')
        .replace(/</g, '&lt;').replace(/>/g, '&gt;');
}
