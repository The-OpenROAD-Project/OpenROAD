// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Paint an unmissable red banner at the top of the document for an
// unhandled error. Intended for bugs — anything that reaches here is
// something a developer should fix. Static-mode feature gaps must be
// surfaced as calm inline messages, not banners; see the widget stubs.

export function installGlobalErrorHandlers(doc = document, win = window) {
    win.addEventListener('unhandledrejection', (ev) => {
        showErrorBanner(ev.reason, doc);
    });
    win.addEventListener('error', (ev) => {
        showErrorBanner(ev.error || ev.message, doc);
    });
}

export function showErrorBanner(err, doc = document) {
    const container = getOrCreateContainer(doc);
    const banner = doc.createElement('div');
    banner.className = 'error-banner';

    const summary = errorSummary(err);
    const summaryEl = doc.createElement('div');
    summaryEl.className = 'error-banner-summary';
    summaryEl.textContent = summary;
    banner.appendChild(summaryEl);

    const stack = errorStack(err);
    if (stack) {
        const details = doc.createElement('details');
        const summaryBtn = doc.createElement('summary');
        summaryBtn.textContent = 'Details';
        details.appendChild(summaryBtn);
        const pre = doc.createElement('pre');
        pre.className = 'error-banner-stack';
        pre.textContent = stack;
        details.appendChild(pre);
        banner.appendChild(details);
    }

    const close = doc.createElement('button');
    close.className = 'error-banner-close';
    close.setAttribute('aria-label', 'Dismiss');
    close.textContent = '×';
    close.addEventListener('click', () => banner.remove());
    banner.appendChild(close);

    container.appendChild(banner);
    return banner;
}

function getOrCreateContainer(doc) {
    let c = doc.getElementById('error-banner-container');
    if (!c) {
        c = doc.createElement('div');
        c.id = 'error-banner-container';
        doc.body.appendChild(c);
    }
    return c;
}

function errorSummary(err) {
    if (err instanceof Error) return err.message || String(err);
    if (err && typeof err === 'object' && err.message) return String(err.message);
    return String(err);
}

function errorStack(err) {
    if (err instanceof Error && err.stack) return err.stack;
    return null;
}
