# ii42 M351 Query-Shape Anchor Policy Report

## Goal

M350 showed that anchor blending has a real but small effect:

- low alpha, especially `alpha=0.03`, is safest on all-query metrics;
- high alpha around `0.18..0.22` gives the best heldout NDCG/MAP;
- high global alpha degrades all-query Recall/NDCG/MAP.

M351 tests the next route: use query-shape heuristics to apply high anchor
weight only when the query surface suggests that the anchor should help.

This is a diagnostic adaptive-policy run, not a new training objective. The
base scorer training remains identical to M346/M350.

## Contract

- Base runner: `scripts/run_m344_explicit_admission_broad8_spark.sh`
- Code change:
  `scripts/research_sae_m322_candidate_pool_scorer.py`
- Shared runner:
  `scripts/run_m334_interaction_feature_scorer_spark.sh`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-candidate-cache-v1`
- Run dir:
  `/home/huoju/leask/runs/ii42-m351-query-shape-anchor-policy-v1`
- Output:
  `/home/huoju/leask/runs/ii42-m351-query-shape-anchor-policy-v1/m351_query_shape_anchor_policy_seed1050_split1050.json`

Training matches M346/M350:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`
- `LISTWISE_WEIGHT=0.80`
- `RANK_DISCOUNT_ADMISSION_WEIGHT=0.30`
- `RANK_DISCOUNT_ADMISSION_CUTOFF=100`
- `RANK_DISCOUNT_ADMISSION_MARGIN=0.03`
- `RANK_DISCOUNT_ADMISSION_NEGATIVES=32`

## Policy Rows

Fixed-alpha controls:

- `0.03`: safer M350 all-query winner.
- `0.18`: M350 heldout NDCG winner.
- `0.22`: M350 heldout MAP winner.

Adaptive policies use:

- low alpha: `0.03`
- high alpha: `0.18`

Policies:

- `bm25_heavy`: high alpha when BM25-source candidate ratio is at least atom
  ratio.
- `atom_heavy`: high alpha when atom-source candidate ratio exceeds BM25
  ratio.
- `overlap_heavy`: high alpha when BM25/atom source overlap ratio is at least
  `0.10`.
- `anchor_spread`: high alpha when the query anchor has a strong top-anchor
  spread.
- `bm25_or_overlap`: high alpha when either BM25-heavy or overlap-heavy is
  true.
- `teacher_overlap`: diagnostic-only policy using dense-teacher overlap in the
  candidate pool. This is not directly product-safe, but it can show whether
  a learned policy should use teacher-like supervision.

## Stop Rule

Promote adaptive-anchor work only if at least one policy improves all-query
NDCG/MAP over fixed `alpha=0.03`, or materially improves heldout NDCG/MAP while
keeping all-query degradation below the M350 high-alpha rows.

If no policy beats fixed `alpha=0.03`, stop heuristic policy work and move to
a learned query policy or a stronger final-rank teacher.

## Current Status

- Completed on `spark-1`.
- Remote result:
  `/home/huoju/leask/runs/ii42-m351-query-shape-anchor-policy-v1/m351_query_shape_anchor_policy_seed1050_split1050.json`
- Local inspected copy:
  `/tmp/ii42-m351-query-shape-anchor-policy-v1/m351_query_shape_anchor_policy_seed1050_split1050.json`
- Training early-stopped at epoch 30 after four stale evals.

## Macro Results

Metrics are ordered as Recall@100, MRR@20, NDCG@10, MAP@100.

| Row | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| base scorer | heldout | 0.683902 | 0.378038 | 0.368333 | 0.299527 |
| base scorer | all | 0.707999 | 0.378358 | 0.372187 | 0.307328 |
| anchor `0.03` | heldout | 0.684136 | 0.378437 | 0.369319 | 0.300175 |
| anchor `0.03` | all | 0.707038 | 0.378365 | 0.372586 | 0.307407 |
| anchor `0.18` | heldout | 0.685850 | 0.378616 | 0.370024 | 0.300344 |
| anchor `0.18` | all | 0.702202 | 0.377973 | 0.371813 | 0.306325 |
| anchor `0.22` | heldout | 0.685645 | 0.378717 | 0.369725 | 0.300430 |
| anchor `0.22` | all | 0.701033 | 0.377835 | 0.371235 | 0.305988 |
| `anchor_spread` | heldout | 0.685850 | 0.378638 | 0.370045 | 0.300365 |
| `anchor_spread` | all | 0.702215 | 0.377995 | 0.371835 | 0.306354 |
| `atom_heavy` | heldout | 0.685488 | 0.378697 | 0.369681 | 0.300228 |
| `atom_heavy` | all | 0.705183 | 0.378226 | 0.372291 | 0.306874 |
| `bm25_heavy` | heldout | 0.684498 | 0.378356 | 0.369662 | 0.300292 |
| `bm25_heavy` | all | 0.704058 | 0.378112 | 0.372108 | 0.306859 |
| `bm25_or_overlap` | heldout | 0.685850 | 0.378616 | 0.370024 | 0.300344 |
| `bm25_or_overlap` | all | 0.702461 | 0.378213 | 0.372247 | 0.306761 |
| `overlap_heavy` | heldout | 0.685850 | 0.378616 | 0.370024 | 0.300344 |
| `overlap_heavy` | all | 0.702584 | 0.378289 | 0.372398 | 0.306955 |
| `teacher_overlap` | heldout | 0.684136 | 0.378437 | 0.369319 | 0.300175 |
| `teacher_overlap` | all | 0.707038 | 0.378365 | 0.372586 | 0.307407 |
| candidate upper bound | heldout | 0.888848 | 1.000000 | 0.930000 | 0.888848 |
| candidate upper bound | all | 0.941474 | 1.000000 | 0.963847 | 0.941474 |

## Adaptive Delta vs Fixed `0.03`

| Row | Split | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `anchor_spread` | heldout | +0.001715 | +0.000201 | +0.000726 | +0.000190 |
| `anchor_spread` | all | -0.004823 | -0.000370 | -0.000750 | -0.001053 |
| `atom_heavy` | heldout | +0.001353 | +0.000260 | +0.000362 | +0.000053 |
| `atom_heavy` | all | -0.001855 | -0.000139 | -0.000295 | -0.000533 |
| `bm25_heavy` | heldout | +0.000362 | -0.000081 | +0.000343 | +0.000117 |
| `bm25_heavy` | all | -0.002980 | -0.000253 | -0.000478 | -0.000549 |
| `bm25_or_overlap` | heldout | +0.001715 | +0.000179 | +0.000705 | +0.000169 |
| `bm25_or_overlap` | all | -0.004577 | -0.000152 | -0.000339 | -0.000646 |
| `overlap_heavy` | heldout | +0.001715 | +0.000179 | +0.000705 | +0.000169 |
| `overlap_heavy` | all | -0.004454 | -0.000075 | -0.000188 | -0.000453 |
| `teacher_overlap` | heldout | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `teacher_overlap` | all | +0.000000 | +0.000000 | +0.000000 | +0.000000 |

## Policy Usage

The policy statistics below are query-weighted across datasets.

| Policy | Split | High-alpha query rate | Mean alpha |
| --- | --- | ---: | ---: |
| `anchor_spread` | heldout | 0.991905 | 0.178786 |
| `anchor_spread` | all | 0.992323 | 0.178848 |
| `atom_heavy` | heldout | 0.420952 | 0.093143 |
| `atom_heavy` | all | 0.374565 | 0.086185 |
| `bm25_heavy` | heldout | 0.579048 | 0.116857 |
| `bm25_heavy` | all | 0.625435 | 0.123815 |
| `bm25_or_overlap` | heldout | 0.994286 | 0.179143 |
| `bm25_or_overlap` | all | 0.986819 | 0.178023 |
| `overlap_heavy` | heldout | 0.986905 | 0.178036 |
| `overlap_heavy` | all | 0.972625 | 0.175894 |
| `teacher_overlap` | heldout | 0.000000 | 0.030000 |
| `teacher_overlap` | all | 0.000000 | 0.030000 |

The source ratios are high almost everywhere:

- heldout mean BM25 ratio `0.933667`, atom ratio `0.951446`, overlap ratio
  `0.885113`;
- all-query mean BM25 ratio `0.929649`, atom ratio `0.944049`, overlap ratio
  `0.878341`.

This explains the failure mode: source-presence heuristics are too blunt. Most
policies either collapse into high alpha, or into low alpha, instead of finding
the query subset where anchor blending is selectively useful.

## Decision

Do not promote M351 heuristic adaptive-anchor policies.

`anchor_spread` gives the best heldout row, but it effectively degenerates into
global high alpha and loses all-query Recall/NDCG/MAP versus fixed `0.03`.
`atom_heavy` is the least harmful heuristic compromise, but it still fails the
safe gate because all-query NDCG and MAP remain below fixed `0.03`.

The useful conclusion is narrower:

- anchor blending is a real but small posthoc ranking signal;
- fixed `alpha=0.03` remains the safest product-like setting;
- heuristic query-shape thresholds are not expressive enough;
- if this path continues, the next version should learn the policy from
  per-query delta labels, or move the signal into a stronger final
  admission/ranking objective instead of adding more hand-written thresholds.
