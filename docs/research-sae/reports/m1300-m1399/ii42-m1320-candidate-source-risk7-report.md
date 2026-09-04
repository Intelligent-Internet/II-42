# M1320 Candidate-Source Risk7 Probe

## Question

M1319 showed that the M1317 two-stage ranking idea does not scale to risk7
because every variant has the same CUB loss. This suggests the candidate stage
is the bottleneck.

M1320 compares two candidate/fill sources on risk7 while holding the clean
prefix machinery fixed:

- `source_abs`;
- `head50_minus_tail`.

It also keeps the older `source_abs_signed_sum_s1` as an anchor.

## Run

```bash
python3 scripts/replay_m1314_head_support_fill_native.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --fill-rules source_abs,head50_minus_tail \
  --prefix-geometries uniform_l1 \
  --fill-to-values 4 \
  --output-root runs/m1320_candidate_source_risk7_v1
```

Artifacts:

- Script: `scripts/replay_m1314_head_support_fill_native.py`
- JSON: `runs/m1320_candidate_source_risk7_v1/m1314_native.json`
- Markdown: `runs/m1320_candidate_source_risk7_v1/m1314_native.md`

Surface:

- datasets: `cqadupstack`, `fiqa`, `webis-touche2020`, `nfcorpus`,
  `dbpedia-entity`, `scidocs`, `trec-covid`
- queries: `599`
- validation: leave-one-dataset-out

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1314_source_abs_signed_sum_s1` | 7.816 | 1 | +0.001848 | +0.001444 | +0.001789 | -0.000961 | +0.000059 | 0.005682 |
| `uniform_l1_source_abs_fill4` | 3.990 | 1 | +0.000181 | +0.001640 | +0.002588 | +0.000581 | -0.000606 | -0.003601 |
| `uniform_l1_head50_minus_tail_fill4` | 3.990 | 1 | +0.001128 | +0.001964 | +0.001274 | +0.000999 | -0.000774 | -0.004046 |

## Interpretation

M1320 explains the M1319 failure:

- clean-prefix fill sources are not CUB-safe on risk7;
- `head50_minus_tail` improves some rank/recall behavior, but worsens CUB;
- plain `source_abs` fill improves NDCG more, but still loses CUB;
- the only CUB-positive candidate source is the older
  `source_abs_signed_sum_s1`, but it has MRR harm.

This means the next structural test should not keep the M1314 fill4 candidate
stage. The candidate stage should use the CUB-positive `source_abs_signed_sum`
anchor, then rank with a lower-pressure query.

## Decision

Stop risk7 scaling for clean-prefix fill candidate sources.

Next valid branch:

1. Candidate stage: `source_abs_signed_sum_s1` to preserve CUB.
2. Ranking stage: `prefix_uniform_l1` or lower-pressure rank query.
3. Gate: CUB must stay non-negative and MRR must improve over
   `source_abs_signed_sum_s1`.

This is M1321. It directly tests whether the old macro-positive signed-sum
source can be converted into a deployable two-stage native policy.
