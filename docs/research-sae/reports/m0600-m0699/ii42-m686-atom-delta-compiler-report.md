# M686 Atom Delta Compiler

## Status

M686 is complete and should be kept as a positive research signal, but not yet
promoted over M674.

M686 changes the failed M685 target. Instead of selecting tail documents and
copying their raw atoms, it reconstructs M683 accepted oracle deltas and trains
an atom-level compiler:

- teacher: M683 accepted rank-safe oracle rows;
- labels: atoms added or boosted by the accepted generated query;
- inference: native proposal-pool atom votes only;
- evaluation: native shared15 PostgreSQL path.

This is the first post-M683 non-oracle run where a generated-posting compiler
can improve holdout Recall and MAP together while preserving candidate upper
bound.

## Teacher Diagnostics

| Item | Value |
| --- | ---: |
| Training atom rows | 71,449 |
| Positive atom rows | 1,177 |
| Positive share | 0.016473 |
| Target atoms | 2,714 |
| Visible target atoms | 1,177 |
| Atom model AUC | 0.734722 |
| Gate holdout AUC | 0.805118 |

The most important diagnostic is visibility: only `1,177 / 2,714` target atoms
are visible in the current native proposal-pool atom votes. That means M686 is
partly proposal-limited, but it is no longer only a document-selection problem.

## Variants

M686 tested four useful surfaces:

1. default: `8 atoms`, `scale=0.05`, `shared_boost=1.15`;
2. conservative A: `4 atoms`, `scale=0.03`, `shared_boost=1.0`;
3. conservative B: `8 atoms`, `scale=0.03`, `shared_boost=1.0`;
4. conservative C: `8 atoms`, `scale=0.02`, `shared_boost=1.0`.

The best current candidate is conservative C.

## Best Candidate: `s002_a8`

Artifacts:

- `runs/m686_atom_delta_compiler_s002_a8_v1/m686_summary.json`
- `runs/m686_atom_delta_compiler_s002_a8_v1/m686_report.md`

Configuration:

- selected atoms: `8`
- expansion scale: `0.02`
- shared boost: `1.0`
- gate apply rate: `0.139344` on holdout, `0.141580` full

Full shared15 delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 | 1.000000 |
| atom_delta_gate | +0.000183 | +0.000016 | -0.000078 | -0.000018 | +0.000026 | 0.998918 |

Holdout delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.003747 | +0.000190 | +0.000000 | +0.000000 | +0.000000 | 1.000000 |
| atom_delta_gate | +0.000820 | +0.000125 | -0.000046 | +0.000000 | +0.000035 | 0.998921 |

## Other Variant Signals

Default `s005_a8_shared1.15`:

- holdout `atom_delta_all`: Recall `+0.000956`, CUB `+0.001998`,
  NDCG `+0.000071`, but MAP `-0.000088`;
- holdout `atom_delta_gate`: MAP `+0.000240`, NDCG `+0.000053`,
  but Recall `0.000000`, CUB `-0.000182`.

Conservative `s003_a8`:

- holdout `atom_delta_gate`: Recall `+0.000934`, MAP `+0.000242`,
  NDCG `+0.000024`, MRR `+0.000000`, CUB `-0.000043`.

Conservative `s003_a4`:

- holdout `atom_delta_gate`: Recall `+0.000934`, MAP `+0.000047`,
  MRR `+0.000000`, but NDCG `-0.000111`, CUB `-0.000150`.

The pattern is consistent:

- atom-level delta prediction can recover Recall;
- lower scale improves CUB and head preservation;
- the current gate is useful but not yet metric-aware enough;
- M674 is still much stronger on Recall.

## Interpretation

M686 is the first clear positive signal after the M685 document-copy failure.

What changed:

1. M684 selected candidate documents and copied atoms.
2. M685 widened the document proposal pool and added boundary features, but
   still copied/averaged raw document atoms.
3. M686 predicts atom deltas directly from proposal-pool atom votes.

The result confirms the current route:

- do not return to traditional SAE reconstruction;
- keep dense-root unified posting as the engineering form;
- use retrieval-constrained generated posting;
- make the compiler atom-level, not document-copy based.

The result also gives a concrete next bottleneck:

> The compiler now has positive Recall/MAP/CUB movement, but it still lacks a
> metric-aware admission gate for NDCG/MRR/CUB and is far behind M674 on Recall.

## Decision

Keep M686 as a positive milestone.

Do not promote it as the new default. It does not beat M674 and still has a
small NDCG regression on the best candidate. However, it is the first
non-oracle generated-posting compiler in this sequence with:

- holdout Recall up;
- holdout MAP up;
- holdout CUB up;
- full Recall up;
- full MAP up;
- full CUB up;
- top95 near preserved.

## Next Step

Proceed to M687 as a metric-aware atom admission stage, not a larger model.

Recommended M687:

1. Keep the M686 atom model and `s002_a8` delta shape.
2. Train a metric-aware query gate or atom-admission gate using M686
   counterfactual rows:
   - labels should require Recall/MAP/CUB non-regression;
   - penalize NDCG/MRR drops;
   - preserve top95 floor.
3. Add per-query counterfactual logging so we can classify:
   - true wins;
   - CUB wins with rank loss;
   - rank wins with Recall miss;
   - unsafe deltas.
4. Promotion gate:
   - holdout Recall, MAP, and CUB must stay positive;
   - NDCG/MRR cannot materially regress;
   - full surface must preserve the same direction;
   - compare against M674 explicitly.

If M687 can remove the tiny NDCG/MRR regression while preserving the M686
Recall/MAP/CUB gains, the route should expand beyond shared15. If it cannot,
the next blocker is missing atom visibility (`1,177 / 2,714`) and the proposal
pool must be expanded before more modeling.
