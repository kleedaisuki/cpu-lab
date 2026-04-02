import tempfile
import unittest
from pathlib import Path
import sys

import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts" / "visualizer"))

from preprocess import DataValidationError, load_and_prepare, validate_required_columns
from plotters import generate_plots


class VisualizerUnitTests(unittest.TestCase):
    def test_missing_required_columns_raise(self) -> None:
        df = pd.DataFrame({"algorithm": ["x"]})
        with self.assertRaises(DataValidationError) as ctx:
            validate_required_columns(df, {"algorithm", "problem_size"}, "unit raw")
        self.assertIn("problem_size", str(ctx.exception))
        self.assertIn("unit raw", str(ctx.exception))

    def test_link_and_metrics(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            input_root = root / "raw"
            cfg_root = root / "config"
            (input_root / "matrix_dot").mkdir(parents=True)
            (input_root / "sum_reduce").mkdir(parents=True)
            cfg_root.mkdir(parents=True)

            matrix_raw = pd.DataFrame(
                [
                    {
                        "benchmark_name": "matrix_dot",
                        "algorithm": "matrix_dot_naive",
                        "problem_size": 16,
                        "runs_per_sample": 1,
                        "sample_count": 1,
                        "best_ns_per_run": 16.0,
                        "mean_ns_per_run": 16.0,
                        "median_ns_per_run": 16.0,
                        "stddev_ns": 0.8,
                        "fit_slope_ns_per_run": 16.0,
                        "fit_intercept_ns": 0.0,
                        "fit_r_squared": 1.0,
                        "notes": "m_naive:baseline",
                    },
                    {
                        "benchmark_name": "matrix_dot",
                        "algorithm": "matrix_dot_cache",
                        "problem_size": 16,
                        "runs_per_sample": 1,
                        "sample_count": 1,
                        "best_ns_per_run": 8.0,
                        "mean_ns_per_run": 8.0,
                        "median_ns_per_run": 8.0,
                        "stddev_ns": 0.4,
                        "fit_slope_ns_per_run": 8.0,
                        "fit_intercept_ns": 0.0,
                        "fit_r_squared": 1.0,
                        "notes": "m_cache:cache_tile_4",
                    },
                ]
            )
            matrix_cfg = pd.DataFrame(
                [
                    {
                        "case_id": "m_naive",
                        "algorithm": "naive",
                        "rows": 4,
                        "cols": 4,
                        "cache_block_cols": 4,
                        "notes": "baseline",
                    },
                    {
                        "case_id": "m_cache",
                        "algorithm": "cache",
                        "rows": 4,
                        "cols": 4,
                        "cache_block_cols": 4,
                        "notes": "cache_tile_4",
                    },
                ]
            )

            sum_raw = pd.DataFrame(
                [
                    {
                        "benchmark_name": "sum_reduce",
                        "algorithm": "sum_naive",
                        "problem_size": 8,
                        "runs_per_sample": 1,
                        "sample_count": 1,
                        "best_ns_per_run": 8.0,
                        "mean_ns_per_run": 8.0,
                        "median_ns_per_run": 8.0,
                        "stddev_ns": 0.8,
                        "fit_slope_ns_per_run": 8.0,
                        "fit_intercept_ns": 0.0,
                        "fit_r_squared": 1.0,
                        "notes": "s_naive:baseline",
                    },
                    {
                        "benchmark_name": "sum_reduce",
                        "algorithm": "sum_superscalar",
                        "problem_size": 8,
                        "runs_per_sample": 1,
                        "sample_count": 1,
                        "best_ns_per_run": 16.0,
                        "mean_ns_per_run": 16.0,
                        "median_ns_per_run": 16.0,
                        "stddev_ns": 1.6,
                        "fit_slope_ns_per_run": 16.0,
                        "fit_intercept_ns": 0.0,
                        "fit_r_squared": 1.0,
                        "notes": "s_super:superscalar_l2",
                    },
                ]
            )
            sum_cfg = pd.DataFrame(
                [
                    {
                        "case_id": "s_naive",
                        "algorithm": "naive",
                        "length": 8,
                        "value_base": 1.0,
                        "value_step": 0.1,
                        "superscalar_lanes": 4,
                        "notes": "baseline",
                    },
                    {
                        "case_id": "s_super",
                        "algorithm": "superscalar",
                        "length": 8,
                        "value_base": 1.0,
                        "value_step": 0.1,
                        "superscalar_lanes": 2,
                        "notes": "superscalar_l2",
                    },
                ]
            )

            matrix_raw.to_csv(input_root / "matrix_dot" / "benchmark_results.csv", index=False)
            sum_raw.to_csv(input_root / "sum_reduce" / "benchmark_results.csv", index=False)
            matrix_cfg.to_csv(cfg_root / "benchmark_matrix.csv", index=False)
            sum_cfg.to_csv(cfg_root / "benchmark_sum.csv", index=False)

            prepared = load_and_prepare(input_root, cfg_root)
            matrix_df = prepared["matrix"]
            cache_row = matrix_df[matrix_df["algorithm"] == "matrix_dot_cache"].iloc[0]
            self.assertEqual(cache_row["case_id"], "m_cache")
            self.assertAlmostEqual(cache_row["speedup_vs_baseline"], 2.0, places=6)
            self.assertAlmostEqual(cache_row["ns_per_element"], 0.5, places=6)
            self.assertAlmostEqual(cache_row["stability_cv"], 0.05, places=6)


class VisualizerIntegrationTests(unittest.TestCase):
    def test_end_to_end_generate_figure_files(self) -> None:
        input_root = ROOT / "data" / "raw"
        config_root = ROOT / "config"
        if not (input_root.exists() and config_root.exists()):
            self.skipTest("real data not available")

        prepared = load_and_prepare(input_root, config_root)
        with tempfile.TemporaryDirectory() as td:
            out_root = Path(td) / "figures"
            generated = generate_plots(prepared, out_root)

            png_files = [p for p in generated if p.suffix == ".png"]
            svg_files = [p for p in generated if p.suffix == ".svg"]
            self.assertEqual(len(png_files), 8)
            self.assertEqual(len(svg_files), 8)
            for p in generated:
                self.assertTrue(p.exists(), f"missing figure: {p}")


if __name__ == "__main__":
    unittest.main()
