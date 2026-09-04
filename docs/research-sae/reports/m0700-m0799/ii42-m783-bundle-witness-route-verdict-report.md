# M783 Bundle Witness Route Verdict

## Decision

Stop the current bundle selector/policy tuning route.

M779 and M781 are useful.  M780 and M782 show the deployable policy is not yet
usable.  The next route must add a new interaction witness, not another
threshold/model over the same native displacement features.

## Evidence

| Run | Question | Result |
| --- | --- | --- |
| M779 | Do stable two-coordinate bundles have a clean oracle ceiling? | yes |
| M780 | Can a trained selector recover that ceiling? | safe but diagnostic-scale |
| M781 | Is the oracle visible from qrels-free native features? | partly yes |
| M782 | Can deterministic cross-zero policy turn that signal into gain? | no |

M779 strict-positive oracle on test:

- clean: yes
- applied queries: 51.7
- dMAP: +0.000596
- dNDCG: +0.001129
- dMRR: +0.000201
- dCUB: +0.000135
- dO@100: +0.000000

M780 best trained selector on test:

- model: HGB
- clean: yes
- applied queries: 5.7
- dMAP: +0.000007
- dNDCG: +0.000033
- utility: +0.000040

M781 observability:

- `cross_rate_at_100` transfers strongly:
  - train AUC: 0.907132
  - eval AUC: 0.903732
- all strict-positive rows have no top100 crossing in test mean.
- positive signatures are stable:
  - dev positive signatures: 13
  - test positive signatures: 13
  - intersection: 13

M782 best deterministic cross-zero policy:

- policy: `cross_rate_at_100 <= 0`, `total_scale <= 0.04`,
  `tail_abs_share_at_100 >= 0.6`, `entropy_delta_at_100 <= 0`
- dev: clean no-op, utility 0
- test: not clean, dMAP -0.000001

## Interpretation

The bundle unit is better than the old single-coordinate row unit.  It creates a
real oracle ceiling without dense-overlap spend.

The available qrels-free native displacement features can detect safety,
especially "do not change the top100 membership."  They do not determine
whether a safe within-top100 reorder is relevance-positive.  That is why M780
collapses to a few safe rows, and M782 can be safe on dev but slightly negative
on heldout tasks.

This means the missing signal is not more training depth or a better global
threshold.  The missing signal is a relevance proxy for the documents affected
by the bundle.

## What To Keep

Keep:

- M779 stable bundle generator as a proposal unit.
- M781 `cross_rate_at_100 == 0` as a safety witness.
- Multi-surface original/seed7642/seed7643 replay as the minimum gate.

Do not keep:

- M780 trained selector as a candidate route.
- M782 deterministic cross-zero policy as an improvement route.
- Any further selector tuning using only bundle shape and native displacement
  features.

## Next Route

The next bounded route should be M784 interaction-witness bundle selection.

Add features that can explain relevance-positive movement inside the unchanged
top100 set:

- lexical overlap / IDF coverage of boosted documents;
- BM25-like score of boosted versus demoted documents;
- query/doc atom overlap from the existing P1 atom publisher path;
- whether boosted documents are supported by query-visible or BM25-visible
  atoms.

The experiment should stay bounded:

1. Build interaction-witness features for the same M779 bundle rows.
2. Audit dev-to-test transfer of those features before training.
3. Only if transfer improves beyond M781, run one trained selector smoke.
4. Stop if it does not beat M780 while staying clean.

Acceptance for M784:

- all surfaces gate and no negative tasks;
- mean utility above +0.00005;
- dMAP above M780's +0.000007 by at least 5x;
- no dense-overlap or candidate-upper-bound regression.

If M784 fails, the native query-local bundle policy route should be considered
exhausted.  The next serious work would be model/output-level generated posting
training with interaction supervision, not another native policy layer.
