# M786 Interaction-Witness Selector Verdict

## Decision

Stop native bundle selector work.

M784 shows the missing relevance proxy exists.  M785 shows the proxy is not
stable enough as a small native selector on the current M779 bundle proposal
pool.  The next route should move interaction supervision into generated
posting / output-level training, not continue policy swaps.

## Evidence

| Run | Question | Result |
| --- | --- | --- |
| M779 | Stable bundle oracle ceiling | clean, useful |
| M781 | Safety observability | strong via `cross_rate_at_100` |
| M782 | Deterministic cross-zero policy | not clean |
| M784 | Interaction witness observability | positive |
| M785 | Interaction-witness selector replay | not clean |

M784 key result inside the cross-zero safety set:

| Feature | Train AUC | Eval AUC |
| --- | ---: | ---: |
| boosted_lex_coverage_max | 0.685203 | 0.719687 |
| boosted_lex_coverage_mean | 0.655800 | 0.715463 |
| demoted_lex_coverage_mean | 0.642464 | 0.700310 |
| demoted_lex_coverage_max | 0.654465 | 0.691949 |

This is the first recent result that adds a new signal beyond native
displacement/safety.  It supports the hypothesis from M783: safe top100-internal
movement needs query-document relevance witnesses.

M785 best dev result:

- model: HGB
- dev clean: yes
- dev applied: 22.7
- dev dMAP: +0.000383
- dev dNDCG: +0.000359
- dev dMRR: +0.000483
- dev utility: +0.000852

M785 test result for the same HGB threshold:

- test clean: no
- test applied: 17.3
- test dMAP: -0.000054
- test dNDCG: +0.000062
- test dCUB: -0.000021
- negative tasks:
  - original: climate-fever, webis-touche2020
  - seed7642: nfcorpus
  - seed7643: dbpedia-entity, fiqa, nfcorpus, trec-covid

The logistic model is smaller and has positive utility on test, but still fails
clean/gate due to seed7643 nfcorpus and CUB regression:

- test applied: 2.7
- test dMAP: +0.000007
- test dNDCG: +0.000080
- test dCUB: +0.000005 mean, but one surface regresses
- negative task: seed7643 nfcorpus

## Interpretation

The failure mode changed.

M780/M782 failed because they could not see relevance-positive movement.
M784/M785 show relevance-positive movement is visible with lexical interaction
features.  However, the current unit of action is still too brittle:

- it selects from a tiny fixed bundle pool;
- it makes hard query-local choices after the posting scores already exist;
- it cannot adjust the generated posting surface itself;
- dev-positive lexical signals do not reliably transfer under seed/surface
  shifts.

So the issue is no longer "no useful signal."  The issue is that the native
selector layer is the wrong place to consume the signal.

## What To Keep

Keep:

- M779 bundle oracle as evidence that support-safe improvements exist.
- M781 cross-zero as a required safety witness.
- M784 lexical interaction features as supervision/witness features.

Do not keep:

- M780 selector.
- M782 deterministic policy.
- M785 native selector as deployable.
- further threshold/model swaps over the same fixed M779 proposal pool.

## Next Route

The next route should be M787 interaction-supervised generated-posting smoke.

Core change:

- do not choose among fixed native bundles after scoring;
- instead use M784-style interaction witnesses to supervise the posting
  generator/output head so that the generated query posting surface already
  favors lexically and semantically supported safe movements.

Bounded design:

1. Use the same shared15 original/seed7642/seed7643 surfaces.
2. Construct a training target from M779/M784:
   - positive: cross-zero rows with high boosted lexical coverage and
     strict-positive oracle label;
   - negative: cross-zero rows with similar native displacement but no
     lexical/retrieval benefit.
3. Train only a small output-level delta/scoring head or proposal generator,
   not a full model rewrite.
4. Evaluate through the same native replay gate.

Acceptance:

- clean on all three surfaces;
- dMAP above M780 by at least 5x;
- utility above +0.00005;
- no dense-overlap or CUB regression;
- no dataset-specific thresholds.

Stop condition:

If M787 cannot beat M785 while staying clean, stop this native query-local
improvement path and return to broader model-level generated-posting training
with a larger supervision surface.
