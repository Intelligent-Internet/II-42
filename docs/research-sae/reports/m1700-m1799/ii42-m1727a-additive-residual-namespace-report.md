# M1727A Frozen-Base Additive Residual Namespace Report

## Decision

**The additive same-source capacity hypothesis passes, while the predeclared
direct-observability gate narrowly fails. Do not train a residual selector.**
The next authorized experiment is a native score-representability audit over
the frozen expanded candidate set.

The formal run is v2. The initial v1 filled every individually cheap query to
the read cap while retaining already-expensive base queries, producing mean
reads `0.181170x`. It also omitted the observed-policy read check from the
decision. That accounting/gate bug was fixed before interpretation; v1 is
excluded and no quality parameter changed.

## Frozen Full Surface

- M1600 1,000-query/8,988-document validation pool;
- M1600 8x512 source, one frozen document key and eight frozen query probes
  per group;
- frozen exact BM25 top256;
- fixed incremental allowance derived from `0.18 - base mean reads`;
- no training, qrels, dataset identity, threshold search, or score tuning;
- exact dense reranking used only as the candidate-admission upper bound.

## Result

| Policy | O@100 | O@256 | R@100 | R@256 | Reads | Extra reads | Extra keys |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Frozen base | 0.865390 | 0.745742 | 0.811074 | 0.679794 | 0.146086x | 0.000000x | 0.000 |
| Residual capacity oracle | 0.997640 | 0.913016 | 0.997294 | 0.896344 | 0.176676x | 0.030589x | 28.394 |
| Frozen-logit expansion | 0.889080 | 0.785539 | 0.845210 | 0.731473 | 0.179909x | 0.033823x | 17.561 |

Frozen-logit expansion versus base gains:

- O@100 `+0.023690`;
- O@256 `+0.039797`;
- R@100 `+0.034137`;
- R@256 `+0.051679`.

This is the first full-scale result in the M1722-M1727 sequence that improves
all four candidate metrics while leaving every base key and document
assignment unchanged and satisfying the read and max-DF limits.

## Why The Formal Gate Still Fails

The oracle gains are much larger: `+0.132250/+0.167273` O@100/O@256. Frozen
logits capture:

- `17.91%` of oracle O@100 gain, below the fixed 20% requirement;
- `23.79%` of oracle O@256 gain, above the requirement.

Oracle-key AUC under frozen query logits is `0.715140`, but matched-count
recall is only `0.031032`, below the `0.10` router-training gate. The logits
contain broad signal but do not reproduce the corpus-aware greedy action.

The formal decision is therefore
`stop_additive_same_source_unobservable`. It closes residual-selector
training, not the measured frozen expansion.

## Interpretation

M1726 and M1727 isolate the previous contradiction:

- changing the shared source under residual supervision loses full-scale base
  geometry;
- leaving the source frozen and adding the next observable keys produces
  useful candidate coverage immediately;
- the remaining gap is no longer candidate capacity or training depth;
- it is whether one native posting-decomposable score can rank the expanded
  candidates without exact dense reranking.

The frozen-logit result is strong enough to justify that scorer audit even
though it misses the relative oracle-capture threshold. It is not strong
enough to justify another query router: the direct logits already provide the
deployable signal, while previous router families repeatedly destroyed hard
heldout quality.

## Next Gate

Freeze the v2 candidate policy and compare:

1. semantic key-hit count;
2. query-logit-weighted semantic posting score;
3. a global qrels-free linear combination of semantic posting and BM25
   contributions trained only to preserve dense score/order.

Every scorer must decompose into one posting accumulator. No rank-conditioned
controller, exact dense rerank, per-query candidate normalization, or dataset
tuning is allowed. If no native scorer preserves at least half of the v2
O@100/O@256 gains without lowering base O@10, stop this expansion before model
training.

## Reproducibility

- host: `spark-1`;
- formal run:
  `/home/huoju/leask/runs/ii42-m1727-additive-residual-v1/runs/m1727a-additive-residual-full-seed1601-v2`;
- excluded gate-bug run: corresponding `v1` directory;
- summary SHA-256:
  `2b4cbeec0cc11e82d24fa816a5a521993fb899ac0bce5e13bbd90e4897dcc087`;
- local result: `ii42-m1727a-additive-residual-namespace-result.json`.
