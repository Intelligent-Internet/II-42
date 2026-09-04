# M1620 Adaptive Second-Probe Report

## Executive Decision

**Stop adaptive second-probe controller training.** First-hit document context
contains real candidate signal, but the locked policy remains far below dense
equivalence and recalls only `0.7-2.4%` of the conditional-oracle key actions.
The signal is retained as access-engine evidence, not as a deployable route.

No qrels, BM25, ANN, dense score, or dataset identity generated the adaptive
actions. Dense top256 was used only for the frozen oracle and evaluation.

## Evaluator Correction

The first M1620 run exposed a flaw inherited from M1600/M1610:
`overlap_indices` used `min(K, returned_count)` as its denominator. A route
returning only 44 candidates could therefore report an apparently high O@256,
and adding candidates could make that value fall even when the candidate set
was a superset.

M1620 v2 uses a fixed dense-target denominator. Underfilled rankings are now
penalized. The invalid v1 result is preserved remotely but is not used for any
decision.

## Locked Surface

- M1610B 8,192-key initial checkpoint;
- 250 heldout MS MARCO-derived queries and 2,250 documents;
- direct qTopK=2 seed route;
- first probe capped at `0.03x` reads;
- at most 128 added keys from the top8 or top32 first-hit documents;
- reciprocal-rank or normalized-score evidence, multiplied by document impact
  and IDF;
- exact total read caps of `0.075x` and `0.15x`.

## Strict Result

| Policy at 0.15x | O@10 | O@100 | O@256 | Reads | Oracle action recall |
| --- | ---: | ---: | ---: | ---: | ---: |
| direct qTopK=2 | 0.762800 | 0.236960 | 0.130109 | 0.026206x | - |
| rank-IDF, top8 | 0.894400 | 0.471080 | 0.317656 | 0.126798x | 0.007285 |
| rank-IDF, top32 | 0.900800 | 0.503600 | 0.344016 | 0.145844x | 0.023645 |
| score-IDF, top8 | 0.897200 | 0.470840 | 0.317641 | 0.127127x | 0.007285 |
| score-IDF, top32 | 0.903600 | 0.506080 | 0.347687 | 0.145806x | 0.023668 |
| conditional oracle | 1.000000 | 0.999280 | 0.877422 | 0.149435x | 1.000000 |

The best observable policy improves O@100 by `0.269120` and O@256 by
`0.217578`. That closes 35.3% and 29.1% of the conditional-oracle gaps. This
is a real query-conditioned bridge signal, not the static co-occurrence result
from M1610B.

It is nevertheless not competitive. The best policy reads `0.145806x` for
O@100 `0.506080` and O@256 `0.347687`. M1565 reaches O@100 `0.906235` and
O@256 `0.842683` at only `0.045015x` reads.

## Why The Gate Fails

1. The policy retrieves broad semantic neighbours of first-hit documents, but
   those keys have only `0.0237` mean recall of the oracle action set.
2. First-probe evidence therefore identifies useful proxy regions, not the
   specific diverse keys required for the dense tail.
3. Increasing evidence from top8 to top32 provides only a small incremental
   gain while nearly filling the entire `0.15x` budget.
4. A learned controller would only reorder the same weak action source. M1230
   previously found that atom-document context did not improve target/harm
   separation, while M1317-M1322 found non-generalizing two-stage policy
   gains. Training another selector is not justified by this surface.

## Retained Conclusion

The first-hit context signal matters, but not enough to solve candidate access.
It supports query-conditioned index traversal as an engineering primitive; it
does not support a new semantic-key controller over M1610B.

The more durable redesign is to stop making the encoder predict a tiny set of
corpus-specific access actions. Preserve a wider, faithful sparse score and
move efficiency into an exact or safely bounded learned-sparse retrieval
engine.

## Reproducibility

- Host: `spark-1`; `spark-2` was not touched.
- Container: `nvcr.io/nvidia/pytorch:26.03-py3`.
- Root: `/home/huoju/leask/runs/ii42-m1620-adaptive-second-probe-v1`.
- Valid run: `m1620a-v2-fixed-depth-seed1611`.
- Summary SHA-256:
  `a2c397ca6d34e32c1bb441d96e25fbf2ef0dae4b7547eb800fb43c748630bd9c`.
