# ii42 M825 Route Scorecard

## Verdict

Stop the M821-M824 generated-bundle selector branch and the blind df/reuse-band atom branch. Keep B12/M150 as admission/scoring controls, and keep M1050 as the low-fanout atom-posting baseline. The next useful route is semantic-neighborhood posting utility, not another threshold/model swap.

## Generated-Bundle Branch

- M821 clean oracle surfaces: 3/3
- M824 failed policy surfaces: seed7642, seed7643

Interpretation: proposal coverage can be repaired, but the observable selector interface cannot recover the oracle under leave-surface-out. This branch should not continue as selector tuning.

## Atom-Posting Branch

### M1050 Heldout Aggregate

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.484541 | 0.392835 | 0.325236 | 0.258054 |
| atom_only | 0.189900 | 0.103341 | 0.095727 | 0.083132 |
| unified_scale_0.5 | 0.503889 | 0.405647 | 0.337627 | 0.270175 |

M1050 heldout deltas versus lexical BM25:

| Row | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| unified_0.5_minus_lexical | 0.019348 | 0.012812 | 0.012391 | 0.012121 |
| atom_only_minus_lexical | -0.294640 | -0.289494 | -0.229509 | -0.174922 |

### M1060 Heldout Aggregate

Scope caveat: this aggregate covers only M1060 eval datasets, not the same three-dataset surface as M1050.

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.521808 | 0.505866 | 0.429390 | 0.330480 |
| atom_only | 0.268728 | 0.190144 | 0.172266 | 0.133618 |
| unified_scale_0.5 | 0.538946 | 0.525899 | 0.448644 | 0.348979 |

M1060 heldout deltas versus lexical BM25:

| Row | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| unified_0.5_minus_lexical | 0.017138 | 0.020033 | 0.019254 | 0.018499 |
| atom_only_minus_lexical | -0.253080 | -0.315723 | -0.257124 | -0.196862 |

M1060 minus M1050 on common datasets for `unified_scale_0.5`:

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| nfcorpus | 0.002727 | 0.010446 | 0.012435 | 0.008024 |
| scifact | -0.020000 | -0.012544 | -0.014821 | -0.017842 |

Interpretation: M1050 is the safer baseline because it gives a three-dataset unified gain with low-fanout atoms. M1060 improves nfcorpus but regresses scifact on the common fair split, so reuse-band statistics alone are not the missing supervision.

## Historical Controls

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| dense_continuity | 0.313200 | 0.299200 | 0.225100 | 0.150400 |
| b12_checkpoint_bm25_sae | 0.334400 | 0.301700 | 0.222500 | 0.152600 |
| b12_residual | 0.332200 | 0.294500 | 0.227800 | 0.158600 |
| m150_a1_c6_historical_frontier | 0.384100 | 0.427100 | 0.301000 | 0.200800 |

B12 and M150 show that BM25+SAE admission/scoring can beat dense. They should remain controls for the next posting-utility route.

## Next Route

- ID: `M825/M1100 semantic-neighborhood posting utility`
- Objective: Start from M1050-style low-fanout posting atoms, add a dense/B12 semantic teacher neighborhood and final admission constraints, and require heldout unified gains without atom-only collapse or fanout regression.
- First gate: Three-dataset M1050 replay with semantic-neighborhood teacher positives and hard lexical negatives; pass only if heldout unified 0.5 beats lexical on recall and MAP without lowering NDCG/MRR macro.

## Stop Rules

- Stop if semantic-neighborhood supervision improves train but not heldout.
- Stop if atom-only remains weak and unified gains vanish versus lexical.
- Stop if fanout regresses toward high-df/full-corpus behavior.
- Stop if gains require dataset-specific thresholds or profile choice.
