# M751 Native-Context Route Summary

## Summary

The new route produced a real positive signal after the coordinate-selector
dead end.

The key discovery is that M654's coordinate oracle was not learnable from
coordinate identity, scale, or simple coordinate statistics.  It became
learnable only after the selector saw native candidate-context features around
the rank boundary.

This changes the route definition:

- Not blind first-stage text-to-coordinate selection.
- Not a conventional BM25/reranker patch.
- Yes: query-time native-context accepted-region selection over replayed
  unified-posting deltas.

## Evidence Chain

| Step | Result | Decision |
| --- | --- | --- |
| M741 coordinate selector | high AUC, zero held-out best-row hits | stop coordinate-only |
| M742 scale-aware selector | zero held-out accepted hits | stop exact row imitation |
| M743 accepted-region selector | tiny safe signal only | insufficient |
| M745 shared4 context selector | dMAP +0.001306, dNDCG +0.000940, O safe | positive |
| M747 shared8 context selector | dMAP +0.001046, dNDCG +0.000769, dMRR +0.001020, O safe | broader positive |
| M749 shared15 context selector | gains but O@100 -0.000149 | needs support filter |
| M750 shared15 O100 filter | dMAP +0.000712, dNDCG +0.000239, dCUB +0.000693, O safe | deployable conservative candidate |
| M750 shared15 O128p99 filter | dMAP +0.000285, dNDCG +0.001906, dMRR +0.007463, O safe | aggressive quality candidate |

## Current Best Candidates

### M750 O100 Filter

Use when preserving native top100 support is the main concern.

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.000712 | +0.000239 | +0.000000 | +0.000693 | +0.000000 |

### M750 O128p99 Filter

Use when NDCG/MRR movement is more important and slight top128 churn is
acceptable.

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.000285 | +0.001906 | +0.007463 | +0.000091 | +0.000000 |

## Interpretation

This is a meaningful breakthrough, but not yet a P1 replacement.

What is proved:

- The missing interface was native boundary context.
- The accepted-region classifier can generalize from dev to held-out boundary
  rows on shared4, shared8, and shared15-root boundary slices.
- A support-overlap filter fixes the dense-overlap regression observed by the
  unfiltered shared15 context selector.

What is not yet proved:

- Full native matrix improvement.
- Query-time cost feasibility.
- Direct compiler replacement without replay.
- Recall@100 improvement; current gains are MAP/NDCG/MRR/CUB.

## Next Stage

M752 should convert this into a more engineering-realistic native replay policy:

1. Freeze M750 O100 and O128p99 filters as two candidates.
2. Run native policy replay on a fuller query sample, not only boundary rows.
3. Report per-dataset metrics and macro deltas.
4. Measure average replay attempts per query and wall time.
5. If cost is too high, train a cheap pre-selector to reduce replay attempts
   before applying the M750 context selector.

Stop if full native replay loses the strict gate or gains disappear outside the
boundary slice.
