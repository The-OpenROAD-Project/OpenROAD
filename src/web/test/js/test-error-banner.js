// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { showErrorBanner, installGlobalErrorHandlers } from '../../src/error-banner.js';

describe('showErrorBanner', () => {
    beforeEach(() => {
        document.body.innerHTML = '';
    });

    it('renders a banner with the error message', () => {
        showErrorBanner(new Error('boom'));
        const banner = document.querySelector('.error-banner');
        assert.ok(banner);
        assert.equal(banner.querySelector('.error-banner-summary').textContent, 'boom');
    });

    it('includes the stack in collapsible details', () => {
        const err = new Error('kaboom');
        showErrorBanner(err);
        const stack = document.querySelector('.error-banner-stack');
        assert.ok(stack);
        assert.ok(stack.textContent.includes('kaboom'));
    });

    it('omits details for non-Error values without a stack', () => {
        showErrorBanner('plain string reason');
        assert.equal(document.querySelector('.error-banner-stack'), null);
        assert.equal(
            document.querySelector('.error-banner-summary').textContent,
            'plain string reason',
        );
    });

    it('stacks multiple banners in one container', () => {
        showErrorBanner(new Error('first'));
        showErrorBanner(new Error('second'));
        const container = document.getElementById('error-banner-container');
        assert.equal(container.querySelectorAll('.error-banner').length, 2);
    });

    it('dismisses when the close button is clicked', () => {
        showErrorBanner(new Error('nope'));
        const banner = document.querySelector('.error-banner');
        banner.querySelector('.error-banner-close').click();
        assert.equal(document.querySelector('.error-banner'), null);
    });
});

describe('installGlobalErrorHandlers', () => {
    beforeEach(() => {
        document.body.innerHTML = '';
    });

    it('paints a banner on an error event', () => {
        installGlobalErrorHandlers(document, window);
        window.dispatchEvent(new window.ErrorEvent('error', {
            error: new Error('global kaboom'),
            message: 'global kaboom',
        }));
        const banner = document.querySelector('.error-banner');
        assert.ok(banner);
        assert.equal(banner.querySelector('.error-banner-summary').textContent, 'global kaboom');
    });

    it('paints a banner on an unhandledrejection event', () => {
        installGlobalErrorHandlers(document, window);
        // jsdom does not ship PromiseRejectionEvent; synthesize a compatible event.
        const ev = new window.Event('unhandledrejection');
        ev.reason = new Error('async kaboom');
        window.dispatchEvent(ev);
        const banner = document.querySelector('.error-banner');
        assert.ok(banner);
        assert.equal(banner.querySelector('.error-banner-summary').textContent, 'async kaboom');
    });
});
