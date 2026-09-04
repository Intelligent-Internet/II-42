# M1264 Uniform Weight Stability

## Question

M1263 found a clean macro-positive fixed-atom calibration signal:
`uniform_l1` keeps the same `source_abs/reserve0` top8 atom set and only
redistributes the per-query L1 impact budget evenly across selected atoms.

M1264 checks whether that signal is stable enough to become a direct native
policy by comparing it against raw signed-sum impacts at macro, row, and
query level.

## Run

- Script: `scripts/audit_m1264_uniform_weight_stability.py`
- Output root: `runs/m1264_uniform_weight_stability_v1/`
- JSON:
  `runs/m1264_uniform_weight_stability_v1/m1264_uniform_weight_stability.json`
- Markdown:
  `runs/m1264_uniform_weight_stability_v1/m1264_uniform_weight_stability.md`
- Datasets: full `shared15`
- Query count: `1342`

## Macro Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1264_raw_s1` | 7.975 | 0 | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | +0.030620 |
| `m1264_uniform_l1_s1` | 7.975 | 0 | +0.001734 | +0.003460 | +0.003526 | +0.003029 | +0.000416 | +0.032578 |

`uniform_l1` improves macro score over raw by `+0.001958` and keeps all five
macro metrics non-negative. The main gains are Recall@100 and CUB; the tradeoff
is slightly lower NDCG@10 and MRR@20 than raw, although both remain above the
P1 baseline.

## Uniform vs Raw Row Shape

Rows where uniform is clearly positive:

| Dataset | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | +0.001918 | +0.000122 | +0.000324 | +0.001736 | +0.002093 | +0.016167 |
| `fiqa` | +0.000000 | +0.002167 | +0.001629 | +0.001648 | +0.000000 | +0.013056 |
| `msmarco` | +0.001984 | +0.000143 | +0.000143 | +0.000000 | -0.000135 | +0.007136 |
| `arguana` | +0.000000 | +0.000476 | +0.000457 | +0.000476 | +0.000000 | +0.003296 |

Rows where uniform is risky relative to raw:

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scidocs` | 3 | +0.002000 | -0.000687 | -0.002533 | -0.006018 | +0.002000 | -0.099542 |
| `webis-touche2020` | 2 | +0.000000 | -0.000929 | -0.000792 | +0.000000 | +0.000000 | -0.021577 |
| `climate-fever` | 1 | +0.000000 | +0.000985 | -0.001883 | +0.001357 | +0.000000 | -0.016924 |
| `hotpotqa` | 2 | +0.000000 | -0.000799 | -0.000425 | +0.000000 | +0.000000 | -0.015492 |
| `dbpedia-entity` | 2 | +0.000058 | -0.000592 | -0.000073 | +0.000026 | +0.000185 | -0.008041 |
| `cqadupstack` | 3 | -0.000246 | +0.000392 | +0.000015 | -0.000500 | -0.000076 | -0.008009 |

## Query-Level Stability

`uniform_l1` vs raw:

| Wins | Losses | Ties | MeanScoreDelta |
| ---: | ---: | ---: | ---: |
| 176 | 196 | 970 | -0.022585 |

Uniform reduces negative query counts relative to the P1 baseline:

| Variant vs baseline | Recall neg | MAP neg | NDCG neg | MRR neg | CUB neg |
| --- | ---: | ---: | ---: | ---: | ---: |
| `raw_s1` | 27 | 188 | 104 | 36 | 40 |
| `uniform_l1_s1` | 24 | 179 | 90 | 29 | 39 |

But relative to raw it still creates query-level ranking losses:

| Uniform vs raw | Recall neg | MAP neg | NDCG neg | MRR neg | CUB neg |
| --- | ---: | ---: | ---: | ---: | ---: |
| Count | 13 | 190 | 40 | 10 | 23 |

The worst losses are concentrated in rank-sensitive cases, especially
`scidocs`, with large MRR/NDCG drops on individual queries.

## Interpretation

`uniform_l1` is a real score-geometry signal, not a default policy.

What it proves:

- the selected `source_abs/reserve0` atom set still contains useful retrieval
  signal;
- raw signed-sum magnitudes are not always the best native scoring geometry;
- reducing magnitude inequality can improve Recall@100 and CUB without changing
  atom admission.

What it does not prove:

- it is not row-safe enough to promote;
- it loses average query-level score against raw;
- its harms are concentrated in rank-sensitive rows, so a direct uniform/raw
  switch would likely become another fragile gate problem.

## Decision

Do not promote `uniform_l1` as an engineering default.

Retain it as:

- a calibration/objective signal;
- evidence that the next branch should optimize score geometry, not only atom
  selection;
- a candidate teacher for learned/listwise impact calibration.

Do not continue by adding a simple threshold or guard on the current result.
The correct next step is an observability audit over the raw selected-atom
geometry:

1. reconstruct the exact fixed atom set per query;
2. compute qrels-free features such as top weight share, entropy,
   positive/negative sign balance, L1 concentration, and contrast statistics;
3. test whether uniform-vs-raw gain/harm is separable before any learned blend
   or native replay.

If that audit cannot separate gains from harms, keep `uniform_l1` as a
regularizer/teacher inside a future training objective rather than as a
deployable query-time policy.
