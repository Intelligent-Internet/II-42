# M1291 Action-Source Atom Outcome Audit

## Question

M1290 showed that generic raw atom candidates do not expose enough safe
effective single atoms for deployable training.  M1291 tests the review-driven
alternative: change the candidate source before changing the selector.

The audit compares action-aware and CUB-aware sources by applying each selected
atom individually through the native scorer and measuring metric movement.

## Setup

- Datasets: `arguana,cqadupstack,fiqa,scidocs`
- Limit: `25` queries per dataset
- Split: leave-one-dataset-out source construction
- Sources: `signed_sum`, `high`, `mid`, `low`, `cub_source_abs`
- Source top-n: `8`
- Scale: `1.0`
- Output:
  `runs/m1291_action_source_atom_outcome_limit25_v1/m1291_outcome.json`

For `cub_source_abs`, CUB-specific contrast and mean deltas are learned from
the other datasets only.  The heldout dataset is used only for native outcome
measurement.

## Source Outcome

| Source | Rows | EffectiveShare | HarmfulShare | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `high_s1` | 71 | 0.084507 | 0.112676 | +0.000000 | -0.000852 | -0.000558 | +0.000000 | +0.000000 |
| `mid_s1` | 71 | 0.070423 | 0.098592 | +0.000000 | -0.001015 | -0.000622 | +0.000000 | +0.000000 |
| `cub_source_abs_s1` | 184 | 0.054348 | 0.146739 | +0.000000 | -0.001103 | -0.000191 | -0.000453 | +0.000000 |
| `low_s1` | 118 | 0.033898 | 0.161017 | +0.000000 | -0.001194 | +0.000038 | -0.000706 | +0.000000 |
| `signed_sum_s1` | 184 | 0.070652 | 0.163043 | +0.000000 | -0.003104 | -0.001552 | -0.002264 | +0.000000 |

No source clears the positive gate.  The best single-atom source is `high_s1`,
but it still has negative MAP/NDCG movement and more harm than effective wins.

## Interpretation

M1291 does not invalidate M1251.  Instead, it clarifies why a naive atom-level
objective is still wrong for this branch.

M1251 showed that set-level `signed_sum_s1` can be macro-positive on full
shared15.  M1291 shows that individual atoms from the same family are not
reliably positive in isolation.  The useful movement is therefore likely
combinatorial: the source works as a small coordinated set, not as independently
labelled atoms.

This is consistent with the current bottleneck:

- Useful action/CUB structure exists.
- Generic raw source is too noisy.
- Single-atom labels are too local and misrepresent set-level movement.
- Post-hoc gate/filter training remains the wrong next step.

## Decision

Stop single-atom outcome label training for this branch.

Do not train an atom classifier from M1291 rows.  The next valid test should be
set-level native replay under constrained source construction.

## Next Valid Step

Run a bounded M1292 set-source audit:

1. Compare small coordinated sets, not individual atoms.
2. Start with the known positive shape: `signed_sum_s1` / all-action top8.
3. Add protected-head constraints directly into set construction rather than
   post-hoc veto.
4. Evaluate on the same limit surface first, then shared15 only if the small
   surface is row-safe.

The concrete hypothesis is:

> The branch should train or construct coordinated posting deltas, not isolated
> atom decisions.  Harm separation must be a set-level source property.
