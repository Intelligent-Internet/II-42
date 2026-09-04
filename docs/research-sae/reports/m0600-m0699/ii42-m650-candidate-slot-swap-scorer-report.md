# M650 / Candidate-Slot Swap Scorer Report

## Status

M650 is a useful negative result with a narrow positive signal.

It converts the M649 counterfactual displacement audit into a trainable
candidate-slot replacement model.  The model is global, uses train split qrels
only, and replays constrained swaps at the top100 boundary.  It is gated
against M646 promote rather than only against the P1.3 baseline.

The result does not beat M646.  It improves MAP beyond M646, but loses macro
Recall@100 because it does not recover the FiQA-heavy gain that M646 found.

## Runs

| Run | Purpose | Status |
| --- | --- | --- |
| `runs/m650_candidate_slot_swap_scorer_bounded_v1` | bounded default check | not promoted |
| `runs/m650_candidate_slot_swap_scorer_recallheavy_v1` | higher safe-positive weight, lower neutral weight | not promoted |
| `runs/m650_candidate_slot_swap_scorer_converged_v1` | bounded default with `max_iter=1000` | not promoted |

The original default attempt was stopped because it spent several minutes in
M646 auxiliary score prediction.  The script now keeps M646 scoring optional
and reads the saved M646 replay JSON as the safety floor by default.  This
removes the expensive dependency while keeping the same promotion gate.

## Best Result

Best run: `runs/m650_candidate_slot_swap_scorer_converged_v1`.

Best config:

| Parameter | Value |
| --- | ---: |
| threshold | 0.0 |
| candidate max rank | 250 |
| max swaps/query | 3 |
| M646 floor gate | below_m646_recall |

Training surface:

| Counter | Value |
| --- | ---: |
| pair count | 80000 |
| safe positive pairs | 4443 |
| destructive pairs | 13300 |
| neutral negative pairs | 62257 |
| train accuracy | 0.797 |

Metrics:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Cand UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1.3-a010 proxy | 0.745306 | 0.688652 | 0.889652 | 0.838500 | 0.993377 |
| M646 promote | 0.745306 | 0.688775 | 0.893508 | 0.838500 | 0.993377 |
| M650 | 0.745306 | 0.688872 | 0.891814 | 0.838500 | 0.993377 |

Deltas:

| Metric | M650 vs P1.3 | M650 vs M646 |
| --- | ---: | ---: |
| NDCG@10 | +0.000000 | +0.000000 |
| MAP@100 | +0.000219 | +0.000096 |
| Recall@100 | +0.002162 | -0.001695 |
| MRR@20 | +0.000000 | +0.000000 |

Promotion counts:

| Counter | M646 promote | M650 converged |
| --- | ---: | ---: |
| under-ranked positives promoted | 13 | 50 |
| top100 positives displaced | 7 | 37 |
| net top100 positives | +6 | +13 |

The raw count improves, but macro Recall does not.  M650 moves positives in the
wrong distribution for the macro objective.

## Dataset Shape

M650 converged vs M646:

| Dataset | dRecall@100 |
| --- | ---: |
| scidocs | +0.015517 |
| trec-covid | +0.005534 |
| dbpedia-entity | +0.000172 |
| nfcorpus | -0.000195 |
| cqadupstack | -0.000726 |
| msmarco | -0.002041 |
| fiqa | -0.052632 |

This explains the failure.  M650 creates broader movement and improves MAP, but
it drops M646's FiQA gain and introduces an MSMARCO regression.  The M646 floor
is not passed.

## What This Proves

1. Pair-local replacement is a real signal.
   M650 promotes more under-ranked positives than M646 and improves MAP over
   both P1.3 and M646.

2. The current train labels are not aligned with macro Recall.
   Safe/destructive replacement labels optimize local replacement correctness,
   but do not preserve the dataset-balanced Recall distribution.

3. M649 separability does not automatically translate into a better scorer.
   The pair feature surface can identify safe replacements, but constrained
   assignment still needs a macro-aware or query-aware objective.

4. This exact M650 family should not be expanded blindly.
   More neutral negatives or larger grids are unlikely to recover the missing
   FiQA behavior; the problem is target shape, not simple model capacity.

## Current Block

The second-stage native boundary line has reached a sharper boundary:

- Row scorers are too weak or too concentrated.
- Hard assignment is too destructive.
- Scalar risk penalty does not help.
- Pair-local swap scoring improves MAP and raw positive count, but loses macro
  Recall versus M646.

The missing piece is macro/query distribution control.  Without richer labels
or an objective that directly encodes dataset-balanced Recall, the native
second-stage swap family is not the next best breakthrough route.

## Next Step

Return to the first-stage retrieval-constrained generated-posting objective,
unless we intentionally introduce a richer second-stage training signal.

The useful retained signal from M650 is:

- candidate-slot pair features can find high-precision replacements,
- M650 may later be useful as a MAP/precision component,
- but it is not the Recall recovery component.

Recommended next stage:

1. Pause this exact M646-M650 boundary-swap family.
2. Start the retrieval-constrained generated-posting objective line.
3. Use M646/M650 as diagnostic teachers:
   - M646 marks high-Recall but concentrated boundary behavior,
   - M650 marks broader high-precision replacement behavior.
4. Train the first-stage generated posting objective to preserve dense/P1
   candidate membership while adding retrieval-constrained positive promotion.

Stop rule for returning to second-stage:

Only return if we have a macro-aware pair target or native query-level objective
that can explicitly preserve FiQA-like recall gains while adding M650's broader
MAP gains.
