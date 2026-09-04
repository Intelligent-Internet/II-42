# M647 / P1.3 Slot Assignment Ranker Report

## Status

M647 is a negative result with one useful diagnostic signal.

It implemented the M646 follow-up exactly: keep the 96-100 boundary slot, train a
slot-conditioned pair classifier, and make deterministic candidate-to-slot
assignments with a global margin threshold.  This broadened gains across more
datasets, but it did not improve the macro result.  Direct hard assignment
causes too much protected-positive displacement.

## Best Replay

Run: `runs/m647_p1p3_slot_assignment_ranker_v1`.

| Metric | P1.3-a010 proxy | P1.3-a010+M647 | Delta |
| --- | ---: | ---: | ---: |
| NDCG@10 | 0.745306 | 0.745306 | +0.000000 |
| MAP@100 | 0.688652 | 0.688671 | +0.000019 |
| Recall@100 | 0.889652 | 0.890174 | +0.000522 |
| MRR@20 | 0.838500 | 0.838500 | +0.000000 |
| Candidate UB | 0.993377 | 0.993377 | +0.000000 |

Best config:

| Parameter | Value |
| --- | ---: |
| slot range | 96-100 |
| candidate max rank | 350 |
| assignment threshold | 2.0 |
| max swaps per query | 5 |

Promotion counts:

| Counter | Value |
| --- | ---: |
| baseline positive top100 | 2898 |
| candidate positive top100 | 2902 |
| under-ranked positives promoted | 27 |
| top100 positives displaced | 23 |
| net top100 positives | +4 |
| positive gain efficiency | 0.148 |

Dataset recall deltas:

| Dataset | dRecall@100 |
| --- | ---: |
| dbpedia-entity | +0.003559 |
| msmarco | +0.005102 |
| nfcorpus | +0.005779 |
| trec-covid | +0.004950 |
| cqadupstack | -0.003920 |
| scidocs | -0.001724 |

## Comparison

| Run | dRecall@100 | dMAP@100 | promoted | displaced | net | max positive share | major regressions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M641 validation | +0.004468 | +0.000131 | 57 | 49 | +8 | 0.784 | msmarco |
| M646 promote | +0.003856 | +0.000123 | 13 | 7 | +6 | 0.873 | none |
| M647 assignment | +0.000522 | +0.000019 | 27 | 23 | +4 | 0.298 | cqadupstack, scidocs |

## What This Proves

1. Hard assignment broadens gain distribution.
   M646 promote was concentrated in FiQA and nfcorpus.  M647's positive share is
   only 0.298, and it moves msmarco/dbpedia/trec-covid too.

2. Hard assignment is too destructive.
   M647 promotes 27 under-ranked positives but displaces 23 existing top100
   positives.  Positive gain efficiency falls to 0.148.

3. The useful signal should stay soft.
   M646's soft slot score gave much better MAP/Recall balance.  M647's direct
   swap threshold loses that balance, even with a high threshold.

4. The next breakthrough is not another assignment threshold.
   The grid already tried thresholds from -1.0 to 2.0 and max swaps from 1 to 5.
   Lower thresholds increased displacement and often made MAP negative.  Higher
   thresholds became too conservative.

## Current Block

The core block is protected-positive risk estimation.  The model can identify
some useful replacement candidates, but it cannot reliably tell when the slot
occupant is a relevant document that should not be displaced.

This is different from M646's block.  M646 was too concentrated but relatively
safe.  M647 is broader but unsafe.

## Next Breakthrough Point

M648 should combine the two partial signals instead of replacing one with the
other:

1. Use M646-style soft slot score as the promotion signal.
2. Add an explicit protected-slot risk penalty learned from M647 pairs.
3. Keep replay as row reranking, not hard assignment.
4. Penalize candidates only when their best replacement slot looks
   protected-positive-like.
5. Gate on:
   - Recall@100 >= M646 promote, ideally >= +0.005.
   - MAP@100 stays >= +0.00010.
   - Positive gain share <= 0.75.
   - No dataset regression below -0.001.

Stop condition:

If a soft promotion score plus protected-slot risk penalty cannot beat M646
promote, then this second-stage native-boundary scorer family is likely near
its limit.  The next useful move would be changing the native feature surface or
returning to first-stage generated-posting objectives.
