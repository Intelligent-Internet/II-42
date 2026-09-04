# M646 / P1.3 Query-Local Boundary-Swap Ranker Report

## Status

M646 is a partial breakthrough, not a promotion-ready model.

It validates one important hypothesis from M645: the useful interface is a
query-local top100 boundary slot, not a broad row-regression or broad
tournament score.  The effective window is very narrow: rank 96-100.  Wider
windows dilute the signal.

The best M646 variant improves both Recall@100 and MAP@100 while keeping
NDCG@10 and MRR@20 flat, but it still fails dominance and the +0.005 Recall
promotion gate.

## Compared Runs

| Run | Slot / scoring | alpha | preserve | window | dRecall@100 | dMAP@100 | promoted | displaced | net | status |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M641 validation | row nonlinear | 0.10 | 95 | 300 | +0.004468 | +0.000131 | 57 | 49 | +8 | breakthrough candidate |
| M644 query-local | linear pairs | 0.15 | 95 | 350 | +0.000552 | +0.000088 | 7 | 2 | +5 | not promoted |
| M645 base | row targets | 0.05 | 98 | 250 | +0.001706 | +0.000086 | 10 | 4 | +6 | not promoted |
| M646 default | 80-140 topk mean | 0.15 | 98 | 300 | +0.000061 | +0.000023 | 4 | 2 | +2 | not promoted |
| M646 slot96 | 96-100 mean | 0.10 | 98 | 350 | +0.004194 | +0.000076 | 4 | 1 | +3 | not promoted |
| M646 slot90-110 | 90-110 mean | 0.12 | 98 | 350 | +0.000202 | +0.000050 | 4 | 1 | +3 | not promoted |
| M646 concat | 96-100 concat | 0.20 | 98 | 300 | +0.000196 | -0.000036 | 10 | 10 | 0 | not promoted |
| M646 promote | 96-100 promote weighted | 0.12 | 95 | 250 | +0.003856 | +0.000123 | 13 | 7 | +6 | dominance limited |

## Best Replay

Best balanced run: `runs/m646_p1p3_query_local_swap_ranker_slot96_promote_v1`.

| Metric | P1.3-a010 proxy | P1.3-a010+M646 | Delta |
| --- | ---: | ---: | ---: |
| NDCG@10 | 0.745306 | 0.745306 | +0.000000 |
| MAP@100 | 0.688652 | 0.688775 | +0.000123 |
| Recall@100 | 0.889652 | 0.893508 | +0.003856 |
| MRR@20 | 0.838500 | 0.838500 | +0.000000 |
| Candidate UB | 0.993377 | 0.993377 | +0.000000 |

Promotion counts:

| Counter | Value |
| --- | ---: |
| baseline positive top100 | 2898 |
| candidate positive top100 | 2904 |
| under-ranked positives promoted | 13 |
| top100 positives displaced | 7 |
| net top100 positives | +6 |
| positive gain efficiency | 0.462 |

Dataset recall deltas:

| Dataset | dRecall@100 |
| --- | ---: |
| fiqa | +0.052632 |
| nfcorpus | +0.006956 |
| cqadupstack | +0.000726 |
| trec-covid | -0.000034 |

The run has no major dataset regression by the current 0.001 threshold, but it
fails dominance because the positive gain is concentrated in FiQA.  Max
positive dataset share is 0.873.

## What M646 Proves

1. The 96-100 boundary slot is a real interface.
   Broad 80-140 scoring produced almost no movement.  Narrow 96-100 scoring
   recovered M641-level recall movement with far lower displacement.

2. Pairwise swap training can improve MAP and Recall together.
   The promote-weighted slot96 variant reaches +0.000123 MAP and +0.003856
   Recall while keeping NDCG/MRR flat.

3. Feature expansion is not the missing piece.
   `diff_abs_concat` regressed MAP and produced zero net top100 positives.
   Adding more pair feature dimensions is not a good next step.

4. The current block is gain distribution and slot policy, not raw capacity.
   M646 can find valuable swaps, but the gain is too concentrated and too few
   queries are moved.

## Current Limits

- Promotion gate is still missed: best Recall delta is +0.003856 to +0.004194,
  below the +0.005 target.
- Dominance fails: FiQA contributes too much of the positive recall gain.
- Default broad opponent tournament is ineffective.
- Wider 90-110 slot loses the signal, so simple window expansion is not a path.
- The current scalar score conversion is still crude: each row gets one
  tournament-derived score, while the true decision is which boundary slot to
  replace.

## Next Breakthrough Point

The next experiment should be M647: slot-specific calibrated boundary policy.

Recommended design:

1. Keep M646's 96-100 interface.
2. Train separate or explicitly conditioned policies for each boundary slot:
   rank 96, 97, 98, 99, and 100.
3. Score a candidate as a replacement for a specific slot, not as a generic row.
4. Build a deterministic per-query assignment step:
   accept only swaps whose predicted margin clears a global threshold and whose
   protected-positive displacement risk is low.
5. Tune only global thresholds on the train split; no dataset-specific tuning.
6. Replay through the same native evaluator.

Acceptance target for M647:

- Recall@100 delta >= +0.005.
- MAP@100 delta remains positive and preferably >= +0.00010.
- NDCG@10 and MRR@20 stay flat.
- No dataset regression below -0.001.
- Max positive dataset share <= 0.75.

Stop condition:

If slot-specific assignment cannot broaden gains beyond FiQA/nfcorpus while
preserving M646's MAP gain, then the current second-stage boundary feature
family is likely exhausted.  At that point, the next meaningful move is to
change the native feature surface or return to first-stage generated-posting
objectives, not further reranker tuning.
