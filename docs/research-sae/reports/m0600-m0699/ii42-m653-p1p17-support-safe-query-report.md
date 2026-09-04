# M653 / P1.17 Support-Safe Query Report

## Status

M653 tests deterministic per-query query-posting deltas before adding model
capacity.  A proposed update is accepted only if its query-level guard passes;
otherwise the scorer falls back to frozen M549.

This stage directly follows M652's block: recall crossing appears only when
dense overlap is damaged.  M653 asks whether a constrained local delta can find
safe crossing cases.

## Runs

All comparable runs use the same split seed `6531`.

| Run | Accept overlap drop | Purpose | Status |
| --- | ---: | --- | --- |
| `m653_support_safe_query_seed6531` | 0.00 | strict support-safe gate | not promoted |
| `m653_support_safe_relaxed_same_seed6531` | 0.01 | allow one dense top100 slot loss | not promoted |
| `m653_support_safe_lenient_same_seed6531` | 0.05 | tradeoff diagnostic only | not promoted |

Artifacts:

- `runs/ii42-m653-p1p17-support-safe-query-v1/m653_support_safe_query_seed6531/m653_support_safe_query_seed6531.json`
- `runs/ii42-m653-p1p17-support-safe-query-v1/m653_support_safe_relaxed_same_seed6531/m653_support_safe_relaxed_same_seed6531.json`
- `runs/ii42-m653-p1p17-support-safe-query-v1/m653_support_safe_lenient_same_seed6531/m653_support_safe_lenient_same_seed6531.json`

## Matrix

| Run | Accepted test | Accepted boundary | All dO | All dNDCG | All dMAP | All dR | All dMRR | Bdy dO | Bdy dNDCG | Bdy dMAP | Bdy dR | Bdy dMRR |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| strict-0.00 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| relaxed-0.01 | 0 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| lenient-0.05 | 1 | 1 | -0.000195 | +0.000000 | +0.000021 | +0.000000 | +0.000000 | -0.001667 | +0.000000 | +0.000178 | +0.000000 | +0.000000 |

## Failed-Check Shape

On the strict test-boundary surface, all 30 attempted updates failed the
`dense_overlap_safe` check.  Several also failed MAP/NDCG/MRR:

| Run | dense overlap | MAP | NDCG | MRR | Recall |
| --- | ---: | ---: | ---: | ---: | ---: |
| strict-0.00 | 30 | 8 | 7 | 5 | 1 |
| relaxed-0.01 | 30 | 8 | 7 | 5 | 1 |
| lenient-0.05 | 28 | 8 | 7 | 5 | 1 |

The relaxed result is important: allowing one dense top100 slot loss still
accepts zero updates.  The lenient result accepts one update, but it does not
improve Recall and it already causes boundary dense-overlap regression beyond
the M653 acceptance target.

## What M653 Proves

1. Strict support-safe local query deltas do not find crossing.
   The solver attempted every boundary-positive test query and accepted zero
   under the strict gate.

2. The blocker is not only an overly strict one-slot overlap guard.
   Allowing `accept_overlap_drop = 0.01` still accepts zero updates.

3. Large overlap relaxation does not recover the desired signal.
   At `0.05`, one update is accepted, but Recall remains flat and overlap
   regresses.  This is not a useful tradeoff.

4. The phase boundary from M652 is confirmed.
   Continuous query-side movement can either preserve support or move the
   boundary, but this local delta form does not do both.

## Current Block

The current query-side route has reached a sharper stop condition:

- unconstrained movement can create Recall crossing but damages dense support;
- constrained movement preserves support but cannot create Recall crossing;
- per-query projected local deltas do not expose hidden safe crossing cases.

This does not prove the whole first-stage encoder idea is impossible.  It does
mean the current active-locked bucket-delta / local query-delta family is not
the right abstraction for the next breakthrough.

## Next Breakthrough Point

M654 should move from local query deltas to support-set transformation:

1. Analyze which atoms/dimensions are responsible for boundary positives that
   M652 can promote but M653 rejects.
2. Compute a feasibility map over coordinates:
   - coordinates that raise boundary positives,
   - coordinates that lower or reorder dense/P1 protected top100,
   - coordinates that are neutral to protected support.
3. If a non-empty neutral-positive coordinate set exists, build a masked
   query compiler over only those coordinates.
4. If no such set exists, stop this query-side compiler family and return to
   output-head/posting-compiler design, because the current query-only support
   space cannot express safe crossing.

Acceptance for M654:

- produce a coordinate/atom feasibility report before training,
- identify whether safe-positive coordinates exist,
- if yes, run a masked compiler canary;
- if no, stop this sub-route with evidence instead of adding capacity blindly.

## Decision

M653 is a useful negative.  It should not be scaled.  The next valid step is
M654 coordinate feasibility / support-set transformation, not another local
delta or scalar-weight sweep.
