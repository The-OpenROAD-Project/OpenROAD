// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Editing-utility dialogs (Global Connect, Insert Buffer) for the web GUI.
//
// Mirrors the Qt globalConnectDialog / insertBufferDialog.  The DB edits run
// on the server: Global Connect reuses the existing Tcl commands
// (add_global_connection / global_connect / clear_global_connect) plus a
// global_connect_info/global_connect_delete handler; Insert Buffer uses the
// buffer_info / insert_buffer handlers (odb API).  The dialogs reuse the same
// modal scaffold and CSS as menu-bar.js's file dialog.

// ── Shared modal scaffold ────────────────────────────────────────────────────
function makeModal(title, extraClass = '') {
    const overlay = document.createElement('div');
    overlay.className = 'modal-overlay';
    overlay.innerHTML = `
        <div class="modal-dialog ${extraClass}">
            <h3>${title}</h3>
            <div class="modal-body"></div>
            <div class="modal-error" style="display:none"></div>
            <div class="modal-buttons"></div>
        </div>`;
    document.body.appendChild(overlay);
    function onKey(e) {
        if (e.key === 'Escape') close();
    }
    const close = () => {
        overlay.remove();
        document.removeEventListener('keydown', onKey);
    };
    overlay.addEventListener('click', (e) => { if (e.target === overlay) close(); });
    document.addEventListener('keydown', onKey);
    return {
        overlay,
        body: overlay.querySelector('.modal-body'),
        errorDiv: overlay.querySelector('.modal-error'),
        buttons: overlay.querySelector('.modal-buttons'),
        close,
    };
}

function showError(errorDiv, msg) {
    errorDiv.textContent = msg;
    errorDiv.style.display = msg ? '' : 'none';
}

function addButton(parent, label, { primary = false } = {}) {
    const btn = document.createElement('button');
    btn.textContent = label;
    if (primary) btn.className = 'primary';
    parent.appendChild(btn);
    return btn;
}

// Tcl-quote a value as a brace-delimited word (safe for regex patterns).
function tclBrace(s) {
    return '{' + String(s) + '}';
}

// ── Global Connect ───────────────────────────────────────────────────────────
export function showGlobalConnectDialog(app) {
    const m = makeModal('Global Connect', 'gc-dialog');

    const table = document.createElement('div');
    table.className = 'gc-table';
    m.body.appendChild(table);

    // New-rule form.
    const form = document.createElement('div');
    form.className = 'gc-form';
    const instInput = document.createElement('input');
    instInput.placeholder = 'Instance pattern (regex)';
    instInput.value = '.*';
    const pinInput = document.createElement('input');
    pinInput.placeholder = 'Pin pattern (regex)';
    const netSelect = document.createElement('select');
    const regionSelect = document.createElement('select');
    const pgWrap = document.createElement('span');
    pgWrap.innerHTML =
        '<label><input type="radio" name="gc-pg" value="power" checked> Power</label>'
        + '<label><input type="radio" name="gc-pg" value="ground"> Ground</label>';
    form.append(labelWrap('Instance', instInput), labelWrap('Pin', pinInput),
                labelWrap('Net', netSelect), labelWrap('Region', regionSelect),
                pgWrap);
    const addBtn = addButton(form, 'Add rule');
    m.body.appendChild(form);

    // Live regex validation gates the Add button.
    function validate() {
        let ok = pinInput.value.trim() !== '' && netSelect.value !== '';
        for (const inp of [instInput, pinInput]) {
            try { new RegExp(inp.value); inp.classList.remove('gc-invalid'); }
            catch { inp.classList.add('gc-invalid'); ok = false; }
        }
        addBtn.disabled = !ok;
    }
    instInput.addEventListener('input', validate);
    pinInput.addEventListener('input', validate);
    netSelect.addEventListener('change', validate);

    function reload() {
        app.websocketManager.request({ type: 'global_connect_info' })
            .then(info => {
                // Rules table.
                table.innerHTML =
                    '<div class="gc-row gc-head"><span>Instance</span><span>Pin</span>'
                    + '<span>Net</span><span>Region</span><span></span></div>';
                (info.rules || []).forEach((r) => {
                    const row = document.createElement('div');
                    row.className = 'gc-row';
                    row.innerHTML = `<span>${esc(r.inst)}</span><span>${esc(r.pin)}</span>`
                        + `<span>${esc(r.net)}</span><span>${esc(r.region || 'All')}</span>`;
                    const del = document.createElement('button');
                    del.textContent = '✕';
                    del.title = 'Delete rule';
                    del.addEventListener('click', () => {
                        // Identify the rule by its fields (not a row index) so a
                        // concurrent change can't delete the wrong rule.
                        app.websocketManager.request({
                            type: 'global_connect_delete',
                            inst: r.inst, pin: r.pin,
                            net: r.net, region: r.region || '',
                        }).then(reload).catch(err => showError(m.errorDiv, String(err)));
                    });
                    row.appendChild(del);
                    table.appendChild(row);
                });
                // Net (special only) + region dropdowns.
                repopulate(netSelect, info.nets || []);
                repopulate(regionSelect, ['All', ...(info.regions || [])]);
                validate();
            })
            .catch(err => showError(m.errorDiv, String(err)));
    }

    addBtn.addEventListener('click', () => {
        const pg = form.querySelector('input[name="gc-pg"]:checked').value;
        let cmd = `add_global_connection -net ${tclBrace(netSelect.value)}`
            + ` -inst_pattern ${tclBrace(instInput.value || '.*')}`
            + ` -pin_pattern ${tclBrace(pinInput.value)} -${pg}`;
        if (regionSelect.value && regionSelect.value !== 'All') {
            cmd += ` -region ${tclBrace(regionSelect.value)}`;
        }
        runTcl(app, cmd, m.errorDiv, 'Rule added.', reload);
    });

    const applyBtn = addButton(m.buttons, 'Apply (force)');
    applyBtn.addEventListener('click', () => {
        app.websocketManager.request({ type: 'global_connect_apply', force: true })
            .then(resp => {
                if (!resp.had_rules) {
                    showError(m.errorDiv,
                        'No global-connect rules defined — add a rule first.');
                    return;
                }
                showError(m.errorDiv, `Connected ${resp.connected} pin(s).`);
                if (app.scheduleRedrawAllLayers) app.scheduleRedrawAllLayers();
                reload();
            })
            .catch(err => showError(m.errorDiv, err.message || String(err)));
    });
    const clearBtn = addButton(m.buttons, 'Clear all');
    clearBtn.addEventListener('click',
        () => runTcl(app, 'clear_global_connect', m.errorDiv, 'Rules cleared.', reload));
    addButton(m.buttons, 'Close').addEventListener('click', m.close);

    reload();
}

// Run a Tcl command; on success show `successMsg`, then redraw + reload.
function runTcl(app, cmd, errorDiv, successMsg, onDone) {
    app.websocketManager.request({ type: 'tcl_eval', cmd })
        .then(resp => {
            if (resp.is_error) {
                showError(errorDiv, resp.output || resp.result || 'Command failed');
                return;
            }
            showError(errorDiv, successMsg || '');
            if (app.scheduleRedrawAllLayers) app.scheduleRedrawAllLayers();
            if (onDone) onDone();
        })
        .catch(err => showError(errorDiv, err.message || String(err)));
}

// ── Insert Buffer ─────────────────────────────────────────────────────────────
export function showInsertBufferDialog(app, netName) {
    const m = makeModal(`Insert Buffer — ${esc(netName)}`, 'buf-dialog');
    const okBtn = addButton(m.buttons, 'Insert', { primary: true });
    okBtn.disabled = true;
    const cancelBtn = addButton(m.buttons, 'Cancel');
    cancelBtn.addEventListener('click', m.close);

    app.websocketManager.request({ type: 'buffer_info', net: netName })
        .then(info => {
            if (!info.can_buffer) {
                m.body.textContent = info.error
                    || 'This net has multiple drivers and cannot be buffered.';
                return;
            }
            buildBufferForm(app, m, netName, info, okBtn);
        })
        .catch(err => { m.body.textContent = String(err); });
}

function buildBufferForm(app, m, netName, info, okBtn) {
    // Pin list: driver XOR loads (mutually exclusive).
    const pinBox = document.createElement('div');
    pinBox.className = 'buf-pins';
    const drivers = info.drivers || [];
    const loads = info.loads || [];
    const selected = new Set();

    function renderPins() {
        pinBox.innerHTML = '';
        const add = (pin, isDriver) => {
            const row = document.createElement('label');
            row.className = 'buf-pin';
            const cb = document.createElement('input');
            cb.type = 'checkbox';
            cb.checked = selected.has(pin.id);
            cb.addEventListener('change', () => {
                if (cb.checked) {
                    if (isDriver) {
                        // Driver is exclusive: clear everything else.
                        selected.clear();
                    } else {
                        // A load clears any selected driver.
                        for (const d of drivers) selected.delete(d.id);
                    }
                    selected.add(pin.id);
                } else {
                    selected.delete(pin.id);
                }
                renderPins();
                updateOk();
            });
            row.append(cb, document.createTextNode(' ' + pin.label));
            pinBox.appendChild(row);
        };
        drivers.forEach(p => add(p, true));
        loads.forEach(p => add(p, false));
    }
    // Preselect the driver (like the Qt dialog).
    if (drivers.length) selected.add(drivers[0].id);
    renderPins();

    const masterSelect = document.createElement('select');
    repopulate(masterSelect, info.masters || []);  // defaults to first option
    const bufName = document.createElement('input');
    bufName.placeholder = 'Optional: new buffer name';
    const netNameInput = document.createElement('input');
    netNameInput.placeholder = 'Optional: new net name';

    m.body.append(
        labelWrap('Pins to buffer', pinBox),
        labelWrap('Buffer master', masterSelect),
        labelWrap('Buffer name', bufName),
        labelWrap('Net name', netNameInput));

    function updateOk() {
        okBtn.disabled = selected.size === 0 || !masterSelect.value;
    }
    masterSelect.addEventListener('change', updateOk);
    updateOk();

    okBtn.addEventListener('click', () => {
        const driverIds = new Set(drivers.map(d => d.id));
        const picked = [...selected];
        const mode = picked.some(id => driverIds.has(id)) ? 'driver' : 'loads';
        app.websocketManager.request({
            type: 'insert_buffer',
            net: netName,
            mode,
            pins: picked,
            master: masterSelect.value,
            buf_name: bufName.value.trim(),
            net_name: netNameInput.value.trim(),
        }).then(resp => {
            if (resp.is_error || resp.ok === false) {
                showError(m.errorDiv, resp.error || resp.result || 'Insertion failed');
                return;
            }
            m.close();
            // Buffer is placed at the pin (renders immediately). Select it so
            // the user can inspect it, and refresh the layout.
            if (resp.inst) app.selectedInstanceName = resp.inst;
            if (app.scheduleRedrawAllLayers) app.scheduleRedrawAllLayers();
        }).catch(err => showError(m.errorDiv, err.message || String(err)));
    });
}

// ── small helpers ─────────────────────────────────────────────────────────────
function labelWrap(label, el) {
    const wrap = document.createElement('label');
    wrap.className = 'edit-field';
    const span = document.createElement('span');
    span.textContent = label;
    wrap.append(span, el);
    return wrap;
}
function repopulate(select, options) {
    const prev = select.value;
    select.innerHTML = '';
    for (const opt of options) {
        const o = document.createElement('option');
        o.value = opt;
        o.textContent = opt;
        select.appendChild(o);
    }
    if (options.includes(prev)) select.value = prev;
}
function esc(s) {
    return String(s == null ? '' : s)
        .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}
