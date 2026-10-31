// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Which tile levels the layout view draws while the zoom changes (#11340).
//
// Shared by every tile slot so they all change level together.  With double
// buffering the new level stays hidden, under the old one scaled, until every
// slot has loaded what is on screen; it is then shown in one frame, fading in
// over the old level when smoothing is on.  Without it the new level paints
// tile by tile over the old one, which stays underneath until the new level
// has settled.

import { prefersReducedMotion } from './ui-utils.js';
import { easeOutQuad } from './view-math.js';

const ATOMIC_ZOOM_TIMEOUT_MS = 10000;
export const ZOOM_FADE_MS = 150;
const kMaxUnder = 3;                     // levels kept under a loading one

export class ZoomSwap {
    constructor({
        isEnabled = () => true,
        isSmooth = () => false,
        reducedMotion = prefersReducedMotion,
        now = () => performance.now(),
        setTimer = (fn, ms) => setTimeout(fn, ms),
        clearTimer = (id) => clearTimeout(id),
        timeoutMs = ATOMIC_ZOOM_TIMEOUT_MS,
        fadeMs = ZOOM_FADE_MS,
        onChange = () => {},
    } = {}) {
        Object.assign(this, { isEnabled, isSmooth, reducedMotion, now, setTimer,
                              clearTimer, timeoutMs, fadeMs, onChange });
        this.shownZ = null;    // level drawn as complete
        this.pendingZ = null;  // level loading behind it (double buffering)
        this.under = [];       // old levels kept under a progressive one
        this.fade = null;      // { fromZ, toZ, start, ms }
        this._timer = null;
    }

    // The level the slots should be loading.
    wantedZ() {
        return this.pendingZ !== null ? this.pendingZ : this.shownZ;
    }

    // The view moved to tile level z.  `hasContent` says whether the level on
    // screen still has tiles to keep showing.
    setZoom(z, { hasContent = true } = {}) {
        if (z === this.wantedZ()) {
            return;
        }
        this.endFade();
        this._clearTimer();
        if (this.shownZ === null) {
            this.shownZ = z;
            this.pendingZ = null;
            this.under = [];
        } else if (!hasContent) {
            // Nothing of the shown level to keep; what lies under it stays.
            this.shownZ = z;
            this.pendingZ = null;
            this.under = this.under.filter((u) => u !== z);
        } else if (z === this.shownZ) {
            this.pendingZ = null;       // zoomed back before the swap
        } else if (this.isEnabled()) {
            this.pendingZ = z;
            this.under = this.under.filter((u) => u !== z);
            this._timer = this.setTimer(() => this.reveal({ settled: false }),
                                        this.timeoutMs);
        } else {
            // A level still loading covers only part of the view, so the
            // ones under it stay too, as many as kMaxUnder.
            this.under = [...this.under.filter((u) => u !== z), this.shownZ]
                .slice(-kMaxUnder);
            this.shownZ = z;
            this.pendingZ = null;
        }
        this.onChange();
    }

    // Called whenever loading progresses.  Returns true when it changed what
    // is drawn.  `animating` holds the reveal until the zoom animation ends.
    check({ settled, animating = false }) {
        if (this.pendingZ !== null) {
            if (animating || (!settled && this.isEnabled())) {
                return false;
            }
            this.reveal({ settled });
            return true;
        }
        if (this.under.length > 0 && settled && !animating) {
            this.under = [];
            this.onChange();
            return true;
        }
        return false;
    }

    // Shown before it is complete, the new level keeps the old ones under it
    // until check() sees it settled.
    reveal({ settled = true } = {}) {
        if (this.pendingZ === null) {
            return;
        }
        this._clearTimer();
        const from = this.shownZ;
        this.shownZ = this.pendingZ;
        this.pendingZ = null;
        if (settled) {
            this.under = [];
        } else if (from !== null) {
            this.under = [...this.under.filter(
                (u) => u !== from && u !== this.shownZ), from].slice(-kMaxUnder);
        }
        const ms = this.isSmooth() && !this.reducedMotion() ? this.fadeMs : 0;
        this.fade = (ms > 0 && from !== null)
            ? { fromZ: from, toZ: this.shownZ, start: this.now(), ms }
            : null;
        this.onChange();
    }

    endFade() {
        if (this.fade) {
            this.fade = null;
            this.onChange();
        }
    }

    // What to draw now, bottom first.
    plan(now = this.now()) {
        if (this.fade) {
            const t = (now - this.fade.start) / this.fade.ms;
            if (t < 1) {
                return {
                    draw: [{ z: this.fade.fromZ, opacity: 1 },
                           { z: this.fade.toZ, opacity: easeOutQuad(Math.max(0, t)) }],
                    fading: true,
                };
            }
            this.fade = null;
        }
        if (this.shownZ === null) {
            return { draw: [], fading: false };
        }
        if (this.under.length > 0) {
            return { draw: [...this.under, this.shownZ].map(
                         (z) => ({ z, opacity: 1 })),
                     fading: false };
        }
        return { draw: [{ z: this.shownZ, opacity: 1 }], fading: false };
    }

    // Nothing is waiting to be shown or fading in.
    isIdle(now = this.now()) {
        return this.pendingZ === null && this.under.length === 0
            && !this.plan(now).fading;
    }

    _clearTimer() {
        if (this._timer !== null) {
            this.clearTimer(this._timer);
            this._timer = null;
        }
    }
}
