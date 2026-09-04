# M788 Generated Delta Frontier

This diagnostic sweeps threshold, topN, and delta scale for the
M787 interaction-supervised generated-delta family. It is not a
promotion run; it checks whether the family can cross Recall@100
boundaries before breaking dense-equivalence gates.

## Summary

| Model | Rows | Clean | Recall+ | Clean Recall+ | Best Clean Utility | Best Clean dMAP | Best Clean dRecall | Best Any dRecall |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| logistic | 96 | 20 | 0 | 0 | +0.000112 | +0.000019 | +0.000000 | n/a |
| hgb | 96 | 16 | 4 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000019 |

## Decision

M788 found boundary movement only in unsafe settings. The signal is not dead, but row-mixed deltas need hard constrained generation rather than threshold policy tuning.
