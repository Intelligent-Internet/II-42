# ii42 M1110-M1117 Scoring Geometry Summary

## Verdict

M1110-M1117 found a real scoring-geometry signal, but not yet a scalable
breakthrough.

The useful retained facts are:

- M1110 confirmed the current `additive_atom_0.5` profile is a strong fixed
  baseline on the exported heldout surface.
- M1111 showed large query-level oracle headroom between
  `additive_atom_0.5` and `lex_residual_atom_0.75`.
- M1112/M1113 showed static query score-distribution selectors are too weak.
- M1114 showed the oracle is mostly rank-quality movement, not simple
  top100 relevant promotion.
- M1115 showed rank-geometry trace features can recover a small local gain.
- M1116 showed the M1115 selector is not robust enough under
  leave-dataset-out pressure.
- M1117 showed a fixed continuous base/alternate blend is safer than binary
  selection, but the gain is still below the promotion threshold.

## Main Evidence

### M1110 fixed profile replay

Heldout `additive_atom_0.5` versus lexical:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.025554 |
| MRR@20 | +0.008237 |
| NDCG@10 | +0.010283 |
| MAP@100 | +0.007204 |

`lex_residual_atom_0.75` had more Recall/MRR but weaker NDCG/MAP tradeoff.

### M1111 profile oracle

Oracle over base/alternate on heldout:

| Metric | Delta vs base |
| --- | ---: |
| Recall@100 | +0.010316 |
| MRR@20 | +0.022390 |
| NDCG@10 | +0.017346 |
| MAP@100 | +0.018486 |

This is large enough to justify studying profile choice, but it does not by
itself prove deployability.

### M1114 boundary trace

Heldout movement showed the alternate profile did not simply add more
relevant documents into top100:

| Movement | Value |
| --- | ---: |
| positive promotions | 9 |
| positive demotions | 24 |
| top100 relevant delta | -15 |
| top10 relevant delta | -11 |
| alt win with recall gain rate | 0.018373 |
| alt win rank-only rate | 0.091864 |

The oracle is primarily rank-quality movement. A pure top100-boundary
crossing objective is the wrong abstraction for this sub-route.

Deployable trace features were more separable than earlier static features:

| Feature | Heldout AUC |
| --- | ---: |
| union top score delta std | 0.724119 |
| demoted margin mean | 0.719729 |
| top100 churn | 0.685279 |

### M1115 rank-geometry selector

Heldout selector versus base:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.006606 |
| MRR@20 | +0.004146 |
| NDCG@10 | +0.002214 |
| MAP@100 | +0.003627 |

This is the first selector-style positive result in this segment, but it is
not enough to scale without robustness checks.

### M1116 leave-dataset-out

LODO macro remained positive:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.009169 |
| MRR@20 | +0.006629 |
| NDCG@10 | +0.002092 |
| MAP@100 | +0.004101 |

But per-dataset MAP/NDCG did not stay clean:

| Dataset | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: |
| fiqa | -0.000966 | -0.000343 |
| nfcorpus | +0.000805 | -0.000667 |
| scifact | +0.006439 | +0.013314 |

This fails the stability gate. Treat M1115 as a local signal, not a scalable
policy.

### M1117 fixed base/alternate blend

Train-selected blend weight `0.35` on heldout:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.007334 |
| MRR@20 | +0.002998 |
| NDCG@10 | +0.002495 |
| MAP@100 | +0.000859 |

Best heldout grid point was near `0.40`:

| Metric | Value |
| --- | ---: |
| Recall@100 | 0.521640 |
| MRR@20 | 0.404328 |
| NDCG@10 | 0.338365 |
| MAP@100 | 0.267319 |

This is safer than binary switching, but the train-selected MAP gain is below
the required threshold. It should not be promoted yet.

## Route Decision

Do not continue static query-feature selector tuning. M1112/M1113 already
showed that interface is too weak.

Do not promote M1115 directly. It passed local heldout but failed robustness.

Keep M1117 fixed blend as a low-risk candidate scoring-geometry probe, not a
milestone. It is simple, global, and does not require a trained selector, but
the gain is still too small.

The next useful step is not another selector variant. It is a broader replay
of fixed scoring geometry on a larger export/native surface:

1. Generate candidate-score exports for a broader shared15/native surface.
2. Replay fixed baseline, M1115 rank-geometry selector, and M1117 blend.
3. Promote only if the fixed blend or a selector keeps per-dataset floors on
   broader validation.
4. If broader replay does not hold, stop profile switching and return to the
   underlying atom/posting score construction.

## Current Default

Keep `additive_atom_0.5` as the frozen scoring baseline.

M1115 and M1117 are retained as evidence-bearing probes, not defaults.
