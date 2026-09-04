# M654S Ridge-Teacher Transfer

Status: `not promoted`

M654S continues the first-stage dense-equivalence line.  It does not use BM25,
rerankers, learned gates, or qrels-driven training.  Qrels are only used for
held-out metric reporting.

The goal was to test the M655 roadmap hypothesis: M654R may have failed because
the teacher transfer surface was too small and because the support gate used an
absolute floor that was not valid on all-query surfaces.

## Implementation Changes

Updated `scripts/train_m654r_ridge_teacher_compiler.py` to support M654S:

- source-specific support metrics:
  - `p1_native` support is now computed from P1 query postings versus dense
    query roots;
  - candidate support is computed from generated query postings versus dense
    query roots.
- relative support gate:
  - active support must not regress versus P1;
  - support cosine must not regress versus P1, with a small numeric tolerance.
- parameterized report/schema/source names so M654S artifacts do not overwrite
  M654R artifacts.

This also means earlier M654R reports are still useful for ranking/overlap
trends, but their zero support deltas should not be treated as authoritative.
M654S is the first ridge-transfer run with correct source-specific support
attribution.

## Runs

All canary runs used:

- datasets: `fiqa,arguana,scidocs,cqadupstack`;
- query source: all available local shared15 queries for those datasets;
- split: `320 train / 40 dev / 40 test`;
- architecture: `p1_support_residual + active-lock`;
- teacher: ridge `0.1`;
- support gate: relative, tolerance `1e-5`.

### Scale 0.01

Run: `m654s_canary_ridge01_s001_seed6543`

Result: no dev checkpoint passed.  Loss decreased from `0.096874` to
`0.044456`, but every trained checkpoint either lost Recall@100, support
cosine, or O@256.

Typical dev shape:

- O@100 improved;
- O@256 was unstable;
- Recall@100 dropped by `-0.00297619`;
- support cosine regressed increasingly as training continued.

Decision: reject.  This scale is too strong for first-stage dense preservation.

### Scale 0.001

Run: `m654s_canary_ridge01_s0001_seed6543`

This is the best signal in M654S.

Dev selected epoch `2` passed all dense-equivalence checks:

| Metric | Dev delta |
| --- | ---: |
| O@100 | `+0.000250000` |
| O@256 | `+0.000061035` |
| CUB | `+0.000000000` |
| Recall@100 | `+0.000000000` |
| support cosine | `-0.000000581` |
| MAP@100 | `-0.000010823` |

Held-out test failed only O@100:

| Metric | Test delta |
| --- | ---: |
| O@100 | `-0.000227273` |
| O@256 | `+0.000088778` |
| CUB | `+0.000000000` |
| Recall@100 | `+0.000000000` |
| support cosine | `-0.000000685` |
| MAP@100 | `+0.000004045` |
| NDCG@10 | `+0.000000000` |
| MRR@20 | `+0.000000000` |

Decision: keep as a positive diagnostic, but do not promote.  It shows a narrow
movement window exists, but dev-safe movement did not fully generalize to the
held-out O@100 gate.

### Scale 0.0005

Run: `m654s_canary_ridge01_s00005_seed6543`

Result: no dev checkpoint passed.  Early epochs preserved Recall/support, but
O@256 dipped by `-0.000097656`.  Later epochs crossed Recall boundary loss.

Decision: reject.  It is not simply "use a smaller delta".

## Interpretation

M654S refines the bottleneck:

1. The implementation now has a trustworthy relative support gate.
2. A larger all-query canary still does not produce a promotable checkpoint.
3. The best run, scale `0.001`, demonstrates safe movement on dev and O@256
   improvement on test, but loses a small amount of O@100 on test.
4. Stronger movement quickly spends Recall/support; weaker movement cannot
   reliably preserve O@256.

This means M654R did not fail only because the support floor was mismatched or
because the split was too small.  The current global P1-support residual
compiler still lacks a reliable way to choose boundary-safe movements.

## Decision

Do not promote M654S.

Keep the code changes because they fix evaluation attribution and enable future
relative-gated first-stage tests.

Do not continue blind scale sweeps.  The observed window is too narrow, and the
failure flips between O@100, O@256, support cosine, and Recall depending on
scale.

## Next Step

The next first-stage probe should be boundary-aware, not just larger or deeper.

Recommended M654T:

1. Build a per-query boundary ledger from M654S trace rows:
   - which dense top100 docs are lost;
   - which dense top256 docs are gained/lost;
   - score margins around P1 top100/top256 boundaries;
   - support cosine delta and active support delta.
2. Train or select against a teacher that is explicitly constrained by boundary
   no-loss:
   - preserve dense top100 membership first;
   - then improve O@256;
   - reject teacher deltas that repair tail at the cost of top100.
3. Keep query-side only for one more isolated proof.
4. Stop query-side-only if the boundary ledger shows O@100-safe and
   O@256-positive movements are mutually exclusive for most affected queries.

This keeps the work aligned with the active first-stage objective: prove whether
dense capability can be preserved in unified postings before any BM25,
reranker, or retrieval-expanded objective is reintroduced.
