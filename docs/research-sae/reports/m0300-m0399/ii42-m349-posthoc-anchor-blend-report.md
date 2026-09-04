# ii42 M349 Post-Hoc Anchor Blend Report

## Goal

M348 showed that M346A remains the best completed broad8 variant, but it still
leaves most of the candidate upper-bound gap unresolved. The dominant failure
mode is ranking quality: many qrel positives are already in the candidate pool
but are not promoted high enough.

M349 tests a minimal policy question before more training-objective changes:
can a simple post-hoc blend with the existing BM25/atom anchor recover top-rank
quality?

The training objective is unchanged from M346A. The only new output is extra
post-hoc rows ranked by:

```text
scorer_score + alpha * anchor
```

where `anchor = normalized_bm25 + 0.5 * normalized_atom`.

## Contract

- Base runner: `scripts/run_m344_explicit_admission_broad8_spark.sh`
- Code change:
  `scripts/research_sae_m322_candidate_pool_scorer.py`
- Shared runner env:
  `scripts/run_m334_interaction_feature_scorer_spark.sh`
- Host: `spark-1`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-candidate-cache-v1`
- Run dir:
  `/home/huoju/leask/runs/ii42-m349-posthoc-anchor-blend-v1`
- Output:
  `/home/huoju/leask/runs/ii42-m349-posthoc-anchor-blend-v1/m349_rankdisc030_anchor_sweep_seed1050_split1050.json`

## Parameters

Training matches M346A:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`
- `LISTWISE_WEIGHT=0.80`
- `RANK_DISCOUNT_ADMISSION_WEIGHT=0.30`
- `RANK_DISCOUNT_ADMISSION_CUTOFF=100`
- `RANK_DISCOUNT_ADMISSION_MARGIN=0.03`
- `RANK_DISCOUNT_ADMISSION_NEGATIVES=32`

Post-hoc alpha sweep:

- `POSTHOC_ANCHOR_ALPHA="0.02 0.05 0.10 0.20 0.50 1.00"`

## Stop Rule

Promote policy work only if one of the anchor-blend rows improves heldout
NDCG@10 or MAP@100 over M346A without a material Recall@100 loss. If the sweep
does not improve M346A, the next route should not be more BM25 preservation or
anchor fusion; it should move to richer ranking evidence, model export, or a
stronger teacher/cross-encoder signal.

## Current Status

- Started on `spark-1` in tmux session `ii42_m349_anchor_sweep`.
- Confirmed `POSTHOC_ANCHOR_ALPHA` arguments reached the scorer command.
- Loaded all 8/8 broad8 candidate caches.
- Entered `train scorer variant=df_le_0p25`.
- Completed with early stop at epoch 30 and wrote the final JSON.

## Result

Final aggregate heldout comparison:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M346A scorer | 0.683902 | 0.378038 | 0.368333 | 0.299527 |
| anchor alpha 0.02 | 0.683641 | 0.378383 | 0.368870 | 0.299964 |
| anchor alpha 0.05 | 0.684364 | 0.378335 | 0.369245 | 0.300193 |
| anchor alpha 0.10 | 0.685586 | 0.378355 | 0.369516 | 0.300242 |
| anchor alpha 0.20 | 0.685625 | 0.378638 | 0.369914 | 0.300364 |
| anchor alpha 0.50 | 0.687992 | 0.376846 | 0.367357 | 0.299165 |
| anchor alpha 1.00 | 0.689329 | 0.374587 | 0.364650 | 0.297514 |
| candidate upper bound | 0.888848 | 1.000000 | 0.930000 | 0.888848 |

Final aggregate all-query comparison:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M346A scorer | 0.707999 | 0.378358 | 0.372187 | 0.307328 |
| anchor alpha 0.02 | 0.706983 | 0.378372 | 0.372369 | 0.307374 |
| anchor alpha 0.05 | 0.706054 | 0.378411 | 0.372509 | 0.307379 |
| anchor alpha 0.10 | 0.705251 | 0.378390 | 0.372280 | 0.307128 |
| anchor alpha 0.20 | 0.701276 | 0.377732 | 0.371444 | 0.305994 |
| anchor alpha 0.50 | 0.697641 | 0.376036 | 0.368675 | 0.303686 |
| anchor alpha 1.00 | 0.693236 | 0.373590 | 0.365633 | 0.300771 |

Best heldout row is `alpha=0.20`:

| Metric | Delta vs M346A |
| --- | ---: |
| Recall@100 | +0.001723 |
| MRR@20 | +0.000601 |
| NDCG@10 | +0.001581 |
| MAP@100 | +0.000838 |

Per-dataset effects for `alpha=0.20`:

| Dataset | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | 0.952381 | 0.268576 | 0.399934 | 0.270011 |
| `cqadupstack` | 0.711280 | 0.396943 | 0.397186 | 0.357076 |
| `fiqa` | 0.619196 | 0.398361 | 0.338239 | 0.276073 |
| `nfcorpus` | 0.287226 | 0.539442 | 0.330642 | 0.153338 |
| `scidocs` | 0.360188 | 0.294670 | 0.164754 | 0.111356 |
| `scifact` | 0.884674 | 0.596898 | 0.632143 | 0.588898 |
| `trec-covid` | 0.083153 | 0.783333 | 0.578626 | 0.051382 |
| `webis-touche2020` | 0.502936 | 0.672222 | 0.327671 | 0.165880 |

## Decision

M349 passes the stop rule. A small post-hoc anchor blend improves all four
heldout aggregate scorer metrics over M346A. The gain is still modest, but it
is cleaner than M347 LambdaRank because it improves ranking quality without a
new training objective.

The useful next step is M350: turn this from a fixed scalar into a stricter
policy/evidence experiment. The minimum version should evaluate a compact
alpha grid around `0.10..0.30` plus per-dataset/query-shape diagnostics. If the
gain survives, then add a runtime-safe query-level alpha predictor; if it does
not, stop anchor policy work and move to richer ranking evidence or a stronger
teacher/cross-encoder signal.
