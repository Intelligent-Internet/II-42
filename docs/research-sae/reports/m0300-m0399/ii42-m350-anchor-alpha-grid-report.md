# ii42 M350 Anchor Alpha Grid Report

## Goal

M349 showed that a small post-hoc blend with the BM25/atom anchor improves the
current best M346A broad8 scorer. The best tested row was `alpha=0.20`, with a
small but clean heldout gain on all four aggregate metrics.

M350 checks whether that signal is stable or just a coarse-grid accident by
running a finer alpha sweep around the M349 winner.

## Contract

- Base runner: `scripts/run_m344_explicit_admission_broad8_spark.sh`
- Host: `spark-1`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-candidate-cache-v1`
- Run dir:
  `/home/huoju/leask/runs/ii42-m350-anchor-alpha-grid-v1`
- Output:
  `/home/huoju/leask/runs/ii42-m350-anchor-alpha-grid-v1/m350_rankdisc030_anchor_grid_seed1050_split1050.json`

Training matches M346A/M349:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`
- `LISTWISE_WEIGHT=0.80`
- `RANK_DISCOUNT_ADMISSION_WEIGHT=0.30`
- `RANK_DISCOUNT_ADMISSION_CUTOFF=100`
- `RANK_DISCOUNT_ADMISSION_MARGIN=0.03`
- `RANK_DISCOUNT_ADMISSION_NEGATIVES=32`

Post-hoc alpha grid:

- `POSTHOC_ANCHOR_ALPHA="0.08 0.10 0.12 0.15 0.18 0.20 0.22 0.25 0.28 0.30 0.35"`

## Stop Rule

Promote anchor policy work only if the best M350 row still improves heldout
NDCG@10 and MAP@100 over M346A, and the winner is not an isolated single alpha.
If the gain is stable around a local region, the next step should be a
query-shape policy or feature-level alpha predictor. If it is not stable, stop
anchor policy work and move to richer ranking evidence or stronger teacher
signals.

## Current Status

- Started `M350A` on `spark-1` in tmux session
  `ii42_m350_anchor_grid`.
- `M350A` output target:
  `/home/huoju/leask/runs/ii42-m350-anchor-alpha-grid-v1/m350_rankdisc030_anchor_grid_seed1050_split1050.json`
- `M350A` alpha grid:
  `0.08 0.10 0.12 0.15 0.18 0.20 0.22 0.25 0.28 0.30 0.35`
- Started `M350B` on `spark-2` in tmux session
  `ii42_m350_low_alpha_grid`.
- `M350B` output target:
  `/home/huoju/leask/runs/ii42-m350-anchor-alpha-low-grid-v1/m350_rankdisc030_anchor_low_grid_seed1050_split1050.json`
- `M350B` alpha grid:
  `0.00 0.01 0.02 0.03 0.04 0.05 0.06 0.07`
- `spark-2` had 8/8 expected candidate cache files by exact dataset prefix
  before launch.
- The `spark-2` work directory is not a git checkout, so the current scorer
  and runner files were synchronized from the local workspace with `rsync`
  before launch.

## Pending Result Extraction

When both JSON files are ready, compare all post-hoc alpha rows against:

- M346A scorer heldout:
  `Recall@100=0.683902`, `MRR@20=0.378038`,
  `NDCG@10=0.368333`, `MAP@100=0.299527`
- M346A scorer all-query:
  `Recall@100=0.707999`, `MRR@20=0.378358`,
  `NDCG@10=0.372187`, `MAP@100=0.307328`
- M349 best heldout row:
  `alpha=0.20`, `NDCG@10=0.369914`, `MAP@100=0.300364`

The decision should prefer a stable alpha region over a single isolated peak.
If the low-alpha rows preserve all-query metrics while still improving heldout
NDCG/MAP, prefer that safer policy over the absolute heldout winner.

## Result

Both distributed shards completed and wrote final JSON:

- `M350A` high-alpha result:
  `/home/huoju/leask/runs/ii42-m350-anchor-alpha-grid-v1/m350_rankdisc030_anchor_grid_seed1050_split1050.json`
- `M350B` low-alpha result:
  `/home/huoju/leask/runs/ii42-m350-anchor-alpha-low-grid-v1/m350_rankdisc030_anchor_low_grid_seed1050_split1050.json`

Merged heldout aggregate:

| Alpha | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| scorer | 0.683902 | 0.378038 | 0.368333 | 0.299527 |
| 0.00 | 0.683902 | 0.378038 | 0.368333 | 0.299527 |
| 0.01 | 0.684025 | 0.378376 | 0.368628 | 0.299869 |
| 0.02 | 0.683641 | 0.378383 | 0.368870 | 0.299964 |
| 0.03 | 0.684136 | 0.378437 | 0.369319 | 0.300175 |
| 0.04 | 0.684057 | 0.378289 | 0.369207 | 0.300090 |
| 0.05 | 0.684364 | 0.378335 | 0.369245 | 0.300193 |
| 0.06 | 0.684478 | 0.378478 | 0.369394 | 0.300183 |
| 0.07 | 0.684720 | 0.378500 | 0.369398 | 0.300222 |
| 0.08 | 0.685435 | 0.378526 | 0.369507 | 0.300305 |
| 0.10 | 0.685586 | 0.378355 | 0.369516 | 0.300242 |
| 0.12 | 0.686025 | 0.378144 | 0.369496 | 0.300102 |
| 0.15 | 0.685381 | 0.378839 | 0.369649 | 0.300252 |
| 0.18 | 0.685850 | 0.378616 | 0.370024 | 0.300344 |
| 0.20 | 0.685625 | 0.378638 | 0.369914 | 0.300364 |
| 0.22 | 0.685645 | 0.378717 | 0.369725 | 0.300430 |
| 0.25 | 0.686104 | 0.378059 | 0.369467 | 0.300233 |
| 0.28 | 0.685853 | 0.378214 | 0.369340 | 0.300223 |
| 0.30 | 0.685502 | 0.378266 | 0.369346 | 0.300251 |
| 0.35 | 0.685779 | 0.378042 | 0.368923 | 0.299944 |
| upper | 0.888848 | 1.000000 | 0.930000 | 0.888848 |

Merged all-query aggregate:

| Alpha | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| scorer | 0.707999 | 0.378358 | 0.372187 | 0.307328 |
| 0.00 | 0.707999 | 0.378358 | 0.372187 | 0.307328 |
| 0.01 | 0.707786 | 0.378378 | 0.372288 | 0.307370 |
| 0.02 | 0.706983 | 0.378372 | 0.372369 | 0.307374 |
| 0.03 | 0.707038 | 0.378365 | 0.372586 | 0.307407 |
| 0.04 | 0.706584 | 0.378319 | 0.372551 | 0.307363 |
| 0.05 | 0.706054 | 0.378411 | 0.372509 | 0.307379 |
| 0.06 | 0.705587 | 0.378416 | 0.372451 | 0.307295 |
| 0.07 | 0.705220 | 0.378485 | 0.372363 | 0.307292 |
| 0.08 | 0.705532 | 0.378448 | 0.372384 | 0.307277 |
| 0.10 | 0.705251 | 0.378390 | 0.372280 | 0.307128 |
| 0.12 | 0.705046 | 0.378255 | 0.372125 | 0.306894 |
| 0.15 | 0.702719 | 0.378249 | 0.371983 | 0.306690 |
| 0.18 | 0.702202 | 0.377973 | 0.371813 | 0.306325 |
| 0.20 | 0.701276 | 0.377732 | 0.371444 | 0.305994 |
| 0.22 | 0.701033 | 0.377835 | 0.371235 | 0.305988 |
| 0.25 | 0.700660 | 0.377510 | 0.370945 | 0.305723 |
| 0.28 | 0.699898 | 0.377219 | 0.370641 | 0.305339 |
| 0.30 | 0.698851 | 0.377304 | 0.370610 | 0.305284 |
| 0.35 | 0.698378 | 0.377049 | 0.370217 | 0.304917 |
| upper | 0.941474 | 1.000000 | 0.963847 | 0.941474 |

Best rows:

| Criterion | Alpha | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Heldout NDCG@10 | 0.18 | 0.685850 | 0.378616 | 0.370024 | 0.300344 |
| Heldout MAP@100 | 0.22 | 0.685645 | 0.378717 | 0.369725 | 0.300430 |
| All-query NDCG@10 | 0.03 | 0.707038 | 0.378365 | 0.372586 | 0.307407 |
| All-query MAP@100 | 0.03 | 0.707038 | 0.378365 | 0.372586 | 0.307407 |

## Decision

M350 confirms that the M349 signal is real, but also shows that a fixed high
alpha is not the safest policy. The high-alpha region `0.18..0.22` gives the
best heldout NDCG/MAP, but it degrades all-query Recall@100, NDCG@10, and
MAP@100 versus the base scorer.

The safer fixed policy is `alpha=0.03`:

- Heldout vs scorer:
  `NDCG@10 +0.000986`, `MAP@100 +0.000648`,
  `Recall@100 +0.000233`, `MRR@20 +0.000399`
- All-query vs scorer:
  `NDCG@10 +0.000399`, `MAP@100 +0.000079`,
  `Recall@100 -0.000962`, `MRR@20 +0.000007`

This points to a real but small anchor-blend effect. The next useful step is
not another global scalar sweep. It should be `M351`: learn or diagnose a
query-shape alpha policy that applies higher alpha only where the anchor is
helpful, while keeping low alpha for queries where high anchor weight hurts
all-query robustness.
