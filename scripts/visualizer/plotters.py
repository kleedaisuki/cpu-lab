"""Plot generation for visualizer pipeline."""

from __future__ import annotations

from pathlib import Path
from typing import Dict, List

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd


FIG_MATRIX = [
    "matrix_time_curve",
    "matrix_normalized_curve",
    "matrix_speedup_heatmap",
    "matrix_stability_cv",
]

FIG_SUM = [
    "sum_time_curve",
    "sum_normalized_curve",
    "sum_speedup_heatmap",
    "sum_anomaly_slowdown",
]


def _save_fig(fig: plt.Figure, out_dir: Path, name: str) -> List[Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    png = out_dir / f"{name}.png"
    svg = out_dir / f"{name}.svg"
    fig.savefig(png, dpi=180, bbox_inches="tight")
    fig.savefig(svg, bbox_inches="tight")
    plt.close(fig)
    return [png, svg]


def _line_plot(df: pd.DataFrame, y_col: str, title: str, y_label: str) -> plt.Figure:
    fig, ax = plt.subplots(figsize=(10, 6))
    for label, g in df.groupby("variant_label"):
        gs = g.sort_values("problem_size")
        ax.plot(gs["problem_size"], gs[y_col], marker="o", linewidth=1.5, label=label)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Problem Size")
    ax.set_ylabel(y_label)
    ax.set_title(title)
    ax.grid(True, alpha=0.25)
    ax.legend(fontsize=8, ncol=2)
    return fig


def _heatmap_plot(
    df: pd.DataFrame,
    param_col: str,
    title: str,
    x_label: str,
    y_label: str,
) -> plt.Figure:
    filtered = df[df["algorithm"].str.contains("naive", na=False) == False].copy()
    fig, ax = plt.subplots(figsize=(10, 5))
    if filtered.empty:
        ax.text(0.5, 0.5, "No non-baseline rows", ha="center", va="center")
        ax.set_axis_off()
        return fig

    pivot = filtered.pivot_table(
        index=param_col,
        columns="problem_size",
        values="speedup_vs_baseline",
        aggfunc="mean",
    ).sort_index()

    im = ax.imshow(pivot.values, aspect="auto", cmap="coolwarm")
    ax.set_title(title)
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)
    ax.set_xticks(range(len(pivot.columns)))
    ax.set_xticklabels([str(int(v)) for v in pivot.columns], rotation=45, ha="right")
    ax.set_yticks(range(len(pivot.index)))
    ax.set_yticklabels([str(int(v)) for v in pivot.index])

    for i in range(pivot.shape[0]):
        for j in range(pivot.shape[1]):
            ax.text(j, i, f"{pivot.values[i, j]:.2f}", ha="center", va="center", fontsize=7)

    fig.colorbar(im, ax=ax, label="Speedup vs baseline")
    return fig


def _stability_plot(df: pd.DataFrame, title: str) -> plt.Figure:
    fig, ax = plt.subplots(figsize=(10, 6))
    for label, g in df.groupby("variant_label"):
        gs = g.sort_values("problem_size")
        ax.plot(gs["problem_size"], gs["stability_cv"], marker="o", linewidth=1.5, label=label)
    ax.set_xscale("log")
    ax.set_xlabel("Problem Size")
    ax.set_ylabel("CV (stddev/mean)")
    ax.set_title(title)
    ax.grid(True, alpha=0.25)
    ax.legend(fontsize=8, ncol=2)
    return fig


def _anomaly_plot(sum_df: pd.DataFrame) -> plt.Figure:
    fig, ax = plt.subplots(figsize=(10, 6))
    super_rows = sum_df[sum_df["algorithm"].str.contains("superscalar", na=False)].copy()
    slower = super_rows[super_rows["speedup_vs_baseline"] < 1.0]

    if slower.empty:
        ax.text(0.5, 0.5, "No slowdown anomalies (speedup < 1)", ha="center", va="center")
        ax.set_axis_off()
        return fig

    scatter = ax.scatter(
        slower["problem_size"],
        slower["speedup_vs_baseline"],
        c=slower["variant_param"].fillna(0),
        cmap="viridis",
        s=55,
        alpha=0.85,
        edgecolor="black",
        linewidths=0.25,
    )
    ax.axhline(1.0, color="red", linestyle="--", linewidth=1.0, label="baseline parity")
    ax.fill_between(
        slower["problem_size"].sort_values().unique(),
        0,
        1,
        color="red",
        alpha=0.08,
    )
    ax.set_xscale("log")
    ax.set_ylim(bottom=0)
    ax.set_xlabel("Problem Size")
    ax.set_ylabel("Speedup vs naive")
    ax.set_title("sum_reduce anomalies: superscalar slower than naive")
    ax.grid(True, alpha=0.25)
    ax.legend(loc="lower right")
    fig.colorbar(scatter, ax=ax, label="superscalar lanes")
    return fig


def generate_plots(prepared: Dict[str, object], out_root: Path) -> List[Path]:
    """Generate 8 figures and return written file paths."""
    matrix = prepared["matrix"]
    sum_df = prepared["sum"]

    outputs: List[Path] = []
    matrix_dir = out_root / "matrix_dot"
    sum_dir = out_root / "sum_reduce"

    outputs.extend(
        _save_fig(
            _line_plot(matrix, "best_ns_per_run", "matrix_dot time curve", "best ns per run"),
            matrix_dir,
            "matrix_time_curve",
        )
    )
    outputs.extend(
        _save_fig(
            _line_plot(
                matrix,
                "ns_per_element",
                "matrix_dot normalized curve",
                "best ns per element",
            ),
            matrix_dir,
            "matrix_normalized_curve",
        )
    )
    outputs.extend(
        _save_fig(
            _heatmap_plot(
                matrix,
                "cache_block_cols",
                "matrix_dot speedup heatmap",
                "problem_size",
                "cache_block_cols",
            ),
            matrix_dir,
            "matrix_speedup_heatmap",
        )
    )
    outputs.extend(
        _save_fig(
            _stability_plot(matrix, "matrix_dot stability (CV)"),
            matrix_dir,
            "matrix_stability_cv",
        )
    )

    outputs.extend(
        _save_fig(
            _line_plot(sum_df, "best_ns_per_run", "sum_reduce time curve", "best ns per run"),
            sum_dir,
            "sum_time_curve",
        )
    )
    outputs.extend(
        _save_fig(
            _line_plot(
                sum_df,
                "ns_per_element",
                "sum_reduce normalized curve",
                "best ns per element",
            ),
            sum_dir,
            "sum_normalized_curve",
        )
    )
    outputs.extend(
        _save_fig(
            _heatmap_plot(
                sum_df,
                "superscalar_lanes",
                "sum_reduce speedup heatmap",
                "problem_size",
                "superscalar lanes",
            ),
            sum_dir,
            "sum_speedup_heatmap",
        )
    )
    outputs.extend(
        _save_fig(
            _anomaly_plot(sum_df),
            sum_dir,
            "sum_anomaly_slowdown",
        )
    )

    return outputs
