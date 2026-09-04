# M1630 Literature And Local Evidence Matrix

## Decision Boundary

M1630 is not authorized by one paper or one oracle. It proceeds only where a
published mechanism and a local result identify the same missing component.
The arXiv corpus cutoff used for this review is 2026-07-10.

## Evidence Matrix

| Evidence | Established result | M1630 consequence |
| --- | --- | --- |
| M1541 | Signed dense-root postings recover near-dense quality after broad accumulation; bounded edge-first access fails | Preserve score information; replace access engine |
| M1544 | Approximate coordinate-centroid blocks lose candidates and require excessive summary scans | Do not use approximate centroid admission |
| M1015 | Membership-aware block64 bounds preserve exact top-k; FiQA decoded/full 0.320 | Reuse safe per-lane membership bounds |
| M1600-M1620 | Sparse covers exist, but query-local routers cannot select useful tail keys | Do not force 4-8 query routes |
| Wacky Weights (`2110.11540`) | Learned impacts reduce WAND/DaaT skipping opportunities; SaaT becomes competitive | Measure score distribution and both metadata/forward work |
| Seismic (`2404.18812`) | Geometric blocks plus summaries and a forward index exploit concentration of importance | Keep forward finalization inside one sparse index |
| BMP (`2405.01117`) | Block upper bounds support safe termination; document ordering tightens bounds | Rank-safe block64 first, ordering only after slack audit |
| Superblock Pruning (`2504.17045`, `2602.02883`) | Two-level bounds can reduce block-bound overhead; robust zero-shot settings exist | Permit one two-level repair after block64 failure anatomy |
| Unified LSR Framework (`2303.13416`) | Document weighting is the largest effectiveness component; query weighting is secondary | Train document impacts before query expansion |
| Li-LSR (`2505.01452`) | Relaxed sparsity improves quality/OOD generalization when paired with a modern engine | Do not pre-impose tiny query/document supports |
| Dense2Sparse PEC (`2402.17535`) | A frozen dense model plus sparse projection can work; uncontrolled expansion causes co-activation | Use dense-root warm start and explicit co-activation diagnostics |
| Static Pruning (`2304.12702`) | Later static pruning can trade small quality loss for speed | Defer until an exact quality baseline exists |

## Stable Mechanism

The literature does not support another small query router. It supports a
representation-engine co-design:

1. represent enough semantic evidence to retain the teacher score;
2. store signed impacts as non-negative same-sign posting channels;
3. calculate safe block upper bounds from membership and per-block maxima;
4. finalize exact sparse scores from the same index's forward payload;
5. optimize sparsity or document ordering only after measuring bound slack.

The first M1630A surface therefore uses the frozen M1541 representation and no
training. If that representation cannot achieve both quality and safe pruning,
the result will identify whether the next repair belongs to representation or
engine rather than hiding both inside one loss.

## Prohibited Interpretations

- A safe exact engine cannot repair a weak sparse score; quality and access are
  separate gates.
- A compact forward sparse payload is not an ANN side index, but a dense vector
  search field would violate the branch contract.
- Seismic's approximate result does not authorize approximate centroid blocks
  after M1544.
- A lower loss, tighter average bound, or faster underfilled result is not a
  pass without exact top100 parity and fixed-depth overlap.
