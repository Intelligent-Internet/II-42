# M1218 Contrastive Atom Proposal Model Smoke

M1218 tests whether the M1217 contrastive teacher can be transferred into a
small qrels-free atom proposal model.

This is the first step after teacher validation:

- train on train-fold teacher targets
- input only baseline query atoms
- predict atom additions on held-out queries
- replay through the same native query path

The smoke failed.  Full shared15 was not launched.

Datasets:

- `cqadupstack`
- `scidocs`
- `webis-touche2020`

## Model

The model is intentionally small and auditable:

- contrastive atom filter: `ratio=1.5`, `min_positive_count=2`
- teacher target: top 8 atoms from oracle-winning filtered deltas
- proposal scoring: base-query atom cooccurrence plus global prior
- variants: `harm_lambda in {0, 0.5, 1.0}`, `top_n in {3, 8}`,
  `scale in {0.5, 1.0}`

Held-out inference does not use oracle action identity.

## Smoke Result

No proposal variant passed the safety gate.

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `proposal_h1_top3_s0.5` | 3.000 | 3 | -0.001209 | -0.000547 | -0.000599 | +0.000105 | +0.000015 | -0.020121 |
| `proposal_h0.5_top8_s1` | 8.000 | 4 | -0.001080 | -0.000172 | -0.000880 | +0.002088 | -0.000583 | -0.029174 |
| `proposal_h0.5_top3_s0.5` | 3.000 | 4 | -0.000130 | -0.000409 | -0.000583 | +0.000105 | -0.000788 | -0.033237 |
| `proposal_h1_top8_s1` | 8.000 | 3 | -0.000933 | +0.000081 | -0.000274 | +0.002459 | -0.001386 | -0.038833 |

The best-looking variants only improve MRR while damaging Recall/CUB/NDCG.
That is not acceptable for this route.

## Interpretation

This is a useful negative result.

M1216/M1217 proved the filtered teacher is safe when replaying oracle-winning
filtered deltas.  M1218 shows that a shallow baseline-atom cooccurrence model
cannot recover those deltas safely.

So the current state is:

- Teacher signal exists.
- Raw oracle atoms are too noisy.
- Contrastive filtering produces a safe target.
- A naive qrels-free cooccurrence proposal model cannot predict that target.

This separates the problem:

> The bottleneck is no longer teacher feasibility.  It is proposal
> observability/modeling.

## Decision

- Do not run full shared15 for M1218.
- Do not launch larger training from the cooccurrence/prior model.
- Keep M1217 teacher as the current positive supervision target.
- Next step should diagnose why the target is not predictable from baseline
  atoms alone.

## Next Direction

Run a proposal observability audit before any new training:

1. Measure held-out overlap between predicted atoms and M1217 teacher atoms.
2. Compare baseline-atom cooccurrence against richer qrels-free features:
   source ranks, fused/BM25/P1 margins, boundary entrant stats, and atom source
   contribution.
3. If target overlap remains weak, the generated-posting model needs
   retrieval-conditioned input, not just text/query atoms.
4. If richer features recover target overlap, train a small supervised model
   with those features and rerun native smoke.

Do not continue by tuning `harm_lambda`, `top_n`, or `scale`; all current
variants failed the same safety pattern.

## Artifacts

- Script: `scripts/audit_m1218_contrastive_atom_proposal_model.py`
- Smoke JSON: `runs/m1218_contrastive_atom_proposal_model_smoke_v1/m1218_contrastive_atom_proposal_model.json`
- Smoke Markdown: `runs/m1218_contrastive_atom_proposal_model_smoke_v1/m1218_contrastive_atom_proposal_model.md`
