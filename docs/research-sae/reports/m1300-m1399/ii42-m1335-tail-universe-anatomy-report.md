# M1335 Tail Universe Anatomy

## Question

M1333 and M1334 both failed at selecting tail atoms. Before training another
model, M1335 asks a simpler source-construction question:

> Is there any pre-scoring tail universe with both useful target coverage and
> acceptable target base rate?

This is an anatomy audit only. It does not run native retrieval.

## Smoke Result

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Universe | TargetCoverage | TargetBaseRate | MeanSize | TargetHit | TargetTotal |
| --- | ---: | ---: | ---: | ---: | ---: |
| `candidate_tail` | 1.0000 | 0.2357 | 3.373 | 198 | 198 |
| `candidate_x_support128` | 0.9646 | 0.2341 | 3.277 | 191 | 198 |
| `candidate_x_support64` | 0.8182 | 0.2355 | 2.763 | 162 | 198 |
| `candidate_x_fill` | 0.7374 | 0.2786 | 2.104 | 146 | 198 |
| `fill_tail` | 0.7374 | 0.2776 | 2.112 | 146 | 198 |
| `candidate_x_fill_x_support32` | 0.4697 | 0.3207 | 1.165 | 93 | 198 |
| `support_top128` | 0.9646 | 0.0064 | 120.092 | 191 | 198 |

Per-dataset best universe:

| Dataset | BestUniverse | TargetCoverage | TargetBaseRate | MeanSize |
| --- | --- | ---: | ---: | ---: |
| `cqadupstack` | `candidate_tail` | 1.0000 | 0.1929 | 4.200 |
| `scidocs` | `candidate_tail` | 1.0000 | 0.3548 | 1.860 |
| `webis-touche2020` | `candidate_tail` | 1.0000 | 0.2179 | 4.776 |

## Interpretation

This explains the apparent conflict between M1333 and M1334.

`candidate_tail` is not a bad universe: it has full target coverage and a
target base rate around `23.6%`. The M1333 failure was not that candidate tail
contains no signal; it was that the available features could not safely choose
a small subset of that tail.

Pure support-generated tail is the opposite: it covers targets but has an
extremely low base rate (`0.64%` at `support_top128`), which explains M1334.

The best structural reading is:

- `rank_prefix` is the clean anchor.
- `candidate_tail` is the recall-bearing tail.
- selecting a tiny tail subset is hard.
- support-only generation is too broad.

## Decision

Do not continue support-only tail generation.

Do not train another tail selector over candidate-tail features.

The only local route still worth testing is continuous low-weight blending of
the whole candidate tail into the rank-prefix anchor. That is exactly what
M1336 tests.

## Artifacts

- Script:
  `scripts/audit_m1335_tail_universe_anatomy.py`
- Smoke JSON:
  `runs/m1335_tail_universe_anatomy_smoke_v1/m1335_tail_universe_anatomy.json`
- Smoke Markdown:
  `runs/m1335_tail_universe_anatomy_smoke_v1/m1335_tail_universe_anatomy.md`
