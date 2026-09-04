# M652 / P1.16 Multi-Teacher Query Report

## Status

M652 is a first-stage generated-posting canary after the M651 target audit.
It keeps document postings and index geometry frozen, and changes only the
query-side generated posting objective.

Three small FiQA/ArguAna canaries were run on `spark-1`:

| Run | Purpose | Status |
| --- | --- | --- |
| `m652_multiteacher_query_seed6521` | default multi-teacher loss | not promoted |
| `m652_multiteacher_anchor_b_seed6522` | stronger anchors, weaker replacement pressure | not promoted |
| `m652_multiteacher_dense_guard_c_seed6523` | dense-overlap guarded, smaller delta/bucket movement | not promoted |

Artifacts:

- `runs/ii42-m652-p1p16-multiteacher-query-v1/m652_multiteacher_query_seed6521/m652_multiteacher_query_seed6521.json`
- `runs/ii42-m652-p1p16-multiteacher-query-v1/m652_multiteacher_anchor_b_seed6522/m652_multiteacher_anchor_b_seed6522.json`
- `runs/ii42-m652-p1p16-multiteacher-query-v1/m652_multiteacher_dense_guard_c_seed6523/m652_multiteacher_dense_guard_c_seed6523.json`

## Best Observed Epochs

These rows use the best diagnostic epoch from each trace, not the selected
checkpoint.  No run selected a promotion-ready trained checkpoint.

| Run | Epoch | All dO@100 | All dNDCG | All dMAP | All dR@100 | All dMRR | Bdy dO@100 | Bdy dNDCG | Bdy dMAP | Bdy dR@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A default | 4 | -0.004635 | +0.000462 | +0.001399 | +0.002604 | +0.001690 | -0.008000 | -0.005201 | -0.000027 | +0.020000 |
| B anchor-heavy | 4 | -0.004792 | -0.000067 | -0.000205 | +0.005208 | +0.000069 | -0.008000 | -0.000836 | -0.000516 | +0.040000 |
| C dense-guarded | 2 | +0.000000 | -0.000783 | +0.000926 | +0.000000 | +0.000921 | +0.000000 | +0.000000 | +0.000003 | +0.000000 |

## What M652 Proves

1. The first-stage route can still create boundary crossing.
   A and B both create positive Recall movement.  B reaches all-query
   `dRecall@100 = +0.005208` and boundary `dRecall@100 = +0.040000`.

2. The crossing still buys recall by breaking dense-overlap geometry.
   Both crossing runs fall to about `dDenseOverlap@100 = -0.0047` all-query
   and `-0.0080` on boundary queries.  That repeats the core M636/M640
   warning: recall improvements are not trustworthy if dense-faithfulness is
   lost.

3. Stronger anchors alone are not enough.
   B improves boundary NDCG damage compared with A, but it still loses
   dense-overlap and boundary MAP.  The auxiliary precision teacher does not
   solve geometry preservation by itself.

4. Dense-guarded movement preserves geometry but does not cross.
   C keeps dense overlap at the best diagnostic epoch and preserves boundary
   ranking, but recall stays flat.  This exposes a phase boundary rather than a
   simple weight-tuning issue.

## Current Block

The current active-locked bucket-delta query compiler has two regimes:

- enough movement to cross top100, but dense-overlap and boundary ranking
  regress;
- enough anchoring to preserve dense geometry, but no top100 crossing.

This is now a clearer blocker than M651 had.  The problem is not that there is
no crossing signal; the problem is that the unconstrained continuous update
does not know which coordinates are allowed to move without changing the dense
top100 support.

## Next Breakthrough Point

Do not continue by only changing scalar weights.

The next experiment should be M653: a constrained support-safe query update.

Design:

1. Freeze document postings and the M549 support exactly as before.
2. Compute per-query dense/P1 top100 protected set and boundary positive set.
3. Learn or solve a small query delta under hard constraints:
   - protected dense top100 scores must not fall below a fixed margin,
   - dense overlap@100 must be optimized as a hard penalty, not just KL,
   - only coordinates that improve boundary positive margin without harming
     protected slots are allowed to move.
4. Start with deterministic projected gradient or closed-form constrained
   delta before training a model.  This tests mathematical feasibility before
   adding capacity.
5. If deterministic constrained deltas cannot cross without overlap loss, the
   current query-side compiler family should stop and the next route should be
   a different output-head/posting compiler.

Acceptance for M653:

- trained or solved checkpoint only; no epoch0 fallback,
- all-query dense overlap regression no worse than `-0.001`,
- boundary dense overlap regression no worse than `-0.001`,
- all-query MAP/NDCG/MRR non-negative,
- boundary MAP/NDCG non-negative or within strict tolerance,
- positive Recall crossing on boundary queries.

## Decision

M652 is a useful negative with a real positive signal.  It should not be scaled
to shared15.  The next valid move is a constrained, support-safe query update
probe, not another unconstrained multi-teacher weight sweep.
