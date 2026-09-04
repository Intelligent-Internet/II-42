# M644 / P1.3 Query-Local Replacement Ranker Report

Status: `not_promoted_linear_replacement_too_conservative`.

## Goal

M644 tests the next step after M643.  M643 showed that row weights can reduce
top100 positive displacement, but they also suppress under-ranked positive
promotion.  The remaining failure is query-local: a boundary positive should
replace a safe top100 negative, but should not replace a protected top100
positive.

M644 therefore builds query-local replacement pairs:

- under-ranked positive > safe top100 negative
- protected top100 positive > under-ranked positive
- protected top100 positive > safe top100 negative

The model is a global linear pairwise scorer over the same M629 feature set, so
it remains auditable and replay-compatible.

## Artifacts

| Artifact | Path |
| --- | --- |
| Script | `scripts/train_m644_query_local_replacement_ranker.py` |
| Tests | `tests/test_train_m644_query_local_replacement_ranker.py` |
| Replay | `runs/m644_p1p3_query_local_replacement_ranker_v1/` |

## Pair Surface

M644 generated `110,791` query-local raw pairs:

| Pair type | Count |
| --- | ---: |
| promotion | 21,846 |
| protection | 23,857 |
| top_positive_negative | 65,088 |

This confirms the training surface is materially different from M629/M643.  It
contains explicit positive-vs-positive protection constraints, not just
positive-vs-negative row classification.

## Native Replay Result

Best configuration:

- alpha `0.15`
- preserve top `95`
- rerank window `350`

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Candidate UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `P1.3-a010 proxy` | 0.745306 | 0.688652 | 0.889652 | 0.838500 | 0.993377 |
| `P1.3-a010+M644` | 0.745306 | 0.688740 | 0.890204 | 0.838500 | 0.993377 |

| Metric | Delta |
| --- | ---: |
| dNDCG@10 | +0.000000 |
| dMAP@100 | +0.000088 |
| dRecall@100 | +0.000552 |
| dMRR@20 | +0.000000 |

## Promotion / Displacement

| Counter | M641 validation | M643 soft | M644 |
| --- | ---: | ---: | ---: |
| Under-ranked positives promoted | 57 | 38 | 7 |
| Top100 positives displaced | 49 | 30 | 2 |
| Net top100 positives | +8 | +8 | +5 |
| Positive gain efficiency | 0.140 | 0.211 | 0.714 |

M644 almost eliminates harmful displacement, which proves the pairwise
protection signal works.  But it is too conservative: only seven under-ranked
positives enter top100, so Recall@100 is far below the M629 and M641 gains.

## Per-Dataset Recall Delta

| Dataset | dRecall@100 |
| --- | ---: |
| arguana | +0.000000 |
| climate-fever | +0.000000 |
| cqadupstack | +0.000726 |
| dbpedia-entity | +0.000148 |
| fever | +0.000000 |
| fiqa | +0.000000 |
| hotpotqa | +0.000000 |
| msmarco | +0.000000 |
| nfcorpus | +0.006189 |
| nq | +0.000000 |
| quora | +0.000000 |
| scidocs | +0.000000 |
| scifact | +0.000000 |
| trec-covid | +0.001126 |
| webis-touche2020 | +0.000000 |

No major per-dataset regression appears.  Max positive dataset share is
`0.756`, just above the `0.75` dominance threshold.

## Diagnosis

M644 is an important negative result:

- Query-local pair construction fixed the displacement problem.
- The linear scorer cannot turn that pair surface into enough promotions.
- The model becomes a conservative protector, not an effective replacer.

This clarifies the bottleneck:

1. M641 nonlinear row classifier has enough capacity to promote positives, but
   displaces too many protected positives.
2. M643 row weights can reduce displacement, but lose recall.
3. M644 query-local linear pairs nearly remove displacement, but are too
   conservative.

The missing component is not the pair surface.  The missing component is
nonlinear replacement scoring that can preserve protected positives while still
finding safe negatives to replace.

## Decision

Do not promote M644.

Do not expand the linear query-local replacement scorer.  It does not beat the
M629 reference and is far below M641.

Keep M644's pair builder as a useful diagnostic/training surface.  The next
attempt should combine M641's nonlinear capacity with M644's query-local
replacement supervision.

## Next Step

The next useful stage is M645:

1. Use the M644 query-local pairs to produce replacement-aware training labels
   or sample weights.
2. Train a nonlinear row scorer, not a linear pairwise scorer.
3. Keep explicit protected-positive constraints by giving protected top100
   positives a high score target, under-ranked positives a middle-high target,
   and safe top100 negatives a low target.
4. Replay with the same M642 promotion and dominance gate.

Acceptance stays unchanged:

- dRecall@100 >= `+0.005`
- dMAP@100 >= `0`
- no NDCG/MRR regression
- no major dataset Recall@100 regression below `-0.001`
- max positive dataset share <= `0.75`

If M645 still cannot exceed M641, then the second-stage scorer line should
pause and the M641/M644 recovered and protected examples should be moved into
first-stage generated-posting training.

## Verification

Completed:

- `python3 -m py_compile scripts/train_m644_query_local_replacement_ranker.py scripts/train_m643_displacement_aware_boundary_ranker.py scripts/train_m641_nonlinear_native_boundary_ranker.py`
- `pytest -q tests/test_train_m644_query_local_replacement_ranker.py`
- `python3 scripts/train_m644_query_local_replacement_ranker.py --output-root runs/m644_p1p3_query_local_replacement_ranker_v1 --model-blend-alpha-grid 0.05,0.08,0.10,0.12,0.15 --preserve-top-k-grid 95,98,99 --rerank-max-rank-grid 250,300,350`
