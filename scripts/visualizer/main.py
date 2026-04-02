"""CLI entrypoint for visualizer pipeline."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Dict, List

import pandas as pd

from preprocess import export_summary_tables, load_and_prepare
from plotters import generate_plots


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Generate static benchmark visualization package.")
    parser.add_argument("--input-root", type=Path, default=Path("data/raw"))
    parser.add_argument("--config-root", type=Path, default=Path("config"))
    parser.add_argument("--out-root", type=Path, default=Path("data/figures"))
    parser.add_argument(
        "--export-summary",
        type=Path,
        default=Path("data/processed/summary"),
        help="Directory for summary CSV outputs.",
    )
    return parser


def _markdown_table(df: pd.DataFrame) -> str:
    if df.empty:
        return "(empty)"
    cols = list(df.columns)
    header = "| " + " | ".join(cols) + " |"
    sep = "| " + " | ".join(["---"] * len(cols)) + " |"
    rows = []
    for _, row in df.iterrows():
        rows.append("| " + " | ".join(str(row[c]) for c in cols) + " |")
    return "\n".join([header, sep] + rows)


def _generate_report(prepared: Dict[str, object], report_path: Path, generated_figs: List[Path]) -> None:
    matrix = prepared["matrix"]
    sum_df = prepared["sum"]
    best = prepared["best_config_by_size"]
    integrity = prepared["integrity"]

    matrix_best_speedup = matrix["speedup_vs_baseline"].max() if not matrix.empty else float("nan")
    sum_best_speedup = sum_df["speedup_vs_baseline"].max() if not sum_df.empty else float("nan")

    slowdown_rows = sum_df[
        sum_df["algorithm"].str.contains("superscalar", na=False)
        & (sum_df["speedup_vs_baseline"] < 1.0)
    ][["problem_size", "variant_param", "speedup_vs_baseline", "case_id"]].sort_values(
        ["problem_size", "variant_param"]
    )

    lines = [
        "# Visualizer Report",
        "",
        "## Key Conclusions",
        f"- matrix_dot max speedup_vs_baseline: **{matrix_best_speedup:.4f}**",
        f"- sum_reduce max speedup_vs_baseline: **{sum_best_speedup:.4f}**",
        "- 双口径指标已导出：absolute ns_per_run + normalized ns_per_element.",
        "",
        "## Best Config By Size",
        _markdown_table(best),
        "",
        "## 异常区间（superscalar slower than naive）",
        _markdown_table(slowdown_rows),
        "",
        "## Data Integrity",
    ]
    for item in integrity:
        lines.append(
            f"- {item['domain']}: rows={item['rows']}, algorithms={item['algorithms']}, "
            f"problem_sizes={item['problem_sizes']}, config_unmatched_rows={item['config_unmatched_rows']}"
        )

    lines.extend(["", "## Generated Figures"])
    for p in generated_figs:
        lines.append(f"- {p.as_posix()}")

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = _build_parser().parse_args()

    prepared = load_and_prepare(args.input_root, args.config_root)
    export_summary_tables(prepared, args.export_summary)
    figure_paths = generate_plots(prepared, args.out_root)
    _generate_report(prepared, args.export_summary / "report.md", figure_paths)

    print("Visualizer completed.")
    print(f"Summary exported to: {args.export_summary}")
    print(f"Figures exported to: {args.out_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
