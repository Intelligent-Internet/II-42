# M630 / P1.4 Score Geometry Audit

Status: smoke audit completed; M630-A is useful diagnostic evidence, not a
promoted model.

## Scope

M630 tests whether the current P1 native posting score geometry is the real
bottleneck.  It freezes the P1 candidate pool and aligns P1 top1000 candidates
against dense teacher top100 rankings.  Dense rank is used only as training and
audit target.  BM25, fused rank, dataset id, query id, and doc id are not model
features for M630-B.

The smoke surface is:

- `nfcorpus`
- `scifact`
- `trec-covid`

Input/output:

- Audit dataset: `runs/m630_p1p4_score_geometry_audit_smoke_v1/m630_p1p4_score_geometry_dataset.jsonl`
- Audit JSON: `runs/m630_p1p4_score_geometry_audit_smoke_v1/m630_p1p4_score_geometry_audit.json`
- Audit MD: `runs/m630_p1p4_score_geometry_audit_smoke_v1/m630_p1p4_score_geometry_audit.md`

## Audit Matrix

| Dataset | Queries | Dense top100 in P1 top1000 | Dense top100 in P1 top100 | Rank corr | Pairwise agreement | P1-vs-dense KL |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 100 | 1.000000 | 0.926900 | 0.926194 | 0.907820 | 1.158151 |
| `scifact` | 100 | 1.000000 | 0.930500 | 0.936969 | 0.911810 | 1.761625 |
| `trec-covid` | 50 | 1.000000 | 0.943200 | 0.957538 | 0.930974 | 0.760101 |
| `macro` | 250 | 1.000000 | 0.931600 | 0.936773 | 0.914047 | 1.319931 |

## Interpretation

The smoke audit shows candidate generation is not the immediate issue on this
surface: dense teacher top100 is fully present inside P1 top1000.  The mismatch
is score geometry and top100 membership: roughly 6.84% of dense top100 support
is present in P1 top1000 but not in P1 top100.

This makes M630-B a valid next probe: if a first-stage scorer can recover the
dense-like ordering without BM25 or qrels, it should improve Recall@100 or at
least preserve top metrics while improving dense overlap and KL.

## Audit Fix

The first M630-A builder version emitted only P1 candidate rows.  That would
make downstream Recall@100 denominators incorrect when qrels positives are
missing from P1 top1000.  The builder now writes per-query
`qrel_count_total` and `qrel_ideal_dcg_10` onto every candidate row, computed
from the full M604 gap row set.  Training still sees only P1 candidates, but
evaluation can use the correct qrels denominator.

## Decision

M630-A is retained as a useful audit dataset builder and should be reused for
future score-geometry probes.  It justifies M630-B smoke testing, but it does
not by itself justify broad shared15 expansion.
