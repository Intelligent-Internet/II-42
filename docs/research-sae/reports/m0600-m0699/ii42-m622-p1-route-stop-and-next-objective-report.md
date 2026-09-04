# II-42 M622 P1 Route Stop Audit and Next Objective

Date: 2026-07-06

## Objective

M622 consolidates the M606-M621 evidence and decides whether the active
P1.2/P1.3 loop should keep training dense-equivalent score surfaces, reopen a
downstream scorer, or change the teacher/objective.

This report does not promote a new model.  It is a route-control checkpoint to
avoid repeating already-failed loss families.

## Frozen Baselines

Keep these frozen:

- P1-a0125 and M549U as historical baselines.
- M603/M604 native infrastructure and scorer-gap audit shape.
- M605 as a rejected benchmark, not a promoted scorer.
- P1.3-a010 as the current native first-stage/fixed-alpha candidate.

## Evidence Summary

### M606: Original P1 score geometry was not dense-equivalent enough

M606 showed the initial P1-a0125 surface had poor dense geometry:

- query-macro dense top100 in P1 top100: `0.421238`,
- dense-vs-false-top100 z-score margin: `-0.582822`,
- dense rank vs P1 rank Pearson: `0.344016`.

This justified the first-stage score-surface work.

### M608: Native score-interface bug was the large first-stage fix

M608 found the native atom interface mismatch:

- documents stored signed coordinates as separate non-negative atom postings,
- queries emitted only same-sign positive atoms,
- native scoring dropped opposite-sign negative contributions.

P1.3 fixed this by using dual signed-dot query emission while keeping the
document posting lifecycle intact.

Aligned shared15 result:

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P1.3 | 0.93972 | 0.94374 | 0.77401 | 0.69393 | 0.85178 | 0.87117 |
| P1.3-a010 | 0.94166 | 0.92843 | 0.77881 | 0.70001 | 0.85426 | 0.87182 |
| P1.3-a0125 | 0.94264 | 0.92054 | 0.77908 | 0.70037 | 0.85420 | 0.87044 |

This means the main dense-equivalent native scoring mismatch is already fixed.

### M612-M614: Dense-only output/head perturbations failed the gate

The following first-stage loss families were tested and rejected:

- query-adaptive coordinate gain,
- baseline score anchoring,
- local soft packing,
- dense topK recall surrogate.

They produced either no-op selections or small qrels-facing gains with dense
overlap / coverage regression.  Scaling these variants would violate the
first-stage dense-equivalence gate.

### M615: Remaining under-ranked positives are mostly dense-miss

M615 decomposed the remaining P1.3-a010 scorer gap:

| Macro | Dense hit | P1 hit | Fused hit | Under-ranked | Under-ranked dense-miss share |
| --- | ---: | ---: | ---: | ---: | ---: |
| Micro | 0.341553 | 0.341150 | 0.342660 | 0.488350 | 0.980678 |
| Dataset macro | 0.806032 | 0.804706 | 0.806628 | 0.163571 | 0.944688 |

The dominant remaining misses are not aligned-dense top100 documents.  A
dense-only teacher can polish the small dense-hit residue, but it cannot teach
the dominant remaining positives into top100.

### M617-M621: Downstream scorer probes convert poorly

Second-stage probes were scoped because M615 showed a measurable scorer gap.
They are now also rejected:

| Line | Best useful signal | Decision |
| --- | --- | --- |
| M617 pairwise linear ranker | selected split gains, full replay near zero or collapse | reject |
| M619 model-based bottom-slot admission | dR@100 `+0.000387`, dMAP `+0.000055` | too small |
| M621 atom-conflict admission | dR@100 `+0.000368`, dMAP `+0.000026` | too small |

M620 found a strong sampled AUC for `atom_sign_conflict_rate`, but M621 showed
that this signal does not convert into meaningful full-replay improvement.

## Additional Gap Check

For under-ranked positives that are candidate-present and aligned-dense-miss:

- total examples: `13,895`,
- BM25 already in top100: `1,813` (`13.05%`),
- P1 already in top100: `83` (`0.60%`).

Fused-rank distribution:

| Fused rank bin | Count |
| --- | ---: |
| 101-200 | 4,356 |
| 201-500 | 6,337 |
| 501-1000 | 3,202 |

This explains why bottom-slot admission can recover only tiny gains.  There is
room in the candidate pool, but the existing P1/BM25 scalar and atom-conflict
signals do not identify the positives reliably enough.

## Decision

Stop the current two failed families:

1. Stop dense-only micro-training that only perturbs the output/head score
   surface while using aligned dense top100 as the sole teacher.
2. Stop scorer-route micro-tuning based on fixed alpha, scalar native features,
   bottom-slot admission, or single atom-interaction heuristics.

Do not run native DB/plugin benchmark for M617/M619/M621.  The replay gains
are too small and would waste engineering-evaluation time.

Keep P1.3-a010 as the current best frozen candidate.

## Next Objective

The next useful direction is not "another reranker" and not "another
dense-only head tweak".  It needs a new first-stage teacher/objective that is
still globally trained and auditable, but no longer limited to aligned dense
top100 imitation.

Recommended M623 scope:

1. Build a qrels-free or weak-label teacher that expands beyond dense top100:
   - dense top100 as the stable core,
   - high-confidence lexical/entity matches as auxiliary positives,
   - corpus-derived query/document pseudo-pairs,
   - consistency across dense root, P1 atom support, and lexical coverage.
2. Train the posting compiler/output head to emit candidate evidence for this
   expanded teacher, not just reproduce aligned dense rank.
3. Keep BM25 out of the first-stage scorer at inference, but allow lexical
   evidence as training supervision if it is global and not dataset-specific.
4. Preserve dense-equivalent gates as floors:
   - active recall/support must not regress,
   - aligned O@100 cannot collapse,
   - P1.3-a010 remains the fallback.
5. Only after M623 improves candidate evidence should M604/M605-style scorer
   audits be reopened.

## Stop Rule for M623

Stop the next loss family if it does not improve at least one of:

- candidate miss rate,
- candidate-present positive rank distribution,
- M604 under-ranked-positive rate,
- native Recall@100/MAP@100,

while preserving dense overlap/support floors.

If gains require dataset-specific thresholds or qrels-tuned rows, reject the
family.

## Verification

Route evidence used in this report is backed by:

- M606 dense-surface diagnostic,
- M608 signed-dot query native score-interface fix,
- M612-M614 first-stage negative canaries,
- M615 dense-rank gap decomposition,
- M617/M619/M621 full replay scorer probes.

The report itself requires no training run.
