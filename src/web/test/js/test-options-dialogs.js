// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { clampFontScale, kFontScaleDefault, kFontScaleMax, kFontScaleMin,
         showAppFontDialog, showArrowStepDialog }
    from '../../src/options-dialogs.js';
import { clampArrowStep, kArrowStepDefault, kArrowStepMax, kArrowStepMin }
    from '../../src/ui-utils.js';

function makeApp(overrides = {}) {
    return {
        arrowStep: kArrowStepDefault,
        fontFamily: '',
        fontScale: kFontScaleDefault,
        steps: [],
        fonts: [],
        setArrowStep(step) { this.steps.push(step); },
        setAppFont(font) { this.fonts.push(font); },
        ...overrides,
    };
}

const errorText = () =>
    document.querySelector('.modal-error').textContent;
const isDialogOpen = () => !!document.querySelector('.modal-overlay');

describe('clampArrowStep', () => {
    it('keeps a value inside Qt\'s 10..1000 range', () => {
        assert.equal(clampArrowStep(20), 20);
        assert.equal(clampArrowStep(kArrowStepMin), kArrowStepMin);
        assert.equal(clampArrowStep(kArrowStepMax), kArrowStepMax);
    });

    it('clamps out-of-range values instead of rejecting them', () => {
        assert.equal(clampArrowStep(0), kArrowStepMin);
        assert.equal(clampArrowStep(-5), kArrowStepMin);
        assert.equal(clampArrowStep(99999), kArrowStepMax);
    });

    // A NaN would reach Leaflet's keyboardPanDelta and leave the arrow keys
    // doing nothing at all, which is worse than ignoring a bad cookie.
    it('falls back to the default for unparseable input', () => {
        assert.equal(clampArrowStep(undefined), kArrowStepDefault);
        assert.equal(clampArrowStep(null), kArrowStepDefault);
        assert.equal(clampArrowStep('abc'), kArrowStepDefault);
        assert.equal(clampArrowStep(''), kArrowStepDefault);
    });

    it('reads a cookie string and rounds a fractional value', () => {
        assert.equal(clampArrowStep('120'), 120);
        assert.equal(clampArrowStep(42.4), 42);
    });
});

describe('clampFontScale', () => {
    it('keeps a percentage inside range and clamps outside it', () => {
        assert.equal(clampFontScale(125), 125);
        assert.equal(clampFontScale(10), kFontScaleMin);
        assert.equal(clampFontScale(1000), kFontScaleMax);
    });

    it('falls back to 100% for unparseable input', () => {
        assert.equal(clampFontScale(''), kFontScaleDefault);
        assert.equal(clampFontScale('nope'), kFontScaleDefault);
        assert.equal(clampFontScale(undefined), kFontScaleDefault);
    });
});

describe('showArrowStepDialog', () => {
    beforeEach(() => { document.body.innerHTML = ''; });

    it('prefills the current step and applies a new one', () => {
        const app = makeApp({ arrowStep: 40 });
        showArrowStepDialog(app);
        const input = document.querySelector('.opt-step');
        assert.equal(input.value, '40');

        input.value = '250';
        document.querySelector('.ok').click();
        assert.deepEqual(app.steps, [250]);
        assert.equal(isDialogOpen(), false, 'dialog closed on apply');
    });

    it('rejects a non-numeric step without touching the setting', () => {
        const app = makeApp();
        showArrowStepDialog(app);
        document.querySelector('.opt-step').value = 'x';
        document.querySelector('.ok').click();
        assert.match(errorText(), /must be a number/);
        assert.deepEqual(app.steps, []);
        assert.ok(isDialogOpen(), 'dialog stays open so the value can be fixed');
    });

    it('rejects a step outside the range rather than clamping silently', () => {
        const app = makeApp();
        showArrowStepDialog(app);
        document.querySelector('.opt-step').value = '5';
        document.querySelector('.ok').click();
        assert.match(errorText(), /between 10 and 1000/);
        assert.deepEqual(app.steps, []);
    });

    it('applies on Enter in the field', () => {
        const app = makeApp();
        showArrowStepDialog(app);
        const input = document.querySelector('.opt-step');
        input.value = '30';
        input.dispatchEvent(new window.KeyboardEvent(
            'keydown', { key: 'Enter', bubbles: true }));
        assert.deepEqual(app.steps, [30]);
    });
});

describe('showAppFontDialog', () => {
    beforeEach(() => { document.body.innerHTML = ''; });

    it('applies the selected family and scale', () => {
        const app = makeApp();
        showAppFontDialog(app);
        const family = document.querySelector('.opt-family');
        family.value = 'monospace';
        document.querySelector('.opt-scale').value = '130';
        document.querySelector('.ok').click();
        assert.deepEqual(app.fonts, [{ family: 'monospace', scale: 130 }]);
    });

    // A stack the user typed by hand must win over the dropdown, otherwise
    // reopening the dialog would silently revert it.
    it('prefers a hand-typed family over the dropdown', () => {
        const app = makeApp();
        showAppFontDialog(app);
        document.querySelector('.opt-custom').value = '"My Font", serif';
        document.querySelector('.ok').click();
        assert.deepEqual(app.fonts,
                         [{ family: '"My Font", serif', scale: 100 }]);
    });

    it('shows a stored custom family as the selected option', () => {
        const app = makeApp({ fontFamily: '"My Font", serif' });
        showAppFontDialog(app);
        assert.equal(document.querySelector('.opt-family').value,
                     '"My Font", serif');
        assert.equal(document.querySelector('.opt-custom').value,
                     '"My Font", serif');
    });

    it('rejects a scale outside the range', () => {
        const app = makeApp();
        showAppFontDialog(app);
        document.querySelector('.opt-scale').value = '400';
        document.querySelector('.ok').click();
        assert.match(errorText(), /between 70 and 200/);
        assert.deepEqual(app.fonts, []);
    });

    it('previews the pending choice without applying it', () => {
        const app = makeApp();
        showAppFontDialog(app);
        const family = document.querySelector('.opt-family');
        family.value = 'serif';
        family.dispatchEvent(new window.Event('input', { bubbles: true }));
        assert.equal(document.querySelector('.opt-preview').style.fontFamily,
                     'serif');
        assert.deepEqual(app.fonts, [], 'nothing applied yet');
    });
});
