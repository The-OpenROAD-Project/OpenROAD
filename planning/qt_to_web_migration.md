<!-- CLAUDE AUTO-UPDATE INSTRUCTIONS
When reviewing this document, Claude should automatically:
1. Check git log for commits related to the qt-to-web migration (grep for "web", "gui", "api", "frontend", "react", "fastapi" in commit messages)
2. Update the "Progress Tracker" section below with:
   - Last updated date (today's date)
   - Completed tasks (based on merged commits)
   - Current phase and task in progress
   - Projected end date (extrapolate from velocity: tasks_completed / elapsed_time * remaining_tasks)
   - Actual vs planned timeline comparison
3. Recalculate the Gantt chart if milestones have shifted
4. Flag any tasks that are behind schedule

Run: cd planning && python3 gantt_chart.py
to regenerate charts after updating progress.
-->

# OpenROAD: Qt to Web Interface Migration Plan

## Progress Tracker

| Field | Value |
|-------|-------|
| **Last Updated** | 2026-03-12 |
| **Current Phase** | Pre-project (Planning) |
| **Tasks Completed** | 0 / 34 |
| **Planned Start** | 2026-04-01 |
| **Projected End (Classic)** | 2027-12-01 |
| **Projected End (AI-Assisted)** | 2027-05-01 |
| **Status** | Planning and approval |

---

## AI No-Estimates Planning

This project plan uses **AI No-Estimates Planning** - a modern approach where traditional time estimates are replaced by AI-driven analysis and continuous recalibration. Instead of asking humans to guess how long tasks will take (a notoriously unreliable process - studies show estimates are off by 2-3x on average), we:

1. **Let AI analyze comparable projects** to derive duration ranges from historical data rather than human intuition
2. **Continuously recalibrate** projections based on actual commit velocity and task completion rates
3. **Track flow metrics** (cycle time, throughput) rather than estimates - what matters is how fast work actually moves, not how fast we predicted it would
4. **Use two scenarios** (classic vs AI-assisted) as uncertainty bounds rather than false-precision single estimates
5. **Update automatically** - Claude reviews git history on each planning session and adjusts projections based on observed velocity

The durations in this document are **AI-generated projections based on research of comparable migrations**, not human estimates. They will be automatically refined as the project progresses and real velocity data becomes available. This eliminates the cognitive overhead and political games around estimation while producing more accurate forecasts over time.

> "The best estimate is no estimate. Ship small increments, measure throughput, and let the data tell you when you'll be done." - adapted from the #NoEstimates movement

---

## Executive Summary

This document proposes migrating OpenROAD's Qt5/Qt6-based desktop GUI (~44,000 lines of C++) to a modern web-based architecture. Two development scenarios are compared: **Classic Development** (20 months, 9 FTE, ~155 person-months) and **AI-Assisted Development with Claude** (13 months, 6 FTE, ~68 person-months). The migration addresses licensing complexity (Qt LGPL/GPL vs OpenROAD's BSD-3-Clause), deployment friction in containerized/cloud environments, and positions OpenROAD for collaborative, browser-based EDA workflows.

---

## 1. Current State Analysis

### Qt Usage in OpenROAD

| Aspect | Details |
|--------|---------|
| Qt Version | Qt5 (CMake), Qt6 (Bazel) |
| Qt Modules | Core, Widgets, Charts, OpenGL |
| Code Size | ~44,318 lines of C++ GUI code |
| Components | 48+ headers, 60+ source files, 4 .ui files |
| Architecture | Renderer/Plugin pattern with abstract Painter API |
| Scripting | Full TCL interface via SWIG |
| Headless | Supported via `BUILD_GUI=OFF` |

### Key GUI Features

1. **Layout Viewer** - Interactive 2D chip layout with zoom, pan, rubber-band selection, OpenGL rendering
2. **Timing Widget** - Path timing visualization, slack/delay analysis, histograms
3. **DRC Viewer** - Design rule check violation browsing and visualization
4. **Clock Tree Viewer** - Clock hierarchy visualization with power analysis
5. **Inspector** - Database object property browsing and editing
6. **Script Widget** - Interactive TCL command execution
7. **Display Controls** - Layer visibility, object filtering
8. **Charts Widget** - Multi-axis plotting for analysis data
9. **Heat Maps** - Pin density, placement density visualization
10. **Chiplet 3D Viewer** - Multi-chip design visualization

### Pain Points with Qt

- **Licensing**: Qt Charts was GPL-only until Qt 5.7; LGPL compliance requires dynamic linking
- **Cloud deployment**: `could not connect to display` errors in headless Docker environments (GitHub Issue #1055)
- **Build friction**: `qt5-default` package obsolete on Ubuntu 22.04+ (GitHub Issue #2247)
- **KLayout dependency**: Similar Qt issues for KLayout in Docker (GitHub Issue #3163)
- **Heavy dependency**: Qt adds significant build time and binary size

---

## 2. Recommended Web Framework Stack

### Backend: FastAPI (Python, MIT License)

| Criterion | Assessment |
|-----------|-----------|
| License | MIT - fully compatible with BSD-3-Clause |
| Performance | Async-native, high throughput for concurrent design jobs |
| API Documentation | Automatic OpenAPI/Swagger docs |
| WebSocket Support | Native, for real-time layout updates |
| Python Integration | Direct integration with OpenROAD's Python bindings |
| Community | 75k+ GitHub stars, active ecosystem |

### Frontend: React + WebGL (MIT License)

| Criterion | Assessment |
|-----------|-----------|
| License | MIT - fully compatible with BSD-3-Clause |
| Layout Rendering | WebGL via deck.gl or custom Canvas renderer |
| Charting | Plotly.js, Recharts, or D3.js for timing/analysis charts |
| State Management | Redux or Zustand for complex application state |
| Component Ecosystem | Largest ecosystem of UI component libraries |
| Developer Pool | Largest frontend developer community |

### Alternative Stacks Considered

| Stack | Pros | Cons | Verdict |
|-------|------|------|---------|
| Flask + Vue | Simpler, easier learning curve | Less performant async, smaller ecosystem | Good for smaller team |
| Django + React | Full-featured backend (ORM, admin) | Heavier than needed for API-only backend | Overkill |
| FastAPI + Svelte | Best runtime performance | Smaller ecosystem, fewer EDA visualization libs | Promising but risky |
| Tauri (Rust + Web) | Native feel, small binary | Still requires local install, no cloud benefit | Not aligned with cloud goals |

---

## 3. Architecture Design

```
+-------------------------------------------------------------+
|                    Browser (React + WebGL)                    |
|  +----------+ +----------+ +----------+ +--------------+    |
|  | Layout   | | Timing   | | DRC      | | Clock Tree   |    |
|  | Viewer   | | Widget   | | Viewer   | | Viewer       |    |
|  +----+-----+ +----+-----+ +----+-----+ +------+-------+    |
|       +-------------+------------+--------------+            |
|                         REST + WebSocket API                 |
+-----------------------------+-------------------------------+
                              | HTTPS / WSS
+-----------------------------+-------------------------------+
|                  FastAPI Backend Server                       |
|  +----------+ +----------+ +----------+ +--------------+    |
|  | Layout   | | Timing   | | DRC      | | Job Queue    |    |
|  | API      | | API      | | API      | | (Celery)     |    |
|  +----+-----+ +----+-----+ +----+-----+ +------+-------+    |
|       +-------------+------------+--------------+            |
|                    Python/TCL Bindings                        |
+-----------------------------+-------------------------------+
                              |
+-----------------------------+-------------------------------+
|              OpenROAD Core (C++ Engine)                       |
|  +------+ +------+ +------+ +------+ +------+ +--------+   |
|  | ODB  | | STA  | | GPL  | | DRT  | | CTS  | | Others |   |
|  +------+ +------+ +------+ +------+ +------+ +--------+   |
+-------------------------------------------------------------+
```

### Key Design Principles

1. **API-First**: Every GUI action maps to a REST endpoint; CLI and scripting remain first-class
2. **Progressive Rendering**: WebGL with level-of-detail for million-polygon layouts
3. **Async Jobs**: Long-running EDA operations via async job queue with WebSocket status updates
4. **Backward Compatible**: TCL scripts continue to work unchanged
5. **Containerized**: Docker-native deployment, no X11/display dependency

---

## 4. Projects That Migrated from Qt to Web

| Project | Domain | Migration Approach | Outcome |
|---------|--------|-------------------|---------|
| **qBittorrent** | Torrent client | Added WebUI alongside Qt; REST API layer | Multiple community-built WebUIs (Angular, React, Vue) |
| **Planorama OpenROAD Cloud** | EDA (OpenROAD) | Built web SaaS wrapping OpenROAD engine | Production cloud EDA platform with real-time monitoring |
| **EasyEDA** | PCB Design | Built as web-native from start | Fully browser-based EDA with 2D/3D visualization |
| **KLayout** | EDA Layout | Added `-without-qt` build option | Headless mode for Docker/CI pipelines |
| **Siemens Xpedition** | PCB Review | Cloud-native viewer | Browser-based review without authoring tool licenses |
| **EPWave** | Waveform Viewer | Web-native on EDA Playground | Browser-based waveform analysis |

### Key Lessons Learned

- **qBittorrent pattern**: Keep native GUI as optional, add API layer first, let community build frontends
- **Planorama precedent**: OpenROAD specifically has been successfully deployed as a web SaaS - proves feasibility
- **Incremental migration**: All successful projects added web capability alongside existing GUI before retiring it

---

## 5. AI Productivity Research: Classic vs Claude-Assisted Development

### Research Sources and Findings

| Source | Study Type | Key Finding |
|--------|-----------|-------------|
| **Anthropic Internal** (2025) | 132 engineers surveyed | 67% increase in merged PRs/day; ~50% self-reported productivity gain |
| **Anthropic Economic** (2025) | Task-level analysis | ~80% median time savings on individual tasks |
| **GitHub/Microsoft RCT** (Peng et al., 2023) | Randomized controlled trial | 55.8% faster task completion with Copilot (p=0.0017) |
| **McKinsey** (2024) | Enterprise study | 16-30% team productivity improvement; 31-45% quality improvement |
| **BCG/Harvard** (2024) | Controlled experiment | 25.1% faster, 40% higher quality with GPT-4 |
| **BCG** (2024) | Enterprise deployment | 30-50% coder augmentation productivity gains |
| **Thoughtworks** (2024) | End-to-end analysis | ~8% net delivery improvement (coding is only half of cycle time) |
| **METR** (2025) | RCT, experienced devs | 19% SLOWER for experienced devs on large familiar codebases |
| **Systematic Review** (37 studies) | Literature review | 18% code quality improvement; velocity gains fade after 1-2 months |

### Applied Productivity Model for This Project

Based on the research, we apply differentiated acceleration rates by task type:

| Task Category | AI Acceleration | Rationale |
|--------------|----------------|-----------|
| **Scaffolding / boilerplate** | 50-60% faster | Highest AI impact area (Anthropic, GitHub studies) |
| **Component development** | 30-40% faster | Significant but needs domain knowledge (McKinsey) |
| **Testing** | 30-40% faster | AI generates test scaffolds; human reviews (BCG) |
| **Documentation** | 50-70% faster | Strong AI capability (Anthropic: 87% time savings) |
| **Architecture / design** | 10-15% faster | Primarily human judgment (METR study caveat) |
| **Deployment / DevOps** | 15-20% faster | Mix of automation and human process |
| **User training / feedback** | 5-10% faster | Inherently human-driven |

**Blended project acceleration: ~35% timeline reduction, ~56% effort reduction** (fewer FTE needed due to AI augmentation of each developer's output).

### Important Caveats

1. **METR study warning**: Experienced developers on large, mature codebases they already know well may see NO benefit or even slowdown
2. **Velocity plateau**: Project-level gains are strongest in first 1-2 months, then return to baseline
3. **End-to-end vs task-level**: Individual task speedups (25-56%) compress to ~8% at the delivery level due to reviews, dependencies, and process overhead
4. **Quality risk**: Anthropic found AI-assisted group scored 17% lower on coding mastery quizzes

---

## 6. Scenario Comparison

### Scenario A: Classic Development

| Metric | Value |
|--------|-------|
| Duration | **20 months** |
| Team Size (Peak) | **9 FTE** |
| Total Effort | **~155 person-months** |
| MVP Delivery | Month 7 |
| Feature Parity | Month 11 |
| Production Ready | Month 14 |
| Qt Retired | Month 20 |

### Scenario B: AI-Assisted Development (Claude)

| Metric | Value |
|--------|-------|
| Duration | **13 months** |
| Team Size (Peak) | **6 FTE** |
| Total Effort | **~68 person-months** |
| MVP Delivery | Month 5 |
| Feature Parity | Month 7.5 |
| Production Ready | Month 9.5 |
| Qt Retired | Month 13 |

### Savings Summary

| Metric | Classic | AI-Assisted | Savings |
|--------|---------|-------------|---------|
| Duration | 20 months | 13 months | **35% faster** |
| Peak Team | 9 FTE | 6 FTE | **33% smaller** |
| Total Effort | 155 PM | 68 PM | **56% reduction** |
| Est. Cost (@$15k/PM) | $2.33M | $1.02M | **$1.31M saved** |

---

## 7. Resource Plan

### Team Roles — Classic Development (9 FTE)

| Role | Count | Key Skills | Responsibilities |
|------|-------|-----------|-----------------|
| Tech Lead / Architect | 1 | C++, Python, System Design, EDA | Architecture decisions, API design, team coordination |
| Backend Engineer (Sr) | 2 | Python, FastAPI, WebSocket, C++ | REST API, OpenROAD Python bindings, job queue |
| Frontend Engineer (Sr) | 2 | React, TypeScript, WebGL, Canvas | Layout viewer, timing widgets, DRC viewer |
| Frontend Engineer (Mid) | 1 | React, TypeScript | UI components, forms, controls, inspector |
| DevOps / Infrastructure | 1 | Docker, K8s, CI/CD, Nginx | Containerization, deployment, reverse proxy |
| QA / Test Engineer | 1 | Selenium, Playwright, Python | E2E testing, visual regression, performance testing |
| UX Designer (0.5) + Tech Writer (0.5) | 1 | Figma, Documentation | UI/UX design, migration guide, API reference |

### Team Roles — AI-Assisted Development (6 FTE)

| Role | Count | Key Skills | Responsibilities |
|------|-------|-----------|-----------------|
| Tech Lead / Architect | 1 | C++, Python, System Design, EDA, Claude | Architecture, API design, AI prompt engineering |
| Backend Engineer (Sr) | 1 | Python, FastAPI, Claude | REST API, bindings (AI generates boilerplate) |
| Frontend Engineer (Sr) | 2 | React, TypeScript, WebGL, Claude | All frontend (AI accelerates component development) |
| DevOps / Infrastructure | 1 | Docker, K8s, CI/CD | Containerization, deployment |
| QA / Test Engineer | 1 | Playwright, Python, Claude | Testing (AI generates test scaffolds) |

### Ideal Candidate Profiles

1. **EDA engineers with web experience** - Understand chip design workflows AND modern web tech
2. **OpenROAD contributors** - Already familiar with the codebase, especially `src/gui/` module
3. **Scientific visualization developers** - Experience with large-dataset rendering in browsers (WebGL, Canvas)
4. **FastAPI/async Python developers** - Can build high-performance API layers over C++ engines
5. **React developers with Canvas/WebGL** - Can build custom renderers for chip layout visualization
6. **Full-stack developers experienced with Claude/AI tools** - Can leverage AI for 2-3x individual output

---

## 8. Detailed Gantt Chart — Both Scenarios

### Phase 1: Foundation

| Task | Classic | AI-Assisted |
|------|---------|-------------|
| Architecture & API Design | M1-M1.5 (1.5 mo) | M1-M1 (1.0 mo) |
| FastAPI Backend Scaffold | M1-M2.5 (1.5 mo) | M0.8-M1.6 (0.8 mo) |
| React Frontend Scaffold | M1-M2.5 (1.5 mo) | M0.8-M1.6 (0.8 mo) |
| Layout Data API | M2-M3 (1.0 mo) | M1.5-M2.2 (0.7 mo) |
| **Milestone: API Prototype** | **Month 3** | **Month 2** |

### Phase 2: MVP

| Task | Classic | AI-Assisted |
|------|---------|-------------|
| WebGL Layout Viewer | M3-M5.5 (2.5 mo) | M2-M3.5 (1.5 mo) |
| Timing Analysis Widget | M4-M6 (2.0 mo) | M2.5-M3.8 (1.3 mo) |
| DRC Violation Viewer | M5-M6.5 (1.5 mo) | M3.5-M4.5 (1.0 mo) |
| Web TCL Terminal | M5.5-M6.5 (1.0 mo) | M3.5-M4.2 (0.7 mo) |
| Display Controls | M6-M7 (1.0 mo) | M4-M4.7 (0.7 mo) |
| **Milestone: MVP Release** | **Month 7** | **Month 5** |

### Phase 3: Feature Parity

| Task | Classic | AI-Assisted |
|------|---------|-------------|
| Clock Tree Viewer | M7-M8.5 (1.5 mo) | M5-M6 (1.0 mo) |
| Inspector / Property Browser | M7.5-M9.5 (2.0 mo) | M5.3-M6.6 (1.3 mo) |
| Heat Maps & Density Viz | M8.5-M10 (1.5 mo) | M5.8-M6.8 (1.0 mo) |
| Charts & Analysis Plots | M9-M10.5 (1.5 mo) | M6.3-M7.3 (1.0 mo) |
| Search / Find / Goto | M9.5-M10.5 (1.0 mo) | M6.8-M7.5 (0.7 mo) |
| Image Export | M10-M11 (1.0 mo) | M7-M7.5 (0.5 mo) |
| **Milestone: Feature Parity** | **Month 11** | **Month 7.5** |

### Phase 4: Testing & Hardening

| Task | Classic | AI-Assisted |
|------|---------|-------------|
| E2E Test Suite (Playwright) | M11-M12.5 (1.5 mo) | M7.5-M8.5 (1.0 mo) |
| Visual Regression Testing | M11.5-M12.5 (1.0 mo) | M7.8-M8.5 (0.7 mo) |
| Performance Benchmarking | M12-M13 (1.0 mo) | M8.2-M9.0 (0.8 mo) |
| Accessibility Audit | M12.5-M13 (0.5 mo) | M8.5-M8.9 (0.4 mo) |
| Security Audit (OWASP) | M13-M14 (1.0 mo) | M8.8-M9.5 (0.7 mo) |
| **Milestone: Production Ready** | **Month 14** | **Month 9.5** |

### Phase 5: Deployment & Migration

| Task | Classic | AI-Assisted |
|------|---------|-------------|
| Docker Image (Web Default) | M14-M15 (1.0 mo) | M9.5-M10.2 (0.7 mo) |
| Documentation & Migration Guide | M14-M15.5 (1.5 mo) | M9.5-M10.5 (1.0 mo) |
| User Training & Feedback | M15-M16 (1.0 mo) | M10-M11 (1.0 mo) |
| Parallel Operation (Qt + Web) | M15-M17 (2.0 mo) | M10-M11.5 (1.5 mo) |
| **Milestone: General Availability** | **Month 17** | **Month 11.5** |

### Phase 6: Qt Retirement

| Task | Classic | AI-Assisted |
|------|---------|-------------|
| Deprecation Announcement | M17-M17.5 (0.5 mo) | M11.5-M11.8 (0.3 mo) |
| Final Qt Bug Fixes | M17-M18.5 (1.5 mo) | M11.5-M12.5 (1.0 mo) |
| Remove Qt from Default Build | M18-M19 (1.0 mo) | M12-M12.7 (0.7 mo) |
| Archive Qt GUI Code | M19-M19.5 (0.5 mo) | M12.5-M12.8 (0.3 mo) |
| **Milestone: Qt GUI Retired** | **Month 20** | **Month 13** |

---

## 9. Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|-----------|-----------|
| WebGL performance for large designs | High | Medium | Progressive rendering, LOD, server-side tile generation |
| Loss of Qt-specific features | Medium | Low | Feature parity checklist, user acceptance testing |
| Team skill gap (EDA + Web) | Medium | Medium | Hire/train hybrid developers, pair programming |
| OpenROAD Python binding limitations | High | Low | Extend bindings as needed, fallback to TCL bridge |
| User adoption resistance | Medium | Medium | Parallel operation period, gather feedback early |
| AI productivity overestimation | Medium | Medium | Conservative 35% estimate; METR study shows risk for experienced devs |
| AI-generated code quality | Medium | Low | Mandatory code review; 17% mastery concern from Anthropic study |

---

## 10. License Comparison

| Component | Current (Qt) | Proposed (Web) |
|-----------|-------------|---------------|
| GUI Framework | Qt5/6 (LGPL/GPL) | React (MIT) |
| Charts | Qt Charts (GPL -> LGPL) | Plotly.js (MIT) |
| Rendering | Qt OpenGL (LGPL) | WebGL (Browser-native) |
| Backend | N/A (embedded C++) | FastAPI (MIT) |
| Build Impact | Heavy (~30min Qt build) | Separate, fast builds |
| **Overall** | **LGPL/GPL compliance needed** | **All MIT/BSD - fully permissive** |

---

## 11. References

### Research Papers and Studies

1. Anthropic, "How AI Is Transforming Work at Anthropic" (2025)
2. Anthropic, "Estimating AI Productivity Gains from Claude Conversations" (2025)
3. Anthropic, "How AI Assistance Impacts the Formation of Coding Skills" (2025)
4. Peng et al., "The Impact of AI on Developer Productivity: Evidence from GitHub Copilot" (arXiv, 2023)
5. METR, "Measuring the Impact of Early-2025 AI on Experienced OS Developer Productivity" (2025)
6. McKinsey, "Unlocking the Value of AI in Software Development" (2024)
7. BCG/Harvard, "Generative AI and Knowledge Workers" (2024)
8. Systematic Literature Review, "The Impact of LLM-Assistants on Developer Productivity" (arXiv, 2025)

### Project References

9. Planorama Design, "Transforming Semiconductor EDA into an Enterprise SaaS"
10. qBittorrent Alternate WebUIs Wiki (GitHub)
11. OpenROAD GUI Documentation (readthedocs)
12. FastAPI Full-Stack Template (GitHub)
13. UCSC OSPO / OpenROAD 2025

### GitHub Issues

14. OpenROAD Issue #1055: Do not initialize GUI unless required
15. OpenROAD Issue #2247: qt5-default dependency on Ubuntu 22.04
16. OpenROAD-flow-scripts Issue #3163: Build headless minimal dependency klayout
17. OpenLane Issue #642: Enable the OR GUI

---

*Document generated: 2026-03-12*
*Project: OpenROAD Qt-to-Web Migration*
*Charts: See gantt_chart.png, comparison_chart.png, resource_chart.png*
