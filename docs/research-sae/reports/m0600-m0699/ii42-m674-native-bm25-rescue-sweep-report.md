# M674 Native BM25 Rescue Sweep Report

## Objective

M674 tests whether the deterministic M670/M673 tail-rescue rule generalizes
from the initial four native rows to the full local shared15 native surface.

The tested rule is intentionally bounded:

- keep the native P1-a0125 candidate set unchanged;
- keep the fused top head untouched;
- reorder only the tail by native BM25 raw score;
- require same-query identity across BM25, dense, P1, and rescued P1;
- reject any setting with CUB, NDCG@10, or MRR@20 regression.

This is a second-stage scorer/ranking probe.  It does not change the P1 encoder
or posting generator.

## Implementation Notes

The first M674 runner attempt repeated the native doc-ord mapping work once per
`preserve_top_k` value.  That made large rows such as `fever` and `msmarco`
unnecessarily slow.  The runner was rebuilt so each dataset extracts native
feature rows once, then evaluates all preserve settings in memory.

An additional wiring bug was found and fixed: the M674 runner inherited the
generic native evaluator default schema (`ii42_beir15`) instead of using the
local engineering schema (`ii42_shared15`).  The broken run produced zero-query
P1 rows and was discarded.  The valid run uses `ii42_shared15`.

Artifacts:

- Summary: `runs/m674_native_bm25_rescue_sweep_v1/m674_sweep_summary.json`
- Best matrix: `runs/m674_native_bm25_rescue_sweep_v1/m674_shared15_k95_matrix.json`
- Best report: `runs/m674_native_bm25_rescue_sweep_v1/m674_shared15_k95_matrix.md`

## Sweep Result

All five preserve settings passed the strict same-query gate on shared15.  The
best setting is `preserve_top_k=95`.

| preserve_top_k | Accepted | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | Row harms |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 95 | true | +0.000000 | +0.005480 | +0.000412 | +0.000000 | +0.000000 | 0 |
| 96 | true | +0.000000 | +0.004630 | +0.000378 | +0.000000 | +0.000000 | 0 |
| 97 | true | +0.000000 | +0.003832 | +0.000288 | +0.000000 | +0.000000 | 0 |
| 98 | true | +0.000000 | +0.003606 | +0.000292 | +0.000000 | +0.000000 | 0 |
| 99 | true | +0.000000 | +0.001087 | +0.000160 | +0.000000 | +0.000000 | 0 |

## Best Matrix

Full shared15 query count is 1342.  Query identities are consistent across
BM25, dense, P1-a0125, and M674.

| Source | CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.906167 | 0.783718 | 0.593976 | 0.670058 | 0.788108 |
| dense | 0.868998 | 0.778166 | 0.640422 | 0.728543 | 0.835412 |
| P1-a0125 | 0.937309 | 0.843488 | 0.662742 | 0.744939 | 0.839746 |
| P1-a0125+M670-k95 | 0.937309 | 0.848968 | 0.663154 | 0.744939 | 0.839746 |

Delta for `P1-a0125+M670-k95`:

| Compared With | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | +0.031142 | +0.065249 | +0.069178 | +0.074880 | +0.051638 |
| dense | +0.068312 | +0.070801 | +0.022732 | +0.016396 | +0.004334 |
| P1-a0125 | +0.000000 | +0.005480 | +0.000412 | +0.000000 | +0.000000 |

## Dataset Movement

The improvement is distributed, but shallow.  It appears only in Recall@100 and
MAP@100 because the rule deliberately avoids changing the protected head.

Rows with non-zero movement at `preserve_top_k=95`:

| Dataset | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: |
| climate-fever | +0.010000 | +0.000203 | +0.000000 | +0.000000 |
| cqadupstack | +0.000759 | +0.000194 | +0.000000 | +0.000000 |
| dbpedia-entity | +0.008292 | +0.001447 | +0.000000 | +0.000000 |
| hotpotqa | +0.005000 | +0.000100 | +0.000000 | +0.000000 |
| msmarco | +0.011550 | +0.001134 | +0.000000 | +0.000000 |
| nfcorpus | +0.014366 | +0.000810 | +0.000000 | +0.000000 |
| scidocs | +0.004000 | +0.000184 | +0.000000 | +0.000000 |
| scifact | +0.020000 | +0.000206 | +0.000000 | +0.000000 |
| trec-covid | +0.001802 | +0.000717 | +0.000000 | +0.000000 |
| webis-touche2020 | +0.006431 | +0.001186 | +0.000000 | +0.000000 |

## Interpretation

M674 proves that support-safe boundary crossing is possible on the native
shared15 engineering path.  The result is not large enough to be the final
breakthrough by itself, but it is important because it survives a broader
same-query matrix without CUB, NDCG, or MRR regression.

This supports the recent strategic critique:

- do not return to traditional SAE reconstruction loss as the main path;
- keep unified posting as the engineering form;
- keep dense faithfulness as the first-stage guard;
- use second-stage retrieval constraints to learn safe boundary movement.

The current deterministic tail rule is best treated as a teacher/probe for the
next compiler objective, not as the final scorer.

## Current Block

The remaining bottleneck is not candidate generation.  P1 already has higher
CUB than BM25 and dense on this surface.  The bottleneck is converting that
candidate capacity into better top100 and eventually top10 ranking without
breaking dense geometry.

M674 shows a hand-written BM25 tail rule can recover some top100 positives.
However, because it only reorders the tail, it cannot improve NDCG@10 or MRR.
This is the central limitation.

## Next Breakthrough Point

The next meaningful stage should be a retrieval-constrained generated-posting
compiler:

1. Keep the dense-root unified posting support as the first-stage floor.
2. Learn only generated posting deltas that satisfy support/overlap/CUB floors.
3. Train on under-ranked positives and dense-miss positives from qrels,
   BM25/entity/corpus-derived pseudo positives, and native candidate evidence.
4. Evaluate only through the native DB path.
5. Reject any variant whose gains come from dataset-specific thresholds or whose
   broader matrix breaks dense faithfulness.

In short: M674 keeps second-stage recovery alive, but the larger breakthrough
should come from a support-safe retrieval-expanded posting compiler, not from
more alpha tuning or a return to SAE reconstruction.
