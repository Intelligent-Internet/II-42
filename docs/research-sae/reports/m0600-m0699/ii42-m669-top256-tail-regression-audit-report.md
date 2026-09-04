# M669 Top256 Tail Regression Audit Report

## Scope

M669 audits the first-stage failure mode exposed by M667/M668. It does not run
new training and does not change the model. It only compares per-query rows
from existing query dumps:

- baseline: `p1_native`
- candidate: output-blend active-lock variants
- split: `test`
- focus: seed `6546`, because it repeatedly fails `dense_overlap_256_safe`

The goal is to determine whether the M668 stop signal is likely caused by
insufficient training depth or by a specific boundary/projection weakness.

## Key Finding

The recurring failure is narrow top256 tail churn, not broad degradation.

For `seed6546`, candidate variants keep:

- dense overlap@100 unchanged;
- Recall@100 unchanged;
- candidate upper bound unchanged;
- MAP/NDCG/MRR effectively unchanged except one unrelated tiny trec-covid MAP
  movement.

But they lose one dense top256 member on a few individual queries. Each loss is
exactly `-0.003906250`, which is `-1 / 256` for that query.

This is a boundary membership failure. Longer training would likely increase
the chance of crossing more boundaries unless the projection itself is changed.

## Seed6546 Breakdown

| Run | Changed queries | O@256 regress queries | O@100 regress queries | O@256 gains |
| --- | ---: | ---: | ---: | ---: |
| M667 blend 0.25 | 5 | 4 | 0 | 0 |
| M668 blend 0.15 | 3 | 3 | 0 | 0 |
| M668 blend 0.20 | 5 | 4 | 0 | 0 |
| M668 blend 0.30 | 6 | 4 | 0 | 1 |

Recurring O@256 regressions:

| Dataset | Query | Delta O@256 | Delta O@100 | Delta Recall@100 |
| --- | --- | ---: | ---: | ---: |
| arguana | `test-environment-assgbatj-pro04a` | `-0.003906250` | `0` | `0` |
| dbpedia-entity | `INEX_LD-2012353` | `-0.003906250` | `0` | `0` |
| nfcorpus | `PLAIN-817` | `-0.003906250` | `0` | `0` |
| fiqa | `753` | `-0.003906250` | `0` | `0` |

The fiqa row appears at blend `0.20`, `0.25`, and `0.30`, but not at `0.15`.
The other three rows appear even at blend `0.15`, so simply making the global
blend smaller is not enough.

## Interpretation

The current route should not be abandoned for lack of training depth. It has a
different problem:

- top100 and retrieval-facing metrics are already stable in this failure case;
- active-lock protects the active support but not the dense top256 tail;
- scalar output damping cannot know which tail members are protected;
- selected checkpoints can pass important top100 gates while still moving one
  tail document across rank 256.

This explains why fixed-scale sweeps did not produce a robust global default.
The scalar is too blunt: smaller scales reduce movement but do not guarantee
top256 preservation, while larger scales create more boundary churn.

## Consequence For Training Strategy

The current fast-iteration ratio is appropriate for this phase. We should not
spend long runs on a mechanism that fails by exact boundary crossing. The
correct next experiment is structural:

- add a top256-aware projection or lock;
- penalize only unsafe tail exits, not all movement;
- keep top100 / Recall / CUB gates unchanged;
- keep BM25, reranker, learned gate, and qrels optimization out of scope.

If a top256-safe projection can remove the seed6546 tail losses while retaining
the seed6545 positive movement, then this route deserves broader training. If
it cannot, deeper training of the same mechanism is unlikely to fix the first
stage.

## Next Step

M670 should implement a query-side top256 preservation guard:

1. compute the baseline dense-overlap top256 membership at evaluation time;
2. project or clamp candidate query output so known top256 tail members cannot
   be displaced unless a strict no-loss check passes;
3. rerun the same 4-seed surface at the safest diagnostic scale, starting from
   `0.25`;
4. accept only if O@100, O@256, Recall@100, CUB, and dense-hit loss all pass.

This is still a first-stage dense-equivalence experiment, not a ranking
optimization experiment.
