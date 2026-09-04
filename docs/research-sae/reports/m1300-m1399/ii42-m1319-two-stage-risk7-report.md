# M1319 Two-Stage Risk7 Anatomy

## Question

M1318 showed that M1317 two-stage replay is clean on the hard-row smoke:

- macro metrics all non-negative;
- dataset-level deltas all non-negative;
- best rank query: `prefix_uniform_l1`.

M1319 expands exactly one level to risk7 before any full `shared15` replay.

## Run

```bash
python3 scripts/audit_m1318_two_stage_row_anatomy.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --output-root runs/m1319_two_stage_row_anatomy_risk7_v1
```

Artifacts:

- Script: `scripts/audit_m1318_two_stage_row_anatomy.py`
- JSON: `runs/m1319_two_stage_row_anatomy_risk7_v1/m1318_anatomy.json`
- Markdown: `runs/m1319_two_stage_row_anatomy_risk7_v1/m1318_anatomy.md`

Surface:

- datasets: `cqadupstack`, `fiqa`, `webis-touche2020`, `nfcorpus`,
  `dbpedia-entity`, `scidocs`, `trec-covid`
- queries: `599`
- validation: leave-one-dataset-out

## Result

| Variant | RankAtoms | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1314_single_pass_fill4` | 3.990 | 1 | +0.001128 | +0.001964 | +0.001274 | +0.000999 | -0.000774 | -0.004046 |
| `m1317_rank_fill_fs0p5` | 3.990 | 1 | +0.000428 | +0.001729 | +0.001290 | +0.000704 | -0.000774 | -0.008807 |
| `m1317_rank_prefix_uniform_l1` | 2.469 | 2 | -0.000231 | +0.001486 | +0.001230 | +0.000662 | -0.000774 | -0.013035 |

The M1317 hard-row winner does not generalize to risk7. On risk7:

- best score reverts to the single-pass M1314 fill4 variant;
- every variant has the same CUB loss: `-0.000774`;
- two-stage prefix ranking loses Recall macro and is worse than single-pass;
- dataset-level failures remain broad.

## Dataset Failure Shape

Best risk7 row: `m1314_single_pass_fill4`.

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 1 | +0.000648 | +0.000682 | +0.000862 | -0.000729 | +0.000076 |
| `dbpedia-entity` | 3 | -0.000387 | +0.003146 | +0.001345 | -0.004474 | -0.000180 |
| `fiqa` | 0 | +0.000000 | +0.003740 | +0.001655 | +0.001708 | +0.000000 |
| `nfcorpus` | 1 | +0.001868 | +0.001619 | +0.005172 | +0.008281 | -0.000374 |
| `scidocs` | 2 | +0.004000 | +0.001613 | -0.001471 | +0.001199 | -0.004500 |
| `trec-covid` | 1 | +0.000791 | +0.000687 | -0.000625 | +0.000000 | +0.000683 |
| `webis-touche2020` | 0 | +0.000475 | +0.001270 | +0.000774 | +0.000000 | +0.000000 |

## Interpretation

M1319 is a stop signal for scaling the current M1317 two-stage rank query.

The important part is the failure mode:

- hard-row two-stage replay fixed NDCG without CUB loss;
- risk7 exposes a candidate-stage CUB loss shared by all ranking variants;
- therefore the next bottleneck is not final rank pressure, but candidate
  source generalization.

This does not invalidate M1317. It says the structure is useful only after the
candidate stage is CUB-safe across broader rows.

## Decision

Do not run full `shared15` for M1317.

Stop:

- direct expansion of `rank_prefix_uniform_l1`;
- more rank-query variants on the same candidate source;
- returning to selector/gate tuning over this source.

Next bounded probe:

1. Compare candidate sources on risk7 while holding the clean prefix fixed:
   `source_abs` vs `head50_minus_tail`.
2. Treat CUB as the first gate, because all risk7 variants failed there.
3. Only revisit two-stage ranking after candidate CUB is non-negative.

The immediate test is M1320:

```bash
python3 scripts/replay_m1314_head_support_fill_native.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --fill-rules source_abs,head50_minus_tail \
  --prefix-geometries uniform_l1 \
  --fill-to-values 4 \
  --output-root runs/m1320_candidate_source_risk7_v1
```
