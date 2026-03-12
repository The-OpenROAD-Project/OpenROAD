#!/usr/bin/env python3
"""
OpenROAD Qt-to-Web Migration - Gantt Chart Generator

Generates professional Gantt charts comparing two scenarios:
  1. Classic Development (20 months, 9 FTE)
  2. AI-Assisted Development with Claude (13 months, 6 FTE)

Requirements: pip install matplotlib
"""

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch
from datetime import datetime, timedelta
import numpy as np


# Phase colors shared across charts
PHASE_COLORS = {
    "Foundation": "#4A90D9",
    "MVP": "#50C878",
    "Feature Parity": "#F5A623",
    "Testing": "#E74C3C",
    "Deployment": "#9B59B6",
    "Qt Retirement": "#95A5A6",
}


def draw_gantt(ax, tasks, start_date, total_months, title, subtitle, team_text):
    """Draw a single Gantt chart on the given axes."""
    bar_height = 0.55

    for i, (name, phase, start_m, duration, is_milestone) in enumerate(tasks):
        y_pos = len(tasks) - 1 - i
        color = PHASE_COLORS[phase]

        if is_milestone:
            ax.plot(start_m, y_pos, marker='D', markersize=12, color=color,
                    markeredgecolor='white', markeredgewidth=2, zorder=5)
        else:
            bar = FancyBboxPatch(
                (start_m, y_pos - bar_height / 2),
                duration, bar_height,
                boxstyle="round,pad=0.05",
                facecolor=color, edgecolor='white',
                linewidth=1.5, alpha=0.85, zorder=3
            )
            ax.add_patch(bar)
            if duration >= 0.8:
                ax.text(start_m + duration / 2, y_pos, f"{duration:.1f}m",
                        ha='center', va='center', fontsize=6.5,
                        color='white', fontweight='bold', zorder=4)

    # Task labels
    for i, (name, phase, start_m, duration, is_milestone) in enumerate(tasks):
        y_pos = len(tasks) - 1 - i
        prefix = "* " if is_milestone else "  "
        fontweight = 'bold' if is_milestone else 'normal'
        ax.text(-0.3, y_pos, f"{prefix}{name}",
                ha='right', va='center', fontsize=7.5,
                fontweight=fontweight, color='#333333')

    # Phase backgrounds
    phase_ranges = {}
    for i, (name, phase, *_) in enumerate(tasks):
        y_pos = len(tasks) - 1 - i
        if phase not in phase_ranges:
            phase_ranges[phase] = [y_pos, y_pos]
        phase_ranges[phase][0] = min(phase_ranges[phase][0], y_pos)
        phase_ranges[phase][1] = max(phase_ranges[phase][1], y_pos)

    for phase, (y_min, y_max) in phase_ranges.items():
        ax.axhspan(y_min - 0.42, y_max + 0.42, alpha=0.06,
                    color=PHASE_COLORS[phase], zorder=1)
        if y_min > 0:
            ax.axhline(y=y_min - 0.5, color='#DDDDDD', linewidth=0.5, zorder=2)

    # Month grid
    month_names = []
    for m in range(total_months + 1):
        date = start_date + timedelta(days=m * 30.44)
        month_names.append(date.strftime('%b\n%Y'))
        ax.axvline(x=m, color='#E0E0E0', linewidth=0.5, zorder=1)

    ax.set_xticks(range(total_months + 1))
    ax.set_xticklabels(month_names, fontsize=6.5, color='#666666')

    # Quarter markers
    for q in range(0, total_months + 1, 3):
        ax.axvline(x=q, color='#BBBBBB', linewidth=1, zorder=2)

    ax.set_xlim(-0.5, total_months + 0.5)
    ax.set_ylim(-1.5, len(tasks))
    ax.set_yticks([])

    ax.set_title(f'{title}\n{subtitle}', fontsize=13, fontweight='bold',
                 color='#2C3E50', pad=12)

    # Team info box
    props = dict(boxstyle='round,pad=0.4', facecolor='#ECF0F1', alpha=0.9, edgecolor='#BDC3C7')
    ax.text(0.5, -1.0, team_text, fontsize=7.5, color='#2C3E50',
            ha='center', va='top', bbox=props)

    ax.annotate(f'Total: {total_months} months', xy=(total_months, -1.0),
                fontsize=9, fontweight='bold', color='#E74C3C', ha='right')


def create_dual_gantt(output_file="gantt_chart.png"):
    """Create side-by-side Gantt charts for classic vs AI-assisted development."""

    # ============================================================
    # SCENARIO 1: Classic Development (20 months, 9 FTE)
    # ============================================================
    classic_tasks = [
        # Phase 1: Foundation (Months 1-3)
        ("Architecture & API Design", "Foundation", 0, 1.5, False),
        ("FastAPI Backend Scaffold", "Foundation", 1, 1.5, False),
        ("React Frontend Scaffold", "Foundation", 1, 1.5, False),
        ("Layout Data API", "Foundation", 2, 1, False),
        ("API Prototype Demo", "Foundation", 3, 0, True),
        # Phase 2: MVP (Months 4-7)
        ("WebGL Layout Viewer", "MVP", 3, 2.5, False),
        ("Timing Analysis Widget", "MVP", 4, 2, False),
        ("DRC Violation Viewer", "MVP", 5, 1.5, False),
        ("Web TCL Terminal", "MVP", 5.5, 1, False),
        ("Display Controls", "MVP", 6, 1, False),
        ("MVP Release (Internal)", "MVP", 7, 0, True),
        # Phase 3: Feature Parity (Months 8-11)
        ("Clock Tree Viewer", "Feature Parity", 7, 1.5, False),
        ("Inspector / Property Browser", "Feature Parity", 7.5, 2, False),
        ("Heat Maps & Density Viz", "Feature Parity", 8.5, 1.5, False),
        ("Charts & Analysis Plots", "Feature Parity", 9, 1.5, False),
        ("Search / Find / Goto", "Feature Parity", 9.5, 1, False),
        ("Image Export", "Feature Parity", 10, 1, False),
        ("Feature Parity (Beta)", "Feature Parity", 11, 0, True),
        # Phase 4: Testing (Months 12-14)
        ("E2E Test Suite (Playwright)", "Testing", 11, 1.5, False),
        ("Visual Regression Testing", "Testing", 11.5, 1, False),
        ("Performance Benchmarking", "Testing", 12, 1, False),
        ("Accessibility Audit", "Testing", 12.5, 0.5, False),
        ("Security Audit (OWASP)", "Testing", 13, 1, False),
        ("Production-Ready Release", "Testing", 14, 0, True),
        # Phase 5: Deployment (Months 15-17)
        ("Docker Image (Web Default)", "Deployment", 14, 1, False),
        ("Documentation & Migration Guide", "Deployment", 14, 1.5, False),
        ("User Training & Feedback", "Deployment", 15, 1, False),
        ("Parallel Operation (Qt + Web)", "Deployment", 15, 2, False),
        ("General Availability", "Deployment", 17, 0, True),
        # Phase 6: Qt Retirement (Months 18-20)
        ("Deprecation Announcement", "Qt Retirement", 17, 0.5, False),
        ("Final Qt Bug Fixes", "Qt Retirement", 17, 1.5, False),
        ("Remove Qt from Default Build", "Qt Retirement", 18, 1, False),
        ("Archive Qt GUI Code", "Qt Retirement", 19, 0.5, False),
        ("Qt GUI Retired", "Qt Retirement", 20, 0, True),
    ]

    # ============================================================
    # SCENARIO 2: AI-Assisted with Claude (13 months, 6 FTE)
    # Applies ~35% acceleration to coding tasks (conservative
    # estimate from research: 25-56% for coding, ~8% end-to-end,
    # balanced at ~35% for a migration project with mixed tasks)
    # ============================================================
    ai_tasks = [
        # Phase 1: Foundation (Months 1-2) - AI helps with scaffolding, API design docs
        ("Architecture & API Design", "Foundation", 0, 1.0, False),
        ("FastAPI Backend Scaffold", "Foundation", 0.8, 0.8, False),
        ("React Frontend Scaffold", "Foundation", 0.8, 0.8, False),
        ("Layout Data API", "Foundation", 1.5, 0.7, False),
        ("API Prototype Demo", "Foundation", 2, 0, True),
        # Phase 2: MVP (Months 3-5) - AI generates component boilerplate, tests
        ("WebGL Layout Viewer", "MVP", 2, 1.5, False),
        ("Timing Analysis Widget", "MVP", 2.5, 1.3, False),
        ("DRC Violation Viewer", "MVP", 3.5, 1.0, False),
        ("Web TCL Terminal", "MVP", 3.5, 0.7, False),
        ("Display Controls", "MVP", 4, 0.7, False),
        ("MVP Release (Internal)", "MVP", 5, 0, True),
        # Phase 3: Feature Parity (Months 5-8) - AI accelerates widget dev significantly
        ("Clock Tree Viewer", "Feature Parity", 5, 1.0, False),
        ("Inspector / Property Browser", "Feature Parity", 5.3, 1.3, False),
        ("Heat Maps & Density Viz", "Feature Parity", 5.8, 1.0, False),
        ("Charts & Analysis Plots", "Feature Parity", 6.3, 1.0, False),
        ("Search / Find / Goto", "Feature Parity", 6.8, 0.7, False),
        ("Image Export", "Feature Parity", 7, 0.5, False),
        ("Feature Parity (Beta)", "Feature Parity", 7.5, 0, True),
        # Phase 4: Testing (Months 8-9.5) - AI generates test scaffolds, but review is human
        ("E2E Test Suite (Playwright)", "Testing", 7.5, 1.0, False),
        ("Visual Regression Testing", "Testing", 7.8, 0.7, False),
        ("Performance Benchmarking", "Testing", 8.2, 0.8, False),
        ("Accessibility Audit", "Testing", 8.5, 0.4, False),
        ("Security Audit (OWASP)", "Testing", 8.8, 0.7, False),
        ("Production-Ready Release", "Testing", 9.5, 0, True),
        # Phase 5: Deployment (Months 10-11.5) - AI helps with docs, less with human tasks
        ("Docker Image (Web Default)", "Deployment", 9.5, 0.7, False),
        ("Documentation & Migration Guide", "Deployment", 9.5, 1.0, False),
        ("User Training & Feedback", "Deployment", 10, 1.0, False),
        ("Parallel Operation (Qt + Web)", "Deployment", 10, 1.5, False),
        ("General Availability", "Deployment", 11.5, 0, True),
        # Phase 6: Qt Retirement (Months 12-13) - Mostly process, AI helps with code removal
        ("Deprecation Announcement", "Qt Retirement", 11.5, 0.3, False),
        ("Final Qt Bug Fixes", "Qt Retirement", 11.5, 1.0, False),
        ("Remove Qt from Default Build", "Qt Retirement", 12, 0.7, False),
        ("Archive Qt GUI Code", "Qt Retirement", 12.5, 0.3, False),
        ("Qt GUI Retired", "Qt Retirement", 13, 0, True),
    ]

    start_date = datetime(2026, 4, 1)

    # Create figure with two subplots
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(24, 24))
    fig.patch.set_facecolor('#FAFAFA')

    draw_gantt(ax1, classic_tasks, start_date, 20,
               "Scenario A: Classic Development",
               "20 months | 9 FTE | ~155 person-months",
               "Team: 9 FTE — Tech Lead: 1 | Backend: 2 | Frontend: 3 | DevOps: 1 | QA: 1 | UX+Docs: 1")

    draw_gantt(ax2, ai_tasks, start_date, 13,
               "Scenario B: AI-Assisted Development (Claude)",
               "13 months | 6 FTE | ~68 person-months  (56% cost reduction)",
               "Team: 6 FTE — Tech Lead: 1 | Backend: 1 | Frontend: 2 | DevOps: 1 | QA: 1 + Claude AI")

    # Shared legend
    legend_patches = [
        mpatches.Patch(color=color, label=phase, alpha=0.85)
        for phase, color in PHASE_COLORS.items()
    ]
    legend_patches.append(plt.Line2D([0], [0], marker='D', color='w',
                                      markerfacecolor='#555555', markersize=10,
                                      label='Milestone'))
    fig.legend(handles=legend_patches, loc='upper center',
               fontsize=10, framealpha=0.9, edgecolor='#CCCCCC',
               ncol=7, bbox_to_anchor=(0.5, 0.98))

    fig.suptitle('OpenROAD: Qt to Web Interface Migration — Two Scenarios',
                 fontsize=18, fontweight='bold', color='#1A1A2E', y=1.01)

    plt.tight_layout(rect=[0, 0, 1, 0.96])
    plt.savefig(output_file, dpi=200, bbox_inches='tight',
                facecolor=fig.get_facecolor(), edgecolor='none')
    plt.close()
    print(f"Dual Gantt chart saved to: {output_file}")


def create_comparison_chart(output_file="comparison_chart.png"):
    """Create a comparison bar chart of the two scenarios."""
    fig, axes = plt.subplots(1, 3, figsize=(18, 6))
    fig.patch.set_facecolor('#FAFAFA')

    scenarios = ["Classic\nDevelopment", "AI-Assisted\n(Claude)"]
    colors = ["#4A90D9", "#50C878"]

    # Chart 1: Timeline
    ax = axes[0]
    values = [20, 13]
    bars = ax.bar(scenarios, values, color=colors, alpha=0.85, edgecolor='white', width=0.5)
    for bar, val in zip(bars, values):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.3,
                f'{val} months', ha='center', fontsize=12, fontweight='bold')
    ax.set_ylabel('Months', fontsize=11)
    ax.set_title('Project Duration', fontsize=13, fontweight='bold', color='#2C3E50')
    ax.set_ylim(0, 25)
    ax.grid(axis='y', alpha=0.3)
    # Savings annotation
    ax.annotate('35% faster', xy=(1, 13), xytext=(1.3, 18),
                fontsize=11, color='#27AE60', fontweight='bold',
                arrowprops=dict(arrowstyle='->', color='#27AE60', lw=2))

    # Chart 2: Team Size
    ax = axes[1]
    values = [9, 6]
    bars = ax.bar(scenarios, values, color=colors, alpha=0.85, edgecolor='white', width=0.5)
    for bar, val in zip(bars, values):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.15,
                f'{val} FTE', ha='center', fontsize=12, fontweight='bold')
    ax.set_ylabel('Full-Time Equivalents', fontsize=11)
    ax.set_title('Team Size (Peak)', fontsize=13, fontweight='bold', color='#2C3E50')
    ax.set_ylim(0, 12)
    ax.grid(axis='y', alpha=0.3)
    ax.annotate('33% smaller', xy=(1, 6), xytext=(1.3, 9),
                fontsize=11, color='#27AE60', fontweight='bold',
                arrowprops=dict(arrowstyle='->', color='#27AE60', lw=2))

    # Chart 3: Total Cost (person-months)
    ax = axes[2]
    values = [155, 68]
    bars = ax.bar(scenarios, values, color=colors, alpha=0.85, edgecolor='white', width=0.5)
    for bar, val in zip(bars, values):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 1.5,
                f'{val} PM', ha='center', fontsize=12, fontweight='bold')
    ax.set_ylabel('Person-Months', fontsize=11)
    ax.set_title('Total Effort', fontsize=13, fontweight='bold', color='#2C3E50')
    ax.set_ylim(0, 185)
    ax.grid(axis='y', alpha=0.3)
    ax.annotate('56% reduction', xy=(1, 68), xytext=(1.3, 130),
                fontsize=11, color='#27AE60', fontweight='bold',
                arrowprops=dict(arrowstyle='->', color='#27AE60', lw=2))

    fig.suptitle('Classic vs AI-Assisted Development — Key Metrics',
                 fontsize=15, fontweight='bold', color='#2C3E50', y=1.02)
    plt.tight_layout()
    plt.savefig(output_file, dpi=200, bbox_inches='tight',
                facecolor=fig.get_facecolor())
    plt.close()
    print(f"Comparison chart saved to: {output_file}")


def create_resource_chart(output_file="resource_chart.png"):
    """Create resource allocation charts for both scenarios."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(18, 7))
    fig.patch.set_facecolor('#FAFAFA')

    roles = ["Tech Lead", "Backend", "Frontend", "DevOps", "QA", "UX/Docs"]
    colors = ["#2C3E50", "#4A90D9", "#50C878", "#9B59B6", "#E74C3C", "#F5A623"]

    # Classic scenario
    phases_classic = ["Found.\n(M1-3)", "MVP\n(M4-7)", "Feat.\n(M8-11)",
                      "Test\n(M12-14)", "Deploy\n(M15-17)", "Retire\n(M18-20)"]
    data_classic = np.array([
        [1.0, 1.0, 0.5, 0.5, 0.5, 0.25],
        [2.0, 2.0, 1.5, 1.0, 1.0, 0.5],
        [1.0, 3.0, 3.0, 1.0, 1.0, 0.5],
        [0.5, 0.5, 0.5, 1.0, 1.0, 0.5],
        [0.0, 0.5, 0.5, 1.0, 0.5, 0.25],
        [0.5, 0.5, 0.5, 0.25, 0.5, 0.25],
    ])

    # AI-assisted scenario
    phases_ai = ["Found.\n(M1-2)", "MVP\n(M3-5)", "Feat.\n(M5-8)",
                 "Test\n(M8-10)", "Deploy\n(M10-12)", "Retire\n(M12-13)"]
    data_ai = np.array([
        [1.0, 1.0, 0.5, 0.5, 0.5, 0.25],
        [1.0, 1.0, 1.0, 0.5, 0.5, 0.25],
        [1.0, 2.0, 2.0, 0.5, 0.5, 0.25],
        [0.5, 0.5, 0.5, 1.0, 1.0, 0.5],
        [0.0, 0.5, 0.5, 1.0, 0.5, 0.25],
        [0.5, 0.5, 0.5, 0.25, 0.5, 0.25],
    ])

    for ax, data, phases, title in [
        (ax1, data_classic, phases_classic, "Classic Development (9 FTE peak)"),
        (ax2, data_ai, phases_ai, "AI-Assisted / Claude (6 FTE peak)")
    ]:
        ax.set_facecolor('#FAFAFA')
        x = np.arange(len(phases))
        width = 0.12
        offsets = np.arange(len(roles)) - (len(roles) - 1) / 2

        for i, (role, color) in enumerate(zip(roles, colors)):
            bars = ax.bar(x + offsets[i] * width, data[i], width * 0.9,
                          label=role, color=color, alpha=0.85, edgecolor='white', linewidth=0.5)
            for bar, val in zip(bars, data[i]):
                if val > 0:
                    ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.02,
                            f'{val:.0f}' if val == int(val) else f'{val:.1f}',
                            ha='center', va='bottom', fontsize=6, color='#555')

        totals = data.sum(axis=0)
        ax.plot(x, totals, 'ko-', linewidth=2, markersize=7, label='Total FTE', zorder=5)
        for xi, total in zip(x, totals):
            ax.text(xi, total + 0.12, f'{total:.1f}', ha='center', fontsize=8,
                    fontweight='bold', color='#333')

        ax.set_ylabel('FTE', fontsize=10)
        ax.set_title(title, fontsize=11, fontweight='bold', color='#2C3E50')
        ax.set_xticks(x)
        ax.set_xticklabels(phases, fontsize=8)
        ax.legend(loc='upper right', fontsize=7, ncol=2, framealpha=0.9)
        ax.set_ylim(0, max(totals) + 1.5)
        ax.grid(axis='y', alpha=0.3)

    fig.suptitle('Resource Allocation Comparison', fontsize=14, fontweight='bold',
                 color='#2C3E50', y=1.02)
    plt.tight_layout()
    plt.savefig(output_file, dpi=200, bbox_inches='tight',
                facecolor=fig.get_facecolor())
    plt.close()
    print(f"Resource chart saved to: {output_file}")


if __name__ == "__main__":
    create_dual_gantt()
    create_comparison_chart()
    create_resource_chart()
    print("\nAll charts generated successfully!")
