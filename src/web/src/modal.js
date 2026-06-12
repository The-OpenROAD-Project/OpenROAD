// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared modal-dialog lifecycle: overlay creation, Escape-to-close (document
// level, removed on close), and backdrop-click dismissal.  Callers supply the
// inner HTML (expected to contain a `.modal-dialog` element) and wire their own
// OK/Cancel buttons via the returned `dialog`/`close`.

export function createModalDialog(innerHtml) {
    const overlay = document.createElement('div');
    overlay.className = 'modal-overlay';
    overlay.innerHTML = innerHtml;

    function onKeydown(e) {
        if (e.key === 'Escape') {
            close();
        }
    }
    function close() {
        document.removeEventListener('keydown', onKeydown);
        overlay.remove();
    }

    document.body.appendChild(overlay);
    overlay.addEventListener('click', (e) => {
        if (e.target === overlay) {
            close();
        }
    });
    document.addEventListener('keydown', onKeydown);

    return { overlay, dialog: overlay.querySelector('.modal-dialog'), close };
}
