// DRC Destroyer — Arcade game for routing visualization
// This is just a fun feature and has no practical value.
"use strict";

// These constants are injected by game.py at build time
// const GAME_DATA = ...;
// const INTRO_LINES = ...;
// const WARNING_VILLAINS = ...;
// const LAYER_COLORS = ...;

const canvas = document.getElementById("c");
const ctx = canvas.getContext("2d");
const W = 960, H = 700;
canvas.width = W; canvas.height = H;

// --------------- COORDINATE HELPERS ---------------

const die = GAME_DATA.die;
const dieW = Math.max(die.xMax - die.xMin, 1);
const dieH = Math.max(die.yMax - die.yMin, 1);

function norm(x, y) {
    return [(x - die.xMin) / dieW, (y - die.yMin) / dieH];
}

function iso(nx, ny, z) {
    const sx = W * 0.1 + nx * W * 0.75 - ny * W * 0.15;
    const sy = H * 0.12 + ny * H * 0.65 + nx * H * 0.1 - (z || 0) * 6;
    return [sx, sy];
}

// --------------- LAYER HELPERS ---------------

const LAYER_Z = {};
["M1","M2","M3","M4","M5","M6","M7","M8","M9","Pad"].forEach((l,i) => LAYER_Z[l] = i);

function layerZ(name) {
    for (const k in LAYER_Z) if (name.includes(k)) return LAYER_Z[k];
    return 0;
}

function layerColor(name) {
    for (const k in LAYER_COLORS) if (name.includes(k)) return LAYER_COLORS[k];
    return "#888";
}

// --------------- PRECOMPUTE DATA ---------------

const wires = GAME_DATA.wires.map(w => {
    const [n1x, n1y] = norm(w.x1, w.y1);
    const [n2x, n2y] = norm(w.x2, w.y2);
    const z = layerZ(w.layer) * 2;
    const [sx1, sy1] = iso(n1x, n1y, z);
    const [sx2, sy2] = iso(n2x, n2y, z);
    return { sx1, sy1, sx2, sy2, color: layerColor(w.layer), z };
}).sort((a, b) => a.z - b.z);

const DRC_COLORS = {
    "Short": "#ff2222", "CutSpcTbl": "#cc44ff", "eolKeepOut": "#ffaa00",
    "Metal Spacing": "#ff8800", "EOL": "#ffff00", "Recheck": "#00ffcc"
};

const targets = GAME_DATA.drc.map(d => {
    const cx = (d.xMin + d.xMax) / 2, cy = (d.yMin + d.yMax) / 2;
    const [nx, ny] = norm(cx, cy);
    const z = layerZ(d.layer) * 2;
    const [sx, sy] = iso(nx, ny, z);
    return { sx, sy, nx, ny, type: d.type, layer: d.layer, alive: true,
             color: DRC_COLORS[d.type] || "#ff4444" };
});

// If no DRC targets, generate training exercise
if (targets.length === 0) {
    for (let i = 0; i < 20; i++) {
        const nx = 0.1 + Math.random() * 0.8;
        const ny = 0.1 + Math.random() * 0.8;
        const [sx, sy] = iso(nx, ny, 4);
        targets.push({ sx, sy, nx, ny, type: "Training", layer: "M3",
                       alive: true, color: "#44ff44" });
    }
}

// Warning markers (placed at random positions on chip)
const warningMarkers = WARNING_VILLAINS.map(v => ({
    name: v.name, quote: v.quote,
    nx: 0.1 + Math.random() * 0.8, ny: 0.1 + Math.random() * 0.8,
    alive: true
}));
warningMarkers.forEach(w => {
    const [sx, sy] = iso(w.nx, w.ny, 10);
    w.sx = sx; w.sy = sy;
});

// --------------- GAME STATE ---------------

let state = "intro"; // intro, attract, play
let tick = 0;
let score = 0;
let bonusActive = true;
let manualMode = false;
let autopilotTarget = 0;
let lastManualInput = 0;
let pilotAsleep = false;
const bombs = [];
const explosions = [];
const keys = {};

let camNx = 0.5, camNy = 0.5;
const camSpeed = 0.005;

let introY = H + 50;
const introSpeed = 0.35;
const introLineHeight = 42;

// Cell ticker
const cells = GAME_DATA.cells || [];
const tickerMessages = [];
let tickerOffset = 0;
const SECTOR_NAMES = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

function randomSector() {
    return Math.floor(Math.random()*9+1) + "-" + SECTOR_NAMES[Math.floor(Math.random()*26)];
}

const CELL_QUIPS = [
    "spotted at sector", "detected in zone", "observed near sector",
    "identified at grid", "lurking in sector"
];

// --------------- INPUT ---------------

document.addEventListener("keydown", e => {
    keys[e.key] = true;
    if (state === "intro") { state = "attract"; }
    if (state === "attract" && (e.key === " " || ["ArrowLeft","ArrowRight","ArrowUp","ArrowDown"].includes(e.key))) {
        state = "play"; manualMode = true; lastManualInput = tick;
        score = 0; bonusActive = true; pilotAsleep = false;
        targets.forEach(t => t.alive = true);
        warningMarkers.forEach(w => w.alive = true);
    }
    if (state === "play") {
        if (["ArrowLeft","ArrowRight","ArrowUp","ArrowDown"].includes(e.key)) {
            lastManualInput = tick;
            if (pilotAsleep) { pilotAsleep = false; }
            if (!manualMode) { manualMode = true; }
        }
        if (e.key === " " && manualMode) {
            bombs.push({ nx: camNx, ny: camNy, z: 80, speed: 3 });
        }
    }
    if (e.key === "d" || e.key === "D") {
        manualMode = !manualMode;
        if (manualMode) lastManualInput = tick;
    }
});
document.addEventListener("keyup", e => { keys[e.key] = false; });

// --------------- UPDATE ---------------

function update() {
    tick++;

    if (state === "intro") {
        introY -= introSpeed;
        if (introY < -INTRO_LINES.length * introLineHeight - 100) state = "attract";
        return;
    }

    const isAutopilot = state === "attract" || (state === "play" && !manualMode);

    if (state === "play" && manualMode && tick - lastManualInput > 150) {
        pilotAsleep = true;
        manualMode = false;
        bonusActive = false;
    }

    if (isAutopilot) {
        const aliveTargets = targets.filter(t => t.alive);
        const aliveWarnings = warningMarkers.filter(w => w.alive);
        const allTargets = [...aliveTargets.map(t => ({nx: t.nx, ny: t.ny})),
                           ...aliveWarnings.map(w => ({nx: w.nx, ny: w.ny}))];
        if (allTargets.length > 0) {
            const tgt = allTargets[autopilotTarget % allTargets.length];
            const dx = tgt.nx - camNx, dy = tgt.ny - camNy;
            const dist = Math.sqrt(dx*dx + dy*dy);
            if (dist > 0.01) {
                camNx += (dx / dist) * camSpeed * 1.2;
                camNy += (dy / dist) * camSpeed * 1.2;
            } else {
                bombs.push({ nx: camNx, ny: camNy, z: 80, speed: 3 });
                autopilotTarget++;
            }
        } else {
            camNx += Math.sin(tick * 0.02) * camSpeed;
            camNy += Math.cos(tick * 0.015) * camSpeed;
        }
        if (state === "attract" && tick % 20 === 0 && allTargets.length > 0) {
            bombs.push({ nx: camNx, ny: camNy, z: 80, speed: 3 });
        }
    } else {
        if (keys["ArrowLeft"]) camNx -= camSpeed;
        if (keys["ArrowRight"]) camNx += camSpeed;
        if (keys["ArrowUp"]) camNy -= camSpeed;
        if (keys["ArrowDown"]) camNy += camSpeed;
    }
    camNx = Math.max(0, Math.min(1, camNx));
    camNy = Math.max(0, Math.min(1, camNy));

    // Update bombs
    for (let i = bombs.length - 1; i >= 0; i--) {
        bombs[i].z -= bombs[i].speed;
        if (bombs[i].z <= 0) {
            const b = bombs[i];
            explosions.push({ nx: b.nx, ny: b.ny, tick: tick, dur: 25 });
            detonate(b.nx, b.ny);
            bombs.splice(i, 1);
        }
    }

    // Update explosions
    for (let i = explosions.length - 1; i >= 0; i--) {
        if (tick - explosions[i].tick > explosions[i].dur) explosions.splice(i, 1);
    }

    // Cell ticker
    if (tick % 300 === 0 && cells.length > 0) {
        const cell = cells[Math.floor(Math.random() * cells.length)];
        const quip = CELL_QUIPS[Math.floor(Math.random() * CELL_QUIPS.length)];
        tickerMessages.push({
            text: `${cell} ${quip} ${randomSector()}`,
            color: layerColor(cell.includes("BUF") ? "M1" : cell.includes("INV") ? "M2" : "M3"),
            x: W + 10
        });
    }
    for (const msg of tickerMessages) msg.x -= 1.0;
    while (tickerMessages.length > 0 && tickerMessages[0].x < -600) tickerMessages.shift();
}

function detonate(bx, by) {
    const blastR = 0.04;
    for (const t of targets) {
        if (!t.alive) continue;
        const dx = t.nx - bx, dy = t.ny - by;
        if (Math.sqrt(dx*dx + dy*dy) < blastR) {
            t.alive = false;
            score += bonusActive ? 20 : 10;
        }
    }
    for (const w of warningMarkers) {
        if (!w.alive) continue;
        const dx = w.nx - bx, dy = w.ny - by;
        if (Math.sqrt(dx*dx + dy*dy) < blastR) {
            w.alive = false;
            score += bonusActive ? 20 : 10;
        }
    }
}

// --------------- DRAW ---------------

function draw() {
    ctx.fillStyle = "#0a0a1a";
    ctx.fillRect(0, 0, W, H);

    if (state === "intro") { drawIntro(); return; }

    drawGrid();
    drawWires();
    drawTargets();
    drawWarningMarkers();
    drawBombs();
    drawExplosions();
    drawBomber();
    drawHUD();
    drawTicker();

    if (state === "attract") drawAttract();
    if (state === "play") drawPlayHUD();

    if (state === "play") {
        const drcLeft = targets.filter(t => t.alive).length;
        const warnLeft = warningMarkers.filter(w => w.alive).length;
        if (drcLeft === 0 && warnLeft === 0) drawVictory();
    }
}

function drawIntro() {
    // True 3D perspective crawl — vanishing point at top center
    const vpX = W / 2;
    const vpY = H * 0.08;
    const floorY = H * 1.1;
    const nearScale = 1.0;
    const farScale = 0.15;

    ctx.save();
    for (let i = 0; i < INTRO_LINES.length; i++) {
        const rawY = introY + i * introLineHeight;
        if (rawY < -100 || rawY > H + 100) continue;

        const depth = Math.max(0, Math.min(1, (rawY - vpY) / (floorY - vpY)));
        if (depth <= 0.01) continue;

        const scale = farScale + (nearScale - farScale) * depth;
        const screenY = vpY + (floorY - vpY) * (1 - Math.pow(1 - depth, 1.5));
        if (screenY < vpY - 10 || screenY > H + 20) continue;

        const alpha = Math.min(1, depth * 2.5);
        const line = INTRO_LINES[i];
        let fontSize, fontWeight, color;

        if (line.includes("DRC DESTROYER")) {
            fontSize = 52; fontWeight = "bold"; color = "#ff4444";
        } else if (line === line.toUpperCase() && line.length > 3) {
            fontSize = 34; fontWeight = "bold"; color = "#ffcc00";
        } else {
            fontSize = 28; fontWeight = ""; color = `rgba(100,200,255,${alpha})`;
        }

        ctx.save();
        ctx.translate(vpX, screenY);
        ctx.scale(scale, scale);
        ctx.globalAlpha = alpha;
        ctx.font = `${fontWeight} ${fontSize}px Courier`;
        ctx.fillStyle = color;
        ctx.textAlign = "center";
        ctx.fillText(line, 0, 0);
        ctx.restore();
    }
    ctx.restore();

    // Skip hint
    const skipFlash = Math.floor(tick / 25) % 2 === 0;
    ctx.fillStyle = skipFlash ? "#ff2222" : "#881111";
    ctx.font = "bold 18px Courier";
    ctx.textAlign = "center";
    ctx.fillText(">>> PRESS ANY KEY TO SKIP <<<", W/2, H - 20);
    drawControlsLegend();
}

function drawGrid() {
    ctx.strokeStyle = "#1a1a2e";
    ctx.lineWidth = 0.5;
    for (let i = 0; i <= 10; i++) {
        const t = i / 10;
        const [x0, y0] = iso(t, 0, 0);
        const [x1, y1] = iso(t, 1, 0);
        ctx.beginPath(); ctx.moveTo(x0, y0); ctx.lineTo(x1, y1); ctx.stroke();
        const [x2, y2] = iso(0, t, 0);
        const [x3, y3] = iso(1, t, 0);
        ctx.beginPath(); ctx.moveTo(x2, y2); ctx.lineTo(x3, y3); ctx.stroke();
    }
}

function drawWires() {
    ctx.lineWidth = 1;
    for (const w of wires) {
        ctx.strokeStyle = w.color;
        ctx.globalAlpha = 0.5;
        ctx.beginPath();
        ctx.moveTo(w.sx1, w.sy1);
        ctx.lineTo(w.sx2, w.sy2);
        ctx.stroke();
    }
    ctx.globalAlpha = 1;
}

function drawTargets() {
    for (const t of targets) {
        if (!t.alive) continue;
        const pulse = 0.7 + 0.3 * Math.sin(tick * 0.15);
        const r = 6 + 4 * pulse;
        ctx.beginPath();
        ctx.arc(t.sx, t.sy, r, 0, Math.PI * 2);
        ctx.fillStyle = t.color;
        ctx.globalAlpha = pulse;
        ctx.fill();
        ctx.globalAlpha = 1;
        ctx.strokeStyle = "#fff";
        ctx.lineWidth = 1;
        ctx.stroke();
        ctx.fillStyle = "#fff";
        ctx.font = "bold 13px Courier";
        ctx.textAlign = "center";
        ctx.fillText(t.type, t.sx, t.sy - r - 5);
    }
}

function drawWarningMarkers() {
    for (const w of warningMarkers) {
        if (!w.alive) continue;
        const pulse = 0.5 + 0.5 * Math.sin(tick * 0.1 + 1);
        ctx.save();
        ctx.translate(w.sx, w.sy);
        ctx.beginPath();
        ctx.moveTo(0, -16); ctx.lineTo(14, 10); ctx.lineTo(-14, 10); ctx.closePath();
        ctx.fillStyle = `rgba(255,200,0,${pulse})`;
        ctx.fill();
        ctx.strokeStyle = "#ff8800";
        ctx.lineWidth = 2;
        ctx.stroke();
        ctx.fillStyle = "#000";
        ctx.font = "bold 16px Courier";
        ctx.textAlign = "center";
        ctx.fillText("!", 0, 7);
        ctx.fillStyle = "#ffcc00";
        ctx.font = "bold 11px Courier";
        ctx.fillText(w.name, 0, -22);
        ctx.restore();
    }
}

function drawBombs() {
    for (const b of bombs) {
        const [sx, sy] = iso(b.nx, b.ny, b.z / 4);
        const size = Math.max(2, 6 - b.z / 20);
        ctx.beginPath();
        ctx.arc(sx, sy, size, 0, Math.PI * 2);
        ctx.fillStyle = "#ff0000";
        ctx.fill();
        ctx.strokeStyle = "#ff8800";
        ctx.lineWidth = 1;
        ctx.stroke();
        const [gx, gy] = iso(b.nx, b.ny, 0);
        ctx.beginPath();
        ctx.ellipse(gx, gy, 3, 1.5, 0, 0, Math.PI * 2);
        ctx.fillStyle = "#330000";
        ctx.fill();
    }
}

function drawExplosions() {
    for (const e of explosions) {
        const age = tick - e.tick;
        const p = age / e.dur;
        const [sx, sy] = iso(e.nx, e.ny, 0);
        const r = 40 * p;
        const colors = ["#ff4400", "#ff8800", "#ffcc00", "#ff6600"];
        const c = colors[Math.min(Math.floor(p * 4), 3)];
        if (r > 1) {
            ctx.beginPath();
            ctx.arc(sx, sy, r, 0, Math.PI * 2);
            ctx.strokeStyle = c;
            ctx.lineWidth = 2;
            ctx.globalAlpha = 1 - p;
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(sx, sy, r * 0.4, 0, Math.PI * 2);
            ctx.fillStyle = c;
            ctx.fill();
            ctx.globalAlpha = 1;
        }
    }
}

function drawBomber() {
    const [bx, by] = iso(camNx, camNy, 16);
    ctx.strokeStyle = "#00ff00";
    ctx.lineWidth = 2;
    ctx.beginPath(); ctx.moveTo(bx - 15, by); ctx.lineTo(bx + 15, by); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(bx, by - 15); ctx.lineTo(bx, by + 15); ctx.stroke();
    ctx.beginPath(); ctx.arc(bx, by, 8, 0, Math.PI * 2); ctx.stroke();
    const [sx, sy] = iso(camNx, camNy, 0);
    ctx.beginPath();
    ctx.ellipse(sx, sy, 5, 2, 0, 0, Math.PI * 2);
    ctx.fillStyle = "#003300";
    ctx.fill();
}

function drawHUD() {
    ctx.fillStyle = "#00ff00";
    ctx.font = "bold 16px Courier";
    ctx.textAlign = "left";
    ctx.fillText(`SCORE: ${String(score).padStart(6, "0")}`, 10, 25);

    const drcLeft = targets.filter(t => t.alive).length;
    const warnLeft = warningMarkers.filter(w => w.alive).length;

    ctx.fillStyle = "#ff4444";
    ctx.font = "bold 14px Courier";
    ctx.fillText(`DRC: ${drcLeft}`, 10, 48);
    ctx.fillStyle = "#ffaa00";
    ctx.fillText(`WARNINGS: ${warnLeft}`, 10, 66);

    ctx.fillStyle = "#00aaff";
    ctx.font = "10px Courier";
    ctx.fillText(GAME_DATA.design_name, 10, H - 25);
    ctx.fillStyle = "#444";
    ctx.fillText("This is just a fun feature and has no practical value.", 10, H - 10);
}

function drawControlsLegend() {
    const bx = W - 260, by = 10, bw = 250, bh = 110;
    ctx.fillStyle = "rgba(0,0,0,0.85)";
    ctx.fillRect(bx, by, bw, bh);
    ctx.strokeStyle = "#333";
    ctx.lineWidth = 1;
    ctx.strokeRect(bx, by, bw, bh);

    ctx.fillStyle = "#00ccff";
    ctx.font = "bold 14px Courier";
    ctx.textAlign = "center";
    ctx.fillText("CONTROLS", bx + bw / 2, by + 20);

    const controls = [
        ["SPACE", "Start / Bomb"],
        ["\u2190\u2191\u2192\u2193 Arrows", "Move"],
        ["D", "Autopilot"],
    ];
    for (let i = 0; i < controls.length; i++) {
        const y = by + 42 + i * 22;
        ctx.fillStyle = "#ffcc00";
        ctx.font = "13px Courier";
        ctx.textAlign = "right";
        ctx.fillText(controls[i][0], bx + bw / 2 - 5, y);
        ctx.fillStyle = "#888";
        ctx.textAlign = "left";
        ctx.fillText(controls[i][1], bx + bw / 2 + 5, y);
    }
}

function drawAttract() {
    const flash = Math.floor(tick / 45) % 2 === 0;
    if (flash) {
        ctx.fillStyle = "#ffff00";
        ctx.font = "bold 34px Courier";
        ctx.textAlign = "center";
        ctx.fillText("INSERT COIN", W / 2, H / 2 - 60);
    }
    ctx.fillStyle = "#ff4444";
    ctx.font = "bold 42px Courier";
    ctx.textAlign = "center";
    ctx.fillText("DRC DESTROYER", W / 2, H / 2);

    ctx.fillStyle = "#888";
    ctx.font = "14px Courier";
    ctx.fillText("Press SPACE or arrow key to start", W / 2, H / 2 + 35);
    ctx.fillStyle = "#555";
    ctx.font = "12px Courier";
    ctx.fillText("This is just a fun feature and has no practical value.", W / 2, H / 2 + 60);

    drawControlsLegend();
}

function drawPlayHUD() {
    ctx.textAlign = "right";
    ctx.font = "12px Courier";
    if (pilotAsleep) {
        const flash = Math.floor(tick / 15) % 2 === 0;
        ctx.fillStyle = flash ? "#ff0000" : "#880000";
        ctx.fillText("WARNING: PILOT ASLEEP!", W - 10, 25);
        ctx.fillStyle = "#ff4444";
        ctx.fillText("Autopilot engaged. Bonus voided.", W - 10, 42);
    } else if (manualMode) {
        ctx.fillStyle = "#00ff00";
        ctx.fillText("MANUAL", W - 10, 25);
        ctx.fillStyle = bonusActive ? "#ffff00" : "#666";
        ctx.fillText(bonusActive ? "BONUS: x2" : "BONUS: VOIDED", W - 10, 42);
    } else {
        const flash = Math.floor(tick / 20) % 2 === 0;
        ctx.fillStyle = flash ? "#00aaff" : "#004466";
        ctx.fillText("AUTOPILOT", W - 10, 25);
    }

    ctx.fillStyle = "#555";
    ctx.font = "13px Courier";
    ctx.textAlign = "right";
    ctx.fillText("[Arrows] Move  [Space] Bomb  [D] Toggle autopilot", W - 10, H - 30);
}

function drawTicker() {
    ctx.save();
    ctx.beginPath();
    ctx.rect(0, H - 32, W, 32);
    ctx.clip();
    ctx.font = "bold 18px Courier";
    for (const msg of tickerMessages) {
        const flash = Math.floor(tick / 20) % 2 === 0;
        ctx.fillStyle = flash ? msg.color : "#333";
        ctx.textAlign = "left";
        ctx.fillText(msg.text, msg.x, H - 10);
    }
    ctx.restore();
}

function drawVictory() {
    ctx.fillStyle = "rgba(0,0,0,0.7)";
    ctx.fillRect(0, H/2 - 60, W, 120);
    ctx.fillStyle = "#00ff00";
    ctx.font = "bold 32px Courier";
    ctx.textAlign = "center";
    ctx.fillText("LEVEL COMPLETE!", W/2, H/2 - 15);
    ctx.fillStyle = "#ffff00";
    ctx.font = "18px Courier";
    ctx.fillText(`Final Score: ${score}${bonusActive ? " (with bonus!)" : ""}`, W/2, H/2 + 20);
    ctx.fillStyle = "#888";
    ctx.font = "12px Courier";
    ctx.fillText("All DRC violations and warnings cleared!", W/2, H/2 + 45);
}

// --------------- MAIN LOOP ---------------

function loop() {
    update();
    draw();
    requestAnimationFrame(loop);
}
loop();
