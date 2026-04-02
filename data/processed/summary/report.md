# Visualizer Report

## Key Conclusions
- matrix_dot max speedup_vs_baseline: **1.9768**
- sum_reduce max speedup_vs_baseline: **1.0000**
- 双口径指标已导出：absolute ns_per_run + normalized ns_per_element.

## Best Config By Size
| domain | problem_size | best_algorithm | variant_param | best_ns_per_run | speedup_vs_baseline | case_id | notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| matrix_dot | 16384 | matrix_dot_cache | 128.0 | 79714.0625 | 1.0530999470764648 | m_128_cache_128 | m_128_cache_128:cache_tile_128_size_128; digest=13064476336938546383 |
| matrix_dot | 36864 | matrix_dot_cache | 128.0 | 182846.875 | 1.0138948231956384 | m_192_cache_128 | m_192_cache_128:cache_tile_128_size_192; digest=10614270075660950329 |
| matrix_dot | 65536 | matrix_dot_cache | 256.0 | 305612.5 | 1.0869565217391304 | m_256_cache_256 | m_256_cache_256:cache_tile_256_size_256; digest=12923839507462904676 |
| matrix_dot | 147456 | matrix_dot_cache | 256.0 | 668962.5 | 1.126987686155801 | m_384_cache_256 | m_384_cache_256:cache_tile_256_size_384; digest=14173234167593883340 |
| matrix_dot | 262144 | matrix_dot_cache | 256.0 | 1279634.375 | 1.2747183643765432 | m_512_cache_256 | m_512_cache_256:cache_tile_256_size_512; digest=4939301634244250759 |
| matrix_dot | 589824 | matrix_dot_cache | 256.0 | 2862937.5 | 1.2582138101163578 | m_768_cache_256 | m_768_cache_256:cache_tile_256_size_768; digest=17702062449183340215 |
| matrix_dot | 1048576 | matrix_dot_cache | 256.0 | 4981025.0 | 1.4547153321856445 | m_1024_cache_256 | m_1024_cache_256:cache_tile_256_size_1024; digest=1390044789617888298 |
| matrix_dot | 2359296 | matrix_dot_cache | 256.0 | 11361390.625 | 1.9767568065638972 | m_1536_cache_256 | m_1536_cache_256:cache_tile_256_size_1536; digest=10901834921722660545 |
| sum_reduce | 1024 | sum_naive | nan | 1778.125 | 1.0 | s_1024_naive | s_1024_naive:baseline_len_1024; digest=2512043020710393901 |
| sum_reduce | 2048 | sum_naive | nan | 3275.0 | 1.0 | s_2048_naive | s_2048_naive:baseline_len_2048; digest=3991814586103565139 |
| sum_reduce | 4096 | sum_naive | nan | 7312.5 | 1.0 | s_4096_naive | s_4096_naive:baseline_len_4096; digest=4265867278912980020 |
| sum_reduce | 8192 | sum_naive | nan | 14718.75 | 1.0 | s_8192_naive | s_8192_naive:baseline_len_8192; digest=8285804849025817662 |
| sum_reduce | 16384 | sum_naive | nan | 26543.75 | 1.0 | s_16384_naive | s_16384_naive:baseline_len_16384; digest=4948716549764992656 |
| sum_reduce | 32768 | sum_naive | nan | 53489.0625 | 1.0 | s_32768_naive | s_32768_naive:baseline_len_32768; digest=6445697804731218407 |
| sum_reduce | 65536 | sum_naive | nan | 110368.75 | 1.0 | s_65536_naive | s_65536_naive:baseline_len_65536; digest=4889642161319717707 |
| sum_reduce | 131072 | sum_naive | nan | 213843.75 | 1.0 | s_131072_naive | s_131072_naive:baseline_len_131072; digest=6403142234192446800 |
| sum_reduce | 262144 | sum_naive | nan | 436812.5 | 1.0 | s_262144_naive | s_262144_naive:baseline_len_262144; digest=7808418140491186077 |
| sum_reduce | 524288 | sum_naive | nan | 879568.75 | 1.0 | s_524288_naive | s_524288_naive:baseline_len_524288; digest=7275744020720224056 |

## 异常区间（superscalar slower than naive）
| problem_size | variant_param | speedup_vs_baseline | case_id |
| --- | --- | --- | --- |
| 1024 | 2.0 | 0.5048802129547472 | s_1024_super_2 |
| 1024 | 4.0 | 0.5273401297497683 | s_1024_super_4 |
| 1024 | 8.0 | 0.41761467889908255 | s_1024_super_8 |
| 1024 | 16.0 | 0.4773489932885906 | s_1024_super_16 |
| 2048 | 2.0 | 0.4782112708190737 | s_2048_super_2 |
| 2048 | 4.0 | 0.5072604065827686 | s_2048_super_4 |
| 2048 | 8.0 | 0.2896627971254837 | s_2048_super_8 |
| 2048 | 16.0 | 0.39825194755842674 | s_2048_super_16 |
| 4096 | 2.0 | 0.5077023215448037 | s_4096_super_2 |
| 4096 | 4.0 | 0.5957230142566191 | s_4096_super_4 |
| 4096 | 8.0 | 0.47619047619047616 | s_4096_super_8 |
| 4096 | 16.0 | 0.4956576996399068 | s_4096_super_16 |
| 8192 | 2.0 | 0.5192943770672547 | s_8192_super_2 |
| 8192 | 4.0 | 0.5126530612244898 | s_8192_super_4 |
| 8192 | 8.0 | 0.4085704371963914 | s_8192_super_8 |
| 8192 | 16.0 | 0.5375485049075553 | s_8192_super_16 |
| 16384 | 2.0 | 0.4577988573892422 | s_16384_super_2 |
| 16384 | 4.0 | 0.5341970378289991 | s_16384_super_4 |
| 16384 | 8.0 | 0.36408058294042006 | s_16384_super_8 |
| 16384 | 16.0 | 0.5141335270262091 | s_16384_super_16 |
| 32768 | 2.0 | 0.4446537122668468 | s_32768_super_2 |
| 32768 | 4.0 | 0.5351582041020511 | s_32768_super_4 |
| 32768 | 8.0 | 0.3837393088141331 | s_32768_super_8 |
| 32768 | 16.0 | 0.45806460245671315 | s_32768_super_16 |
| 65536 | 2.0 | 0.488519420161558 | s_65536_super_2 |
| 65536 | 4.0 | 0.5537125297880346 | s_65536_super_4 |
| 65536 | 8.0 | 0.4058607216731786 | s_65536_super_8 |
| 65536 | 16.0 | 0.4706776035662644 | s_65536_super_16 |
| 131072 | 2.0 | 0.4303489392209948 | s_131072_super_2 |
| 131072 | 4.0 | 0.48336682689421095 | s_131072_super_4 |
| 131072 | 8.0 | 0.3960665377892507 | s_131072_super_8 |
| 131072 | 16.0 | 0.4806017530059557 | s_131072_super_16 |
| 262144 | 2.0 | 0.44229418353721434 | s_262144_super_2 |
| 262144 | 4.0 | 0.48662545888766845 | s_262144_super_4 |
| 262144 | 8.0 | 0.39187101730030083 | s_262144_super_8 |
| 262144 | 16.0 | 0.4510305328116807 | s_262144_super_16 |
| 524288 | 2.0 | 0.4451639025346927 | s_524288_super_2 |
| 524288 | 4.0 | 0.48665957759507567 | s_524288_super_4 |
| 524288 | 8.0 | 0.3868877516656289 | s_524288_super_8 |
| 524288 | 16.0 | 0.4605976304248216 | s_524288_super_16 |

## Data Integrity
- matrix_dot: rows=46, algorithms=['matrix_dot_cache', 'matrix_dot_naive'], problem_sizes=[16384, 36864, 65536, 147456, 262144, 589824, 1048576, 2359296], config_unmatched_rows=0
- sum_reduce: rows=50, algorithms=['sum_naive', 'sum_superscalar'], problem_sizes=[1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288], config_unmatched_rows=0

## Generated Figures
- data/figures/matrix_dot/matrix_time_curve.png
- data/figures/matrix_dot/matrix_time_curve.svg
- data/figures/matrix_dot/matrix_normalized_curve.png
- data/figures/matrix_dot/matrix_normalized_curve.svg
- data/figures/matrix_dot/matrix_speedup_heatmap.png
- data/figures/matrix_dot/matrix_speedup_heatmap.svg
- data/figures/matrix_dot/matrix_stability_cv.png
- data/figures/matrix_dot/matrix_stability_cv.svg
- data/figures/sum_reduce/sum_time_curve.png
- data/figures/sum_reduce/sum_time_curve.svg
- data/figures/sum_reduce/sum_normalized_curve.png
- data/figures/sum_reduce/sum_normalized_curve.svg
- data/figures/sum_reduce/sum_speedup_heatmap.png
- data/figures/sum_reduce/sum_speedup_heatmap.svg
- data/figures/sum_reduce/sum_anomaly_slowdown.png
- data/figures/sum_reduce/sum_anomaly_slowdown.svg
