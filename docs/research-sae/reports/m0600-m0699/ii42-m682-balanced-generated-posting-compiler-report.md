# M682 Balanced Generated-Posting Compiler

## Purpose

M682 tests whether the M681 broader retrieval teacher can improve P1 by
training a conservative global query-side atom boost/expansion compiler.

The motivation is the current route critique:

- Do not return to traditional SAE reconstruction loss.
- Keep unified posting as the engineering form.
- Upgrade from dense-only mimic to dense-faithful plus
  retrieval-constrained generated posting.
- Require support-safe boundary crossing before scaling.

M682 is intentionally small. It is not a new model family. It answers one
question: can a broader retrieval teacher become a simple global atom table
without breaking native candidate geometry?

## Inputs

- Teacher: `runs/m681_broader_retrieval_teacher_v1/m681_teacher_events.jsonl`
- Teacher events loaded: `13885`
- Train events after dataset/query balancing: `752`
- Evaluation path: native shared15 PostgreSQL path
- Queries evaluated: `1342`
- Heldout event-query surface: `61`
- Variants: baseline, M674, M682 strengths `0.01`, `0.02`, `0.04`

The raw M681 teacher JSONL is intentionally local and not committed because it
is large. The committed M681 builder can reproduce it from the native index.

## Full Shared15 Result

Best M682 strength by the current selector: `0.02`.

| Source | Queries | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | Candidate UB | Top95 overlap |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 1342 | 0.864763 | 0.667856 | 0.737286 | 0.820878 | 0.947106 | 1.000000 |
| M674 | 1342 | 0.870086 | 0.668196 | 0.737286 | 0.820878 | 0.947106 | 1.000000 |
| M682 | 1342 | 0.864916 | 0.668036 | 0.737327 | 0.821283 | 0.946917 | 0.991144 |

Delta vs baseline:

| Variant | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M682 0.01 | +0.000125 | +0.000219 | +0.000109 | +0.000542 | -0.000205 | 0.992062 |
| M682 0.02 | +0.000153 | +0.000180 | +0.000040 | +0.000405 | -0.000188 | 0.991144 |
| M682 0.04 | -0.000032 | +0.000592 | +0.000300 | +0.000830 | -0.000187 | 0.988807 |

## Heldout Event-Query Result

| Source | Queries | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | Candidate UB | Top95 overlap |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 61 | 0.444886 | 0.262160 | 0.464682 | 0.706666 | 0.800021 | 1.000000 |
| M674 | 61 | 0.460304 | 0.263087 | 0.464682 | 0.706666 | 0.800021 | 1.000000 |
| M682 | 61 | 0.446004 | 0.262496 | 0.464640 | 0.706666 | 0.798312 | 0.987748 |

M682 has small heldout Recall/MAP lift over baseline, but it is much weaker
than M674 and loses candidate upper bound. That violates the support-safe
boundary-crossing requirement.

## Interpretation

M682 should not be promoted.

The broader teacher is useful as an audit artifact, but a simple global atom
boost/expansion table is the wrong compiler. It touches too many queries in the
same direction, which produces tiny MAP/MRR gains while lowering candidate
upper bound and top95 preservation.

This directly supports the critique: current dense-only mimic and arbitrary
posting tweaks are at a bottleneck. The next useful version should not be a
larger global table or a return to SAE reconstruction. It should be a
support-safe, query-local, retrieval-constrained compiler.

## Blocker

The current blocker is not candidate availability alone. M681 showed many
under-ranked positives have shared support with the current query/doc atoms.
The blocker is controlled boundary crossing:

- moving useful positives into top100,
- without lowering candidate upper bound,
- without disrupting top95/head geometry,
- without selecting on eval qrels,
- and without dataset-specific thresholds.

M682 lacks the mechanism to satisfy those constraints.

## Next Breakthrough Point

The next experiment should be M683:

1. Build a counterfactual boundary table from M681 events.
2. For each query, compare proposed generated atoms against the specific
   boundary slot they would displace.
3. Train or derive a query-local compiler that emits generated postings only
   when the candidate-level counterfactual is support-safe.
4. Gate on native full shared15:
   - candidate upper bound must not drop,
   - top95 overlap must stay above floor,
   - Recall@100 and MAP@100 must improve over P1 baseline,
   - and the result must beat or complement M674, not merely polish ranks.

This keeps the main route aligned with P1:

- first phase: dense-root unified posting and support preservation,
- second phase: retrieval-constrained expansion only where it is safe.

## Files

- Script: `scripts/train_m682_balanced_generated_posting_compiler.py`
- Tests: `tests/test_train_m682_balanced_generated_posting_compiler.py`
- Run summary: `runs/m682_balanced_generated_posting_compiler_v1/m682_summary.json`
- Run report: `runs/m682_balanced_generated_posting_compiler_v1/m682_report.md`

## Decision

Reject M682 as a route to promote.

Keep M681 teacher and M682 evidence as proof that broader retrieval positives
cannot be consumed by a simple global atom table. Continue with
support-safe boundary crossing and query-local generated-posting compilation.
