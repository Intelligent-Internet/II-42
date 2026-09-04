# M648 / P1.3 Soft Risk Penalty Ranker Report

## Status

M648 is a controlled negative result.

It tested the most direct follow-up to M647: keep the safer M646 soft
promotion scorer, add a learned protected-slot risk model, and subtract that
risk before native row reranking.  The result preserves the M646 gain but does
not create a new improvement surface.  The best normal grid selects
`risk_penalty_lambda=0.0`; the high-risk grid selects `2.0`, but only improves
MAP by about `3.3e-7` with identical Recall and identical promotion structure.

This means the current scalar protected-slot risk penalty is not the missing
piece.  It does not absorb M647's broader coverage advantage.

## Runs

| Run | Purpose |
| --- | --- |
| `runs/m648_p1p3_soft_risk_penalty_ranker_v1` | normal risk grid `0.0..1.0` |
| `runs/m648_p1p3_soft_risk_penalty_ranker_highrisk_v1` | stress grid `0.0,2.0,5.0,10.0,20.0` |

Training surface:

| Counter | Value |
| --- | ---: |
| M646 promotion pairs | 103224 |
| risk train slots | 5200 |
| risk positive slots | 244 |
| risk negative slots | 4956 |

## Best Replay

| Metric | M646 Promote | M648 Highrisk | Difference |
| --- | ---: | ---: | ---: |
| dNDCG@10 | +0.000000 | +0.000000 | +0.000000 |
| dMAP@100 | +0.000123115 | +0.000123449 | +0.000000334 |
| dRecall@100 | +0.003856088 | +0.003856088 | +0.000000000 |
| dMRR@20 | +0.000000 | +0.000000 | +0.000000 |
| promoted positives | 13 | 13 | 0 |
| displaced positives | 7 | 7 | 0 |
| net top100 positives | +6 | +6 | 0 |
| positive gain efficiency | 0.462 | 0.462 | 0.000 |

Best high-risk config:

| Parameter | Value |
| --- | ---: |
| risk penalty lambda | 2.0 |
| model blend alpha | 0.12 |
| preserve top-k | 95 |
| rerank max rank | 250 |
| gate status | offline gate passed |
| promotion status | capacity breakthrough dominance limited |

Dataset Recall deltas for the best high-risk replay:

| Dataset | dRecall@100 |
| --- | ---: |
| fiqa | +0.052632 |
| nfcorpus | +0.006956 |
| cqadupstack | +0.000726 |
| trec-covid | -0.000034 |

## What This Proves

1. M646 remains the best safe scorer in this family.
   M648's best normal grid exactly falls back to M646 with
   `risk_penalty_lambda=0.0`.

2. The risk classifier is not useless, but its scalar penalty is too weak a
   control surface.
   Even extreme lambda values do not change the best Recall/promotion structure.
   The high-risk run only finds a microscopic MAP tie-break.

3. M647's broad coverage signal is not recovered.
   M647 promoted 27 positives across more datasets but displaced 23 positives.
   M648 stays at M646's 13 promoted / 7 displaced shape.  It avoids damage, but
   it also avoids the extra coverage.

4. The current block is feature and decision geometry, not lambda tuning.
   A protected-slot probability over the 96-100 window does not tell the
   reranker which candidate should be allowed to cross the boundary.

## Current Limit

The P1.3 native boundary scorer has two partial but incompatible signals:

| Signal | Strength | Failure |
| --- | --- | --- |
| M646 soft promotion | Safe MAP/Recall balance | concentrated in FiQA |
| M647 hard assignment | broader dataset movement | destructive displacement |
| M648 risk penalty | preserves M646 | does not recover broader movement |

So the block is not simply "avoid displacing protected positives."  The harder
problem is deciding which under-ranked positive has enough counterfactual value
to replace a specific boundary document without dataset-specific tuning.

## Next Breakthrough Point

Do not continue scalar slot-risk lambda search.

The next useful step should be one of:

1. M649 counterfactual displacement audit.
   Extract the actual M646/M647 promoted and displaced rows, then test whether
   existing native features can separate:
   - promoted positive vs displaced positive,
   - promoted positive vs safe negative,
   - destructive swap vs net-positive swap.

2. If separability is weak, stop this row-reranker family and return to the
   first-stage retrieval-constrained generated-posting objective.

3. If separability is strong, train a pairwise counterfactual swap model rather
   than another scalar row score.  The target should be "this candidate may
   replace this specific slot," not "this candidate has a higher global score."

Acceptance for any next scorer:

- Beat M646 promote on Recall@100 and MAP@100.
- Keep NDCG@10 and MRR@20 non-regressed.
- Keep positive gain share below 0.75.
- Avoid dataset Recall regression below -0.001.
- Show movement beyond FiQA/nfcorpus without recreating M647 displacement.

## Conclusion

M648 should be saved as a boundary result, not promoted as a new model.  It
closes the simplest protected-risk hypothesis and narrows the next search:
either prove counterfactual swap separability, or pivot back to changing the
generated posting objective.
