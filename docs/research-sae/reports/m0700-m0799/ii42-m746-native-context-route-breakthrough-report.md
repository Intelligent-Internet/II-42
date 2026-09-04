# M746 Native-Context Route Breakthrough

## Question

After M741/M742/M743 failed or produced only tiny movement, the remaining
question was whether M654's oracle coordinate movement depends on information
missing from coordinate identity and scale features.

M745 tested that directly by exporting native candidate-context features from
each M654 coordinate x scale replay attempt:

- top-k candidate overlap,
- top-k threshold deltas,
- top-k margin deltas,
- top-k mean score deltas,
- global score mean/std deltas.

These features describe the local native rank boundary and do not directly use
qrels metrics as deployable inputs.

## Result

M745 shared4 held-out boundary result:

| Feature set | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| context | 25 | 12 | 2 | +0.000000 | +0.001306 | +0.000940 | +0.000000 | +0.000222 | +0.000000 | 1 |
| context_group | 24 | 11 | 2 | +0.000000 | +0.000178 | +0.000000 | +0.000000 | -0.000084 | +0.000000 | 0 |
| context_teacher | 24 | 10 | 2 | +0.000000 | +0.000050 | +0.000000 | +0.000000 | +0.000317 | +0.000000 | 1 |
| context_teacher_group | 25 | 12 | 0 | +0.000000 | +0.000116 | +0.000000 | +0.000000 | -0.000004 | +0.000000 | 0 |

M654 oracle on the same held-out slice:

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001932 | +0.001103 | +0.000000 | +0.000706 | +0.000000 |

The deployable `context` feature set recovers most of the oracle MAP/NDCG
movement while preserving dense overlap and CUB.

## Interpretation

This is the first positive signal after the M741-M743 failures:

- M741 showed coordinate identity alone cannot pick the right update.
- M742 showed adding scale still cannot reproduce the oracle row.
- M743 showed accepted attempts are weakly learnable, but without boundary
  movement.
- M745 shows that native candidate-context features are the missing interface.

The route should now be reframed as native replay-context selection, not blind
coordinate selection.

## Limits

This is still a small shared4 boundary-slice result.  It is not yet a full
route promotion.

Known limits:

- The result improves MAP/NDCG, not Recall@100.
- The feature export is replay-based and must be assessed for query-time cost.
- It must survive shared8/shared15 native replay before becoming a P1 route.
- The current implementation is a selector over replayed attempts, not yet a
  direct compiler that emits deltas without replay.

## Next Step

Run M747 shared8 native-context replay:

1. Export M654 context scale rows on shared8.
2. Train the same M745 `context` selector on dev boundary rows.
3. Evaluate held-out boundary rows without changing threshold on test.
4. Accept only if MAP or NDCG improves with no O@100, CUB, MRR, or Recall
   regression.
5. If shared8 passes, move to shared15/native and then measure query-time
   replay cost.

Stop if the M745 `context` signal disappears on shared8.
