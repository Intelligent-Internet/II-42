# M1225 CUB-Specific Atom Native Replay

## Question

M1224 showed that CUB-specific atom labels cleanly separate target atoms from
support-harm atoms.  M1225 tests whether this transfers to native retrieval.

Only one selector is replayed:

- teacher: M1224 CUB-specific target/harm
- selector: `rule_source_abs_top8`
- scales: `0.5`, `1.0`

This is a bounded conversion test, not a selector sweep.

## Results

### Hard-Row Smoke

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | NegMetrics |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1225_cub_source_abs_top8_s0.5` | 4.574 | -0.000933 | +0.000094 | +0.000811 | +0.001116 | -0.000803 | 2 |
| `m1225_cub_source_abs_top8_s1` | 4.574 | +0.000821 | +0.000031 | +0.000271 | +0.000379 | +0.000015 | 0 |

Scale `1.0` is the first hard-row pass for this branch with all five metrics
non-negative.

### Full Shared15

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | NegMetrics |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1225_cub_source_abs_top8_s0.5` | 7.970 | +0.001371 | +0.001187 | +0.000592 | +0.000729 | -0.000226 | 1 |
| `m1225_cub_source_abs_top8_s1` | 7.970 | +0.001685 | +0.002312 | +0.002033 | +0.002252 | +0.000012 | 0 |

Scale `1.0` is materially stronger than M1223 and preserves CUB at macro level.

### Per-Dataset Shape

The macro gain is meaningful but not row-robust yet.

For `m1225_cub_source_abs_top8_s1`, using the same native baseline as M1223:

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 0 | +0.010000 | +0.005798 | +0.006996 | +0.004937 | +0.000000 |
| `arguana` | 0 | +0.000000 | +0.008241 | +0.006284 | +0.008223 | +0.000000 |
| `climate-fever` | 0 | +0.002500 | +0.002992 | +0.004732 | +0.011163 | +0.000000 |
| `nfcorpus` | 1 | +0.000056 | +0.002756 | +0.005636 | +0.009430 | -0.000456 |
| `scidocs` | 0 | +0.008000 | +0.004153 | +0.001168 | +0.001411 | +0.000000 |
| `hotpotqa` | 0 | +0.000000 | +0.007356 | +0.003649 | +0.000000 | +0.000000 |
| `dbpedia-entity` | 1 | +0.001910 | +0.004096 | +0.003206 | +0.000000 | -0.000334 |
| `msmarco` | 0 | +0.000890 | +0.001737 | +0.000326 | +0.000000 | +0.001244 |
| `trec-covid` | 1 | +0.000249 | +0.000532 | -0.001530 | +0.000000 | +0.000691 |
| `webis-touche2020` | 1 | +0.000475 | +0.000434 | -0.001547 | +0.000000 | +0.000000 |
| `fiqa` | 2 | +0.000000 | -0.002940 | -0.000867 | +0.000588 | +0.000000 |
| `cqadupstack` | 4 | -0.000590 | -0.002651 | -0.002138 | -0.005526 | +0.000076 |

Metric sign counts:

- Recall@100: 8 positive, 6 zero, 1 negative.
- MAP@100: 10 positive, 3 zero, 2 negative.
- NDCG@10: 8 positive, 3 zero, 4 negative.
- MRR@20: 6 positive, 8 zero, 1 negative.
- Candidate upper bound: 3 positive, 10 zero, 2 negative.

## Interpretation

M1225 is the strongest recent signal.

It validates the review hypothesis: the main blocker was not simply model depth
or threshold tuning.  The teacher/candidate construction had mixed
ranking-positive atoms with support-risky atoms.  Once the teacher is made
CUB-specific, native replay improves substantially.

However, M1225 is not row-robust.  `cqadupstack` and `fiqa` still lose ranking
quality, while `nfcorpus` and `dbpedia-entity` still lose some CUB.  The route
is promising but cannot be promoted as default.

## Decision

Keep M1225 as the current best next-stage branch.

Do not continue generic threshold tuning.

Next valid step:

1. Add a row-level guard to M1225 using CUB-specific replay outcomes, not the
   older M1222 generic harm labels.
2. Require macro all-positive and fewer row-level negative metrics than M1225.
3. If guarded M1225 remains row-fragile, move the CUB-specific teacher into the
   generated-posting training objective rather than hand replay.

## Artifacts

- Script: `scripts/replay_m1225_cub_specific_atom_native.py`
- Smoke JSON: `runs/m1225_cub_specific_atom_native_smoke_v1/m1225_cub_specific_atom_native.json`
- Full JSON: `runs/m1225_cub_specific_atom_native_v1/m1225_cub_specific_atom_native.json`
