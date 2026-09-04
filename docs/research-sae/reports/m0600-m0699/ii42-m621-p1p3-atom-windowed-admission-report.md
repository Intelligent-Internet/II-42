# II-42 M621 P1.3 Atom-Interaction Windowed Admission Report

Date: 2026-07-06

## Objective

M621 tests whether the M620 atom-interaction signal can become a useful scorer
policy.  It does not train a model and does not change the P1 encoder/posting
generator.

This is a full M604 replay probe, not a native DB/plugin benchmark.

## Implementation

- Probe script: `scripts/probe_m621_atom_windowed_admission.py`
- Feature: `atom_sign_conflict_rate`
- Gap root:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Atom root:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1`
- Output JSON:
  `runs/m621_p1p3_atom_windowed_admission_v1/m621_atom_windowed_admission.json`
- Output Markdown:
  `runs/m621_p1p3_atom_windowed_admission_v1/m621_atom_windowed_admission.md`

Grid:

- Preserve top-k: `95, 98, 99`
- Max admission rank: `200, 300, 500, 1000`
- Feature blend alpha: `0.05, 0.10, 0.20, 0.50, 1.0`

## Result

The probe tested 60 configs and accepted 9 under the mechanical macro gate.

Best config:

`top95_max500_atom_sign_conflict_rate_a0.05`

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Cand UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| Baseline | 0.775070 | 0.711403 | 0.882313 | 0.857118 | 1.000000 |
| M621 | 0.775070 | 0.711429 | 0.882680 | 0.857118 | 1.000000 |

Delta:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.000000 |
| MAP@100 | +0.000026 |
| Recall@100 | +0.000368 |
| MRR@20 | +0.000000 |

Per-dataset deltas for the best config:

| Dataset | dMAP@100 | dRecall@100 |
| --- | ---: | ---: |
| climate-fever | +0.000001 | +0.000000 |
| cqadupstack | -0.000027 | -0.000038 |
| dbpedia-entity | +0.000160 | +0.000293 |
| fiqa | +0.000002 | +0.000000 |
| msmarco | -0.000006 | +0.000000 |
| nfcorpus | +0.000100 | +0.000847 |
| scidocs | +0.000161 | +0.004000 |
| trec-covid | -0.000084 | -0.000336 |
| webis-touche2020 | +0.000002 | +0.000000 |

## Interpretation

M621 converts the M620 separability signal into only a tiny replay gain.
It does not beat M619 meaningfully:

- M619 best Recall delta: `+0.000387`
- M621 best Recall delta: `+0.000368`
- M619 best MAP delta: `+0.000055`
- M621 best MAP delta: `+0.000026`

The weak conversion matters.  M620 showed query-local separability on sampled
under-ranked positives, but once applied to the full candidate replay the
signal is too sparse and still harms `cqadupstack` and `trec-covid` slightly.

## Decision

Do not promote M621.

Do not run native DB/plugin benchmark for M621.

Stop the current scorer-route micro-tuning family:

- M617 pairwise linear scorer: rejected.
- M619 model-based bottom-slot admission: too small.
- M621 atom-interaction bottom-slot admission: too small and weaker than M619.

The next useful work should not be another alpha/window/search tweak.  It
requires changing the retrieval objective or teacher signal, or returning to a
first-stage model design that can create stronger candidate evidence rather
than only reshuffling the bottom of top100.

## Verification

Local verification passed for the M621 implementation:

- `python3 -m py_compile scripts/probe_m621_atom_windowed_admission.py`
- `pytest -q tests/test_probe_m621_atom_windowed_admission.py`
