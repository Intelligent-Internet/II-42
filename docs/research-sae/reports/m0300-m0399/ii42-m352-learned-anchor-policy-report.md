# ii42 M352 Learned Anchor Policy Report

## Goal

M351 showed that hand-written query-shape anchor policies do not beat fixed
`alpha=0.03` on the safe all-query gate. The failure mode was clear: source
ratios and anchor spread are too blunt, and most policies collapse into either
near-global high alpha or fixed low alpha.

M352 tests the next narrower question: can a small learned query-level gate,
trained only on train-query low/high alpha deltas, select when to use the
posthoc anchor signal without hurting the all-query surface?

This remains a posthoc diagnostic. It does not change the scorer training loss.

## Contract

- Base runner: `scripts/run_m344_explicit_admission_broad8_spark.sh`
- Shared runner: `scripts/run_m334_interaction_feature_scorer_spark.sh`
- M352 runner: `scripts/run_m352_learned_anchor_policy_spark.sh`
- Run dir:
  `/home/huoju/leask/runs/ii42-m352-learned-anchor-policy-v1`
- Output:
  `/home/huoju/leask/runs/ii42-m352-learned-anchor-policy-v1/m352_learned_anchor_policy_seed1050_split1050.json`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-candidate-cache-v1`

Training is kept comparable with M351:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`
- `LISTWISE_WEIGHT=0.80`
- `RANK_DISCOUNT_ADMISSION_WEIGHT=0.30`
- `RANK_DISCOUNT_ADMISSION_CUTOFF=100`
- `RANK_DISCOUNT_ADMISSION_MARGIN=0.03`
- `RANK_DISCOUNT_ADMISSION_NEGATIVES=32`

## Learned Policy

The learned gate uses train queries only:

1. Rank each train query with low alpha `0.03`.
2. Rank each train query with high alpha `0.18`.
3. Label the query as high-alpha-positive if the high-alpha row improves the
   selected objective over low alpha.
4. Train a small logistic gate on query-only features derived from source
   ratios, anchor statistics, scorer confidence, and low/high top-rank
   stability.
5. Apply the learned gate to train, heldout, and all-query splits.

Objectives evaluated:

- `ndcg_map`: label by `NDCG@10 + MAP@100`;
- `ndcg`: label by `NDCG@10`;
- `map`: label by `MAP@100`.

Fixed-alpha controls are included in the same run:

- `0.03`;
- `0.18`;
- `0.22`.

## Stop Rule

Promote this line only if a learned policy improves all-query NDCG/MAP over
fixed `alpha=0.03`, or gives a clearly better heldout row while keeping
all-query degradation below the M351/M350 high-alpha degradation.

If learned policies also fail this gate, stop posthoc anchor-policy work. The
anchor signal should then be treated as a small diagnostic feature and moved
into a stronger final admission/ranking objective only if that broader route is
already being tested.

## Current Status

- Local implementation added and syntax-checked.
- Runner added and syntax-checked.
- Launched on `spark-1` in tmux session `ii42_m352_learned_anchor`.
- Confirmed the command includes fixed alpha controls `0.03 0.18 0.22`.
- Confirmed the command includes learned objectives `ndcg_map`, `ndcg`, and
  `map`.
- Confirmed 8/8 candidate caches loaded successfully.
- Completed on `spark-1`.
- Training early-stopped at epoch 30 after four stale evals.
- Remote result:
  `/home/huoju/leask/runs/ii42-m352-learned-anchor-policy-v1/m352_learned_anchor_policy_seed1050_split1050.json`
- Local inspected copy:
  `/tmp/ii42-m352-learned-anchor-policy-v1/m352_learned_anchor_policy_seed1050_split1050.json`

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
| learned `ndcg_map` | heldout | 0.685795 | 0.378363 | 0.369971 | 0.300305 |
| learned `ndcg_map` | all | 0.703534 | 0.378573 | 0.372945 | 0.307506 |
| learned `ndcg` | heldout | 0.685722 | 0.378359 | 0.370168 | 0.300530 |
| learned `ndcg` | all | 0.705132 | 0.378604 | 0.372955 | 0.307633 |
| learned `map` | heldout | 0.685795 | 0.378363 | 0.369969 | 0.300306 |
| learned `map` | all | 0.703474 | 0.378575 | 0.372945 | 0.307526 |
| candidate upper bound | heldout | 0.888848 | 1.000000 | 0.930000 | 0.888848 |
| candidate upper bound | all | 0.941474 | 1.000000 | 0.963847 | 0.941474 |

## Delta vs Fixed `0.03`

| Row | Split | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| learned `ndcg_map` | heldout | +0.001660 | -0.000074 | +0.000652 | +0.000130 |
| learned `ndcg_map` | all | -0.003504 | +0.000209 | +0.000360 | +0.000099 |
| learned `ndcg` | heldout | +0.001587 | -0.000078 | +0.000848 | +0.000355 |
| learned `ndcg` | all | -0.001906 | +0.000239 | +0.000369 | +0.000226 |
| learned `map` | heldout | +0.001660 | -0.000074 | +0.000650 | +0.000131 |
| learned `map` | all | -0.003563 | +0.000210 | +0.000359 | +0.000119 |

## Learned Policy Usage

| Policy | Split | High-alpha query rate | Mean alpha |
| --- | --- | ---: | ---: |
| `ndcg_map` | heldout | 0.756190 | 0.143429 |
| `ndcg_map` | all | 0.633691 | 0.125054 |
| `ndcg` | heldout | 0.656905 | 0.128536 |
| `ndcg` | all | 0.531431 | 0.109715 |
| `map` | heldout | 0.757143 | 0.143571 |
| `map` | all | 0.634849 | 0.125227 |

Training labels and fit diagnostics:

| Policy | Positive label rate | Predicted high rate | Train accuracy |
| --- | ---: | ---: | ---: |
| `ndcg_map` | 0.130178 | 0.443417 | 0.659393 |
| `ndcg` | 0.027367 | 0.336538 | 0.680473 |
| `map` | 0.130178 | 0.444896 | 0.659393 |

## Decision

M352 is the first positive evidence for a learned anchor gate. It beats fixed
`alpha=0.03` on all-query NDCG/MAP/MRR, and the best row is learned `ndcg`:

- all-query NDCG@10: `+0.000369`;
- all-query MAP@100: `+0.000226`;
- all-query MRR@20: `+0.000239`;
- all-query Recall@100: `-0.001906`.

This should not be treated as final promotion because Recall still drops.
However, it is meaningfully better than M351 heuristics: the learned gate does
not collapse into global high alpha, and it preserves enough all-query behavior
to produce a net rank-quality improvement.

Next step: promote the learned-policy direction to a recall-constrained M353
canary. The immediate target should be a threshold/calibration sweep over the
learned gate probability or a min-delta label margin, with the objective of
keeping the NDCG/MAP gain while reducing Recall loss.
