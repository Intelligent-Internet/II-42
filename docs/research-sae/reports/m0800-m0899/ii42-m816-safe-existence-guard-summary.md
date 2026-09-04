# M816 Safe-Existence Guard Summary

M816 tested whether the M815 failure came from missing query-level abstention:
the pairwise model can rank oracle-like bundles, but it still applies unsafe
top bundles on held-out surfaces.

## M816A: Safe-Existence Separability

M816A predicts whether a query has any safe-positive generated bundle using
only aggregate, inference-available candidate features.  A leakage feature
from the first draft was removed before accepting the result.

No-leak HGB leave-surface-out result:

| Held-out | AUC | AP | P@10% |
| --- | ---: | ---: | ---: |
| original | 0.8762 | 0.6771 | 0.789 |
| seed7642 | 0.8754 | 0.7144 | 0.842 |
| seed7643 | 0.8761 | 0.7373 | 0.750 |

Conclusion: safe-existence is real and learnable.  This is the first strong
signal explaining why M815C partially worked but still selected unsafe rows.

## M816B: Existence-Guarded Replay

M816B combines:

- M815 pairwise top-bundle ranker.
- M815 top-bundle abstainer.
- M816 query-level safe-existence guard.

The default product combination improves utility but remains non-clean:

| Held-out | Applied | Utility | Clean | Negative Tasks |
| --- | ---: | ---: | ---: | --- |
| original | 40 | +0.000516 | 0 | nfcorpus, trec-covid, webis-touche2020 |
| seed7642 | 16 | +0.000066 | 0 | dbpedia-entity, trec-covid |
| seed7643 | 22 | +0.000036 | 0 | nfcorpus, trec-covid |

Held-out oracle threshold sweep shows clean thresholds exist on every surface:

| Held-out | Clean Threshold Exists | Oracle Threshold | Oracle Utility |
| --- | ---: | ---: | ---: |
| original | 1 | 0.4626 | +0.000147 |
| seed7642 | 1 | 0.2945 | +0.000080 |
| seed7643 | 1 | 0.6531 | +0.000000 |

This means the probability score is not useless; the failure is conservative
calibration and top-bundle safety ordering, especially on nfcorpus-like rows.

## Conservative Floor Sweep

| Floor | All Clean | Applied | Sum Utility |
| ---: | ---: | ---: | ---: |
| 0.50 | 0 | 14 | +0.000094 |
| 0.55 | 0 | 9 | +0.000085 |
| 0.60 | 1 | 4 | +0.000006 |
| 0.65 | 1 | 2 | +0.000000 |

The clean deployable floor exists but is too conservative to be a useful
breakthrough.  The useful non-clean floors are mostly blocked by seed7643
nfcorpus.

## Decision

Keep M816 safe-existence guard as a real component.  Do not claim M816B as a
deployable selector yet.

The next useful step is not more threshold swapping.  The remaining bottleneck
is task/query-local harmful top-bundle selection.  A next probe should focus on
why nfcorpus/trec-covid style rows pass the existence guard but fail top-bundle
safety:

- Add group-vs-top disagreement features: existence probability high, top
  safety probability low, and top score gap/margin shape.
- Add conservative harmful-task diagnostics without hard-coding task identity.
- Train a risk-first veto for the selected top bundle, evaluated by held-out
  native replay.

Stop condition for the selector route: if a risk-first veto cannot keep the
0.50-0.55 utility while removing seed7643 nfcorpus failures, generated-bundle
selection should be paused and the proposal/top-bundle teacher must be rebuilt.
