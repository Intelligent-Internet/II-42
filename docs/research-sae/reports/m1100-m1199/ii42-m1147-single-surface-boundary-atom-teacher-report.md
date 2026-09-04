# ii42 M1147 Single-Surface Boundary Atom Teacher Audit

## Purpose

M1146 rejected a direct protected-tail transcode route: subtracting M1129 and
M1137 query atoms does not provide a discriminative same-query delta target.

M1147 tests a narrower and more useful question:

> If M1143 recall-gain actions identify qrels-positive documents crossing the
> top100 boundary, can those positives be explained inside one frozen posting
> namespace?

This audit uses only the M1129 atom namespace for the teacher.  It does not
train a model and does not deploy a gate.

## Inputs

- M1143 recall-gain oracle choices:
  `runs/m1143_protected_tail_query_gate_audit_v1/summary.json`
- M1129 atom tables from M1142:
  `ii42_m1142.<dataset>_m1129_atoms`
- M1147 rows:
  `runs/m1147_single_surface_boundary_atom_teacher_v1/teacher_rows.json`
- M1147 summary:
  `runs/m1147_single_surface_boundary_atom_teacher_v1/summary.md`
- Script:
  `scripts/audit_m1147_single_surface_boundary_atom_teacher.py`

## Method

For each `oracle_recall_gain` action from M1143:

1. Reconstruct the base top100 and the protected-tail candidate top100.
2. Take inserted qrels-positive documents as positive boundary targets.
3. Take displaced non-relevant base top100 documents as negative boundary
   targets.
4. Stay entirely inside the M1129 atom namespace.
5. Compute atom gains:
   `mean(atom impact in positive docs) - mean(atom impact in negative docs)`.
6. Select top K positive-gain atoms and measure whether adding them to the
   query improves the positive-vs-negative boundary margin.

M1147 also saves `target_atoms_top32` for each event row, so M1148 can train or
audit a compiler target without re-running native streams.

## Result

Shared15 event coverage:

| Metric | Value |
| --- | ---: |
| `event_count` | 149 |
| `ok_count` | 149 |
| `no_boundary_pair_count` | 0 |
| `positive_docs_mean` | 4.322148 |
| `negative_docs_mean` | 18.402685 |
| `positive_gain_atom_count_mean` | 86.523490 |

Top atom feasibility:

| Top K | Gain sum mean | New atom rate | Margin delta mean | Improve rate |
| ---: | ---: | ---: | ---: | ---: |
| 4 | 3.095518 | 0.723714 | +13.034281 | 0.986577 |
| 8 | 3.494874 | 0.819511 | +13.339906 | 0.986577 |
| 16 | 3.763936 | 0.875593 | +13.362342 | 0.986577 |
| 32 | 3.953070 | 0.905316 | +13.368267 | 0.986577 |

Dataset breakdown:

| Dataset | Events | OK | Top8 margin delta | Top8 improve rate |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | 1 | 1 | +2.673835 | 1.000000 |
| `climate-fever` | 6 | 6 | +160.408717 | 1.000000 |
| `cqadupstack` | 8 | 8 | +0.345557 | 1.000000 |
| `dbpedia-entity` | 27 | 27 | +1.716255 | 1.000000 |
| `fiqa` | 7 | 7 | +9.941291 | 1.000000 |
| `msmarco` | 16 | 16 | +45.075298 | 1.000000 |
| `nfcorpus` | 41 | 41 | +2.494185 | 1.000000 |
| `quora` | 1 | 1 | +0.000000 | 0.000000 |
| `scidocs` | 18 | 18 | +3.454856 | 0.944444 |
| `scifact` | 1 | 1 | +0.209223 | 1.000000 |
| `trec-covid` | 19 | 19 | +0.918435 | 1.000000 |
| `webis-touche2020` | 4 | 4 | +0.128578 | 1.000000 |

## Interpretation

M1147 is a genuine positive signal, and it resolves the M1146 contradiction:

- M1146 says protected-tail stream composition is not directly transcodable by
  subtracting two checkpoint query atom vectors.
- M1147 says the qrels-positive boundary documents found by that oracle do
  expose a clean same-surface atom teacher inside M1129.

The most important detail is the new-atom rate:

- top8 new atom rate is `0.819511`;
- top16 new atom rate is `0.875593`;
- top32 new atom rate is `0.905316`.

This means the next compiler cannot be only a rescaler of existing query
atoms.  It must generate or admit new atoms based on boundary-pair evidence.

## Decision

Promote M1147 as the next training target surface.

Do not promote it as a model result.  It is an oracle teacher construction,
using qrels-positive boundary documents.  Its value is that it gives a
same-namespace target after M1146 rejected the cross-checkpoint transcode path.

## Next Step

Proceed to M1148:

1. Use `target_atoms_top32` from M1147 as supervised atom labels.
2. Train a small atom selector/compiler under leave-one-dataset-out discipline.
3. Use only inference-time features:
   - base query atoms and weights;
   - native candidate/boundary evidence;
   - atom fanout and document support statistics;
   - positive-gain-like proxy features that do not use qrels at inference.
4. First gate is target-atom recovery on heldout datasets.
5. Only if target recovery is real, replay top4/top8 generated atoms through
   the native shared15 evaluator.

Stop if M1148 repeats M711/M717: high train AUC but poor heldout target
recovery.  Continue if heldout target atom recovery is non-trivial and margin
movement survives native replay.
