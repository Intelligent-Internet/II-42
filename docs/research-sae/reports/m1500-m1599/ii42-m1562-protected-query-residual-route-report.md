# M1562 Protected Query-Residual Route Report

## Decision

**Stop query-bipartite route supervision.**

Protecting both centroid routes recovers part of M1561's transductive
capacity, but does not make the query-derived route stable across held-out
queries.  The source fails before any encoder training, so additional epochs,
losses, or model capacity are not justified.

## Surface

- NFCorpus, 2,063 documents and 100 queries.
- Three deterministic 50/50 query splits.
- Equal-cost triple comparison: centroid top3 versus centroid top2 plus one
  query-derived residual route.
- Fixed 15% unique candidate union; all posting reads counted.
- Exact dense reranking over touched candidates.
- ClearML task: `356c34b2027f489997d686ecb3c352a8`.
- M1542 centroid-dual parity delta: `0.0`.

## Macro Result

| Source | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | Reads |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| centroid dual | 0.858000 | 0.646000 | 0.487891 | 0.335445 | 0.427787 | 0.1729x |
| centroid triple | 0.846000 | 0.635867 | 0.472292 | 0.346630 | 0.420613 | 0.1773x |
| train protected triple | 0.826000 | 0.610600 | 0.455651 | 0.334790 | 0.422460 | 0.1711x |
| all-query protected triple | 0.934000 | 0.681333 | 0.502682 | 0.352622 | 0.429304 | 0.1749x |

Relative to the equal-cost centroid triple, the all-query diagnostic gains:

- O@10: `+0.088000`;
- O@100: `+0.045467`;
- O@256: `+0.030391`;
- oracle O@100: `+0.042467`.

This confirms that preserving the two semantic edges was necessary: the
transductive O@100 gain is almost twice M1561's `+0.0242`.  It remains below
the predeclared `+0.05` capacity floor and, more importantly, does not
generalize.

## Held-Out Transfer

Train-only O@100 deltas versus centroid triple are `-0.0260`, `-0.0260`, and
`-0.0238`, for a mean of `-0.025267`.  Mean O@10 and O@256 also fall by
`-0.020000` and `-0.016641`.

The train-only source fits the construction side but loses direct held-out
route-edge coverage@4 by `-0.0302`.  Keeping the semantic base prevents the
larger M1561 collapse, but it cannot turn the residual route into a stable
cross-query code.

Centroid top3 is itself worse than centroid dual on dense overlap at the same
15% unique union.  More document edges produce larger route lists and fewer
probes before the union budget fills; fanout is not free even when storage and
reads remain inside the accounting cap.

## Conclusion

The M1561/M1562 sequence separates three facts:

1. query-document dense edges can memorize head access;
2. replacing semantic routes was harmful, and protection partially fixes it;
3. query-specific route labels do not transfer across queries and therefore
   are not a valid posting vocabulary teacher.

Do not continue with query calibration, synthetic-query scale, teacher-depth
sweeps, or a text-to-route compiler on this label source.  The next route must
derive a corpus-stable factorization whose posting codes preserve multiple
dense directions without depending on which queries happened to supervise a
document.
