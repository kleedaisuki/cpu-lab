"""Data loading and feature engineering for benchmark visualization."""

from __future__ import annotations

from dataclasses import dataclass, asdict
import re
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

import pandas as pd


RAW_REQUIRED_COLUMNS = {
    "benchmark_name",
    "algorithm",
    "problem_size",
    "best_ns_per_run",
    "mean_ns_per_run",
    "median_ns_per_run",
    "stddev_ns",
    "notes",
}

MATRIX_CONFIG_REQUIRED_COLUMNS = {
    "case_id",
    "algorithm",
    "rows",
    "cols",
    "cache_block_cols",
    "notes",
}

SUM_CONFIG_REQUIRED_COLUMNS = {
    "case_id",
    "algorithm",
    "length",
    "superscalar_lanes",
    "notes",
}


class DataValidationError(ValueError):
    """Raised when required input schema is missing."""


@dataclass
class ExperimentRecord:
    """Unified logical record used by visualization pipeline."""

    domain: str
    benchmark_name: str
    algorithm: str
    problem_size: int
    case_id: str
    variant_param: float | None
    best_ns_per_run: float
    mean_ns_per_run: float
    median_ns_per_run: float
    stddev_ns: float
    stability_cv: float
    ns_per_element: float
    speedup_vs_baseline: float
    notes: str

    def to_dict(self) -> Dict[str, object]:
        """Convert record to serializable dict."""
        return asdict(self)


def validate_required_columns(df: pd.DataFrame, required: Iterable[str], source: str) -> None:
    """Validate that required columns are present in DataFrame."""
    missing = sorted(set(required) - set(df.columns))
    if missing:
        raise DataValidationError(f"{source} missing required columns: {', '.join(missing)}")


def _read_csv(path: Path) -> pd.DataFrame:
    if not path.exists():
        raise FileNotFoundError(f"Missing input file: {path}")
    return pd.read_csv(path)


def _extract_case_id(notes: str) -> str:
    if not isinstance(notes, str) or not notes:
        return ""
    return notes.split(":", 1)[0].strip()


def _extract_from_notes(notes: str, domain: str) -> Tuple[float | None, str]:
    if not isinstance(notes, str):
        return None, "unknown"
    if domain == "matrix_dot":
        m = re.search(r"cache_tile_(\d+)", notes)
        if m:
            value = float(m.group(1))
            return value, f"cache_{int(value)}"
        if "baseline" in notes:
            return None, "naive"
    if domain == "sum_reduce":
        m = re.search(r"superscalar_l(\d+)", notes)
        if m:
            value = float(m.group(1))
            return value, f"lane_{int(value)}"
        if "baseline" in notes:
            return None, "naive"
    return None, "unknown"

def _build_variant_columns(df: pd.DataFrame, domain: str) -> pd.DataFrame:
    if domain == "matrix_dot":
        df["variant_param"] = df.get("cache_block_cols")
        df.loc[df["algorithm"].str.contains("naive", na=False), "variant_param"] = pd.NA
        df["variant_label"] = df["variant_param"].apply(
            lambda v: "naive" if pd.isna(v) else f"cache_{int(v)}"
        )
    else:
        df["variant_param"] = df.get("superscalar_lanes")
        df.loc[df["algorithm"].str.contains("naive", na=False), "variant_param"] = pd.NA
        df["variant_label"] = df["variant_param"].apply(
            lambda v: "naive" if pd.isna(v) else f"lane_{int(v)}"
        )
    return df


def _attach_baseline_speedup(df: pd.DataFrame) -> pd.DataFrame:
    baseline = (
        df[df["algorithm"].str.contains("naive", na=False)]
        .groupby("problem_size", as_index=False)["best_ns_per_run"]
        .min()
        .rename(columns={"best_ns_per_run": "baseline_best_ns_per_run"})
    )
    merged = df.merge(baseline, on="problem_size", how="left")
    merged["speedup_vs_baseline"] = (
        merged["baseline_best_ns_per_run"] / merged["best_ns_per_run"]
    )
    return merged


def _to_numeric(df: pd.DataFrame, cols: List[str]) -> pd.DataFrame:
    for col in cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")
    return df


def _prepare_domain(
    domain: str,
    raw_df: pd.DataFrame,
    cfg_df: pd.DataFrame,
    config_required: Iterable[str],
    join_cols: List[str],
) -> Tuple[pd.DataFrame, Dict[str, object]]:
    validate_required_columns(raw_df, RAW_REQUIRED_COLUMNS, f"{domain} raw")
    validate_required_columns(cfg_df, config_required, f"{domain} config")

    raw_df = raw_df.copy()
    cfg_df = cfg_df.copy()
    raw_df["domain"] = domain
    raw_df["case_id"] = raw_df["notes"].apply(_extract_case_id)

    cfg_selected = cfg_df[join_cols].copy()
    rename_map = {}
    if "algorithm" in cfg_selected.columns:
        rename_map["algorithm"] = "config_algorithm"
    if "notes" in cfg_selected.columns:
        rename_map["notes"] = "config_notes"
    cfg_selected = cfg_selected.rename(columns=rename_map)

    merged = raw_df.merge(cfg_selected, on="case_id", how="left", indicator=True)
    unmatched = int((merged["_merge"] == "left_only").sum())
    merged = merged.drop(columns=["_merge"])

    numeric_cols = [
        "problem_size",
        "best_ns_per_run",
        "mean_ns_per_run",
        "median_ns_per_run",
        "stddev_ns",
        "rows",
        "cols",
        "cache_block_cols",
        "length",
        "superscalar_lanes",
    ]
    merged = _to_numeric(merged, numeric_cols)

    for i in merged.index:
        if domain == "matrix_dot" and pd.isna(merged.at[i, "cache_block_cols"]):
            parsed_value, _ = _extract_from_notes(str(merged.at[i, "notes"]), domain)
            merged.at[i, "cache_block_cols"] = parsed_value
        if domain == "sum_reduce" and pd.isna(merged.at[i, "superscalar_lanes"]):
            parsed_value, _ = _extract_from_notes(str(merged.at[i, "notes"]), domain)
            merged.at[i, "superscalar_lanes"] = parsed_value

    merged = _build_variant_columns(merged, domain)
    merged["stability_cv"] = merged["stddev_ns"] / merged["mean_ns_per_run"]
    merged["ns_per_element"] = merged["best_ns_per_run"] / merged["problem_size"]
    merged["mean_ns_per_element"] = merged["mean_ns_per_run"] / merged["problem_size"]
    merged["median_ns_per_element"] = merged["median_ns_per_run"] / merged["problem_size"]
    merged = _attach_baseline_speedup(merged)

    summary = {
        "domain": domain,
        "rows": int(len(merged)),
        "problem_sizes": sorted(
            int(v) for v in merged["problem_size"].dropna().astype(int).unique().tolist()
        ),
        "algorithms": sorted(str(v) for v in merged["algorithm"].dropna().unique().tolist()),
        "config_unmatched_rows": unmatched,
    }
    return merged.sort_values(["problem_size", "algorithm"]).reset_index(drop=True), summary

def _best_config_table(all_domains: pd.DataFrame) -> pd.DataFrame:
    rows = []
    for (domain, problem_size), group in all_domains.groupby(["domain", "problem_size"]):
        best = group.sort_values("best_ns_per_run", ascending=True).iloc[0]
        rows.append(
            {
                "domain": domain,
                "problem_size": int(problem_size),
                "best_algorithm": best["algorithm"],
                "variant_param": (
                    None if pd.isna(best["variant_param"]) else float(best["variant_param"])
                ),
                "best_ns_per_run": float(best["best_ns_per_run"]),
                "speedup_vs_baseline": float(best["speedup_vs_baseline"]),
                "case_id": best.get("case_id", ""),
                "notes": best.get("notes", ""),
            }
        )
    return pd.DataFrame(rows).sort_values(["domain", "problem_size"]).reset_index(drop=True)


def _resolve_sum_raw_path(input_root: Path, sum_raw_file: str) -> Path:
    """解析 sum_reduce 输入文件路径，优先用户指定，缺失时回退默认文件 / Resolve sum_reduce input path with fallback."""
    preferred = input_root / "sum_reduce" / sum_raw_file
    if preferred.exists():
        return preferred
    fallback = input_root / "sum_reduce" / "benchmark_results.csv"
    if sum_raw_file != "benchmark_results.csv" and fallback.exists():
        return fallback
    return preferred

def load_and_prepare(
    input_root: Path,
    config_root: Path,
    sum_raw_file: str = "benchmark_results_after_fix.csv",
) -> Dict[str, object]:
    """加载原始/配置 CSV 并构造衍生指标 / Load raw/config CSVs and compute derived metrics."""
    matrix_raw = _read_csv(input_root / "matrix_dot" / "benchmark_results.csv")
    sum_raw = _read_csv(_resolve_sum_raw_path(input_root, sum_raw_file))
    matrix_cfg = _read_csv(config_root / "benchmark_matrix.csv")
    sum_cfg = _read_csv(config_root / "benchmark_sum.csv")

    matrix_df, matrix_summary = _prepare_domain(
        domain="matrix_dot",
        raw_df=matrix_raw,
        cfg_df=matrix_cfg,
        config_required=MATRIX_CONFIG_REQUIRED_COLUMNS,
        join_cols=[
            "case_id",
            "algorithm",
            "rows",
            "cols",
            "cache_block_cols",
            "notes",
        ],
    )
    sum_df, sum_summary = _prepare_domain(
        domain="sum_reduce",
        raw_df=sum_raw,
        cfg_df=sum_cfg,
        config_required=SUM_CONFIG_REQUIRED_COLUMNS,
        join_cols=[
            "case_id",
            "algorithm",
            "length",
            "superscalar_lanes",
            "notes",
        ],
    )

    combined = pd.concat([matrix_df, sum_df], ignore_index=True)
    best_table = _best_config_table(combined)

    records: List[ExperimentRecord] = []
    for _, row in combined.iterrows():
        records.append(
            ExperimentRecord(
                domain=str(row["domain"]),
                benchmark_name=str(row["benchmark_name"]),
                algorithm=str(row["algorithm"]),
                problem_size=int(row["problem_size"]),
                case_id=str(row.get("case_id", "")),
                variant_param=None if pd.isna(row["variant_param"]) else float(row["variant_param"]),
                best_ns_per_run=float(row["best_ns_per_run"]),
                mean_ns_per_run=float(row["mean_ns_per_run"]),
                median_ns_per_run=float(row["median_ns_per_run"]),
                stddev_ns=float(row["stddev_ns"]),
                stability_cv=float(row["stability_cv"]),
                ns_per_element=float(row["ns_per_element"]),
                speedup_vs_baseline=float(row["speedup_vs_baseline"]),
                notes=str(row.get("notes", "")),
            )
        )

    return {
        "matrix": matrix_df,
        "sum": sum_df,
        "combined": combined,
        "best_config_by_size": best_table,
        "integrity": [matrix_summary, sum_summary],
        "records": records,
    }

def export_summary_tables(prepared: Dict[str, object], export_root: Path) -> Dict[str, Path]:
    """Write processed summary tables to disk."""
    export_root.mkdir(parents=True, exist_ok=True)

    matrix_path = export_root / "summary_matrix_dot.csv"
    sum_path = export_root / "summary_sum_reduce.csv"
    best_path = export_root / "best_config_by_size.csv"

    prepared["matrix"].to_csv(matrix_path, index=False)
    prepared["sum"].to_csv(sum_path, index=False)
    prepared["best_config_by_size"].to_csv(best_path, index=False)

    return {
        "summary_matrix_dot.csv": matrix_path,
        "summary_sum_reduce.csv": sum_path,
        "best_config_by_size.csv": best_path,
    }

