# M1318 Two-Stage Row Anatomy

## Question

M1317 found a macro-safe hard-row smoke for the two-stage native structure:

`candidate: M1314 fill4` -> `rank: prefix_uniform_l1`

M1318 checks whether that result is distributed across datasets and queries
before any broader replay.

## Run

```bash
python3 scripts/audit_m1318_two_stage_row_anatomy.py \
  --datasets cqadupstack,scidocs,webis-touche2020 \
  --output-root runs/m1318_two_stage_row_anatomy_smoke_v1
```

Artifacts:

- Script: `scripts/audit_m1318_two_stage_row_anatomy.py`
- JSON: `runs/m1318_two_stage_row_anatomy_smoke_v1/m1318_anatomy.json`
- Markdown: `runs/m1318_two_stage_row_anatomy_smoke_v1/m1318_anatomy.md`

Surface:

- datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- queries: `249`
- validation: leave-one-dataset-out

## Macro Result

| Variant | RankAtoms | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1317_rank_prefix_uniform_l1` | 1.309 | 0 | +0.001977 | +0.000862 | +0.000654 | +0.000695 | +0.000015 | 0.015183 |
| `m1317_rank_fill_fs0p5` | 3.422 | 0 | +0.001847 | +0.000739 | +0.000395 | +0.001364 | +0.000015 | 0.014988 |
| `m1314_single_pass_fill4` | 3.422 | 1 | +0.002030 | +0.000708 | -0.000054 | +0.001153 | +0.000015 | 0.013944 |

## Dataset Deltas

Best variant: `m1317_rank_prefix_uniform_l1`.

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 0 | +0.000922 | +0.000314 | +0.000880 | +0.000055 | +0.000038 |
| `scidocs` | 0 | +0.004000 | +0.001699 | +0.000222 | +0.001675 | +0.000000 |
| `webis-touche2020` | 0 | +0.000000 | +0.000272 | +0.001072 | +0.000000 | +0.000000 |

The signal is not carried by a single dataset. All three hard rows are
non-negative across all five metrics.

## Query-Level Harm

| Variant | AnyNegQ | RecallNegQ | MAPNegQ | NDCGNegQ | MRRNegQ | CUBNegQ |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1317_rank_prefix_uniform_l1` | 58 | 2 | 57 | 12 | 5 | 1 |
| `m1317_rank_fill_fs0p5` | 65 | 3 | 64 | 15 | 5 | 1 |
| `m1314_single_pass_fill4` | 68 | 3 | 66 | 19 | 7 | 1 |

Query-level harm still exists, but it is reduced versus the single-pass
baseline. More importantly, it no longer aggregates into dataset-level harm.

## Interpretation

M1318 confirms M1317 as a real structural signal on the hard-row smoke:

- macro deltas all positive;
- dataset deltas all non-negative;
- query harm is lower than M1314 single-pass;
- the ranking query is small (`1.309` atoms/query), so this is not a hidden
  larger-ranking-vector fix.

This supports the current structural hypothesis:

> unified posting should use one query vector to generate candidate support and
> a lower-pressure query vector to rank that support.

## Decision

Proceed to risk7 replay before full `shared15`.

Next gate:

```bash
python3 scripts/audit_m1318_two_stage_row_anatomy.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --output-root runs/m1319_two_stage_row_anatomy_risk7_v1
```

Promote only if:

- macro deltas remain non-negative;
- dataset-level negative metrics stay absent or tiny and explainable;
- CUB remains non-negative;
- gains do not collapse outside the original hard-row smoke.

If risk7 fails, do not return to single-pass value/gate tuning. The failure
should be analyzed as a two-stage rank-policy/generalization problem.
