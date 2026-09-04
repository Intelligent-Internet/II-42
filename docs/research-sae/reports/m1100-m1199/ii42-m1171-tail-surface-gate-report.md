# M1171 Tail Surface Gate

M1171 tests whether the M1170 tail_full-vs-direct decision can be learned from
query-time features under leave-one-dataset-out validation.

## Inputs

- Rows: 138 M1170 selected queries.
- Features: M1144 native tail features, M1144 class probabilities, M1145 action
  probabilities, and M1145 action-specific boundary features.
- Labels:
  - `tail_better`: tail_full utility beats direct_protect.
  - `tail_rank_gain`: tail_full improves MAP/NDCG/MRR combined.
  - `tail_safe`: tail_full does not reduce CUB or Recall versus direct.
- Output: `runs/m1171_tail_surface_gate_v1/tail_surface_gate.json`.

## Predictability

| Label | Positives | LODO AUC |
| --- | ---: | ---: |
| `tail_better` | 61 | 0.524377 |
| `tail_rank_gain` | 61 | 0.524377 |
| `tail_safe` | 134 | 0.567164 |

The current deployable features do not reliably separate safe tail switches.

## Best Policy

No row-floor-clean policy was found.

The best non-clean policy accepts 76 selected queries:

| Policy | dCUB | dRecall | dMAP | dNDCG | dMRR | Clean |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `better0.10_safe0.90_rank0.30` | +0.000000 | +0.000000 | +0.000530 | +0.000736 | +0.003142 | false |

The failure is row-level/dataset-level harm:

| Metric | Min Dataset Tail-Gate Minus Direct |
| --- | ---: |
| CUB | +0.000000 |
| Recall@100 | +0.000000 |
| MAP@100 | -0.000294 |
| NDCG@10 | -0.019744 |
| MRR@20 | +0.000000 |

## Interpretation

M1170 proved the M1137 tail surface carries real ranking value.  M1171 shows
the current M1144/M1145 feature gate is not safe enough to deploy that value
globally.  This is not a reason to return to compact query-delta injection;
M1169 already rejected that.  It means the surface-router branch needs a
stronger guard or a more local/native validation mechanism.

## Decision

Do not promote a learned blind tail router yet.

Promising next directions:

1. Use tail_full as a reranking/reference teacher, but require native replay
   validation before accepting a query class.
2. Build a class-conditioned router that only targets the M1166 classes where
   M1170 showed stable gains: `clean_relevant_entry`, `rank_gain_no_top100_recall`,
   and selected `relevant_swap`.
3. Add better query-time features for tail harm, because current no-harm
   prediction is weak.

Stop condition for the current gate: current features alone are insufficient
for a safe deployable surface router.
