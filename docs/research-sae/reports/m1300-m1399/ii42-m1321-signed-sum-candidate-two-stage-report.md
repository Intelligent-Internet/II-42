# M1321 Signed-Sum Candidate Two-Stage Risk7

## Question

M1320 showed that the only CUB-positive risk7 candidate source is the older
`source_abs_signed_sum_s1`, but it has macro MRR harm. M1321 tests whether
using that source only for candidate generation, then ranking with a lower
pressure query, can keep CUB while fixing MRR.

## Run

```bash
python3 scripts/audit_m1318_two_stage_row_anatomy.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --candidate-stage signed_sum_top8 \
  --output-root runs/m1321_signed_sum_candidate_two_stage_risk7_v1
```

Artifacts:

- Script: `scripts/audit_m1318_two_stage_row_anatomy.py`
- JSON: `runs/m1321_signed_sum_candidate_two_stage_risk7_v1/m1318_anatomy.json`
- Markdown: `runs/m1321_signed_sum_candidate_two_stage_risk7_v1/m1318_anatomy.md`

Surface:

- datasets: `cqadupstack`, `fiqa`, `webis-touche2020`, `nfcorpus`,
  `dbpedia-entity`, `scidocs`, `trec-covid`
- queries: `599`
- validation: leave-one-dataset-out

## Result

| Variant | RankAtoms | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1321_rank_fill_fs0p5` | 3.990 | 0 | +0.000428 | +0.001729 | +0.001290 | +0.000704 | +0.000059 | 0.011376 |
| `m1321_rank_prefix_uniform_l1` | 2.469 | 1 | -0.000231 | +0.001486 | +0.001230 | +0.000662 | +0.000059 | 0.007148 |
| `m1321_candidate_self_signed_sum` | 7.816 | 1 | +0.001848 | +0.001444 | +0.001789 | -0.000961 | +0.000059 | 0.005682 |

M1321 is the first risk7 two-stage result with all macro deltas positive.

The best variant, `m1321_rank_fill_fs0p5`, keeps the CUB gain from the
candidate stage and converts macro MRR from negative to positive.

## Dataset-Level Shape

Best variant: `m1321_rank_fill_fs0p5`.

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 2 | -0.001233 | +0.000431 | +0.000758 | -0.000655 | +0.000000 |
| `dbpedia-entity` | 3 | -0.000543 | +0.003014 | +0.000952 | -0.004474 | -0.000461 |
| `fiqa` | 0 | +0.000000 | +0.003117 | +0.001067 | +0.000374 | +0.000000 |
| `nfcorpus` | 0 | +0.001847 | +0.001520 | +0.005170 | +0.008281 | +0.000339 |
| `scidocs` | 1 | +0.002000 | +0.001512 | +0.000362 | +0.000687 | -0.000500 |
| `trec-covid` | 1 | +0.000525 | +0.000398 | -0.001964 | +0.000000 | +0.001947 |
| `webis-touche2020` | 0 | +0.000475 | +0.001155 | +0.000817 | +0.000000 | +0.000000 |

## Interpretation

M1321 is a real risk7 structural improvement, but not yet a deployable policy.

It proves:

- signed-sum top8 is a better candidate stage than clean-prefix fill4 on risk7;
- a lower-pressure ranking query can fix macro MRR while keeping CUB;
- two-stage native replay is still the right structure to investigate.

It also exposes the next bottleneck:

- row-level harm remains concentrated in `cqadupstack`, `dbpedia-entity`,
  `scidocs`, and `trec-covid`;
- the ranking query sometimes damages head order where candidate self was
  already good.

## Decision

Do not promote to full `shared15` yet.

Next bounded structural probe:

> keep the signed-sum candidate stage, preserve the candidate-self head order,
> and rerank only the tail with `rank_fill_fs0p5`.

This tests whether the remaining harm is caused by over-reranking the head.
It is still qrels-free and native-index-shaped, not a dataset-specific gate.
