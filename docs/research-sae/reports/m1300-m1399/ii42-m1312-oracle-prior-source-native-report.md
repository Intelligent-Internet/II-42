# M1312 Oracle-Prior Source Native Report

## Question

M1311 showed that two-stage query-feature selection cannot approximate the
M1309 all-safe oracle. M1312 moves the supervision into source construction
instead of adding another selector.

For each leave-one-dataset-out fold:

1. use training folds to compute the M1309 all-safe oracle choices;
2. mark atoms from oracle-move choices as positive;
3. mark `signed_sum` atoms from oracle-abstain queries as false-move risk;
4. construct held-out `signed_sum` sources by filtering or shrinking atoms
   using this oracle-conditioned prior.

This tests whether global atom move/abstain counts can internalize the harm
separation that M1311 could not learn as a post-hoc gate.

## Run

```bash
python3 scripts/replay_m1312_oracle_prior_source_native.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --modes signed_sum,mean_teacher \
  --scales 0.5,0.75,1.0 \
  --risk-penalties 0.25,0.5,1.0 \
  --output-root runs/m1312_oracle_prior_source_native_risk7_v1
```

Artifacts:

- Script: `scripts/replay_m1312_oracle_prior_source_native.py`
- JSON: `runs/m1312_oracle_prior_source_native_risk7_v1/m1312_native.json`
- Markdown: `runs/m1312_oracle_prior_source_native_risk7_v1/m1312_native.md`

Surface:

- datasets: `cqadupstack`, `fiqa`, `webis-touche2020`, `nfcorpus`,
  `dbpedia-entity`, `scidocs`, `trec-covid`
- queries: `599`
- validation: leave-one-dataset-out

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `signed_sum_s1` | 7.816 | 1 | +0.001848 | +0.001444 | +0.001789 | -0.000961 | +0.000059 | 0.005682 |
| `mean_teacher_s1` | 7.673 | 1 | +0.000463 | +0.001098 | +0.000710 | -0.000260 | +0.000966 | 0.004869 |
| `m1312_prior_r1_filter_s1` | 2.648 | 1 | +0.000634 | +0.001292 | +0.000813 | -0.000379 | +0.000194 | 0.004319 |
| `m1312_prior_r0.5_weighted_s1` | 5.020 | 1 | +0.000738 | +0.001622 | +0.001416 | -0.000664 | +0.000739 | 0.004156 |
| `m1312_prior_r0.5_filter_s1` | 5.020 | 1 | +0.000790 | +0.001889 | +0.001909 | -0.001490 | +0.000668 | -0.003777 |
| `m1312_prior_r1_weighted_s1` | 2.648 | 3 | +0.000288 | +0.000756 | -0.000067 | -0.000567 | -0.000095 | -0.006370 |
| `m1312_prior_r0.25_weighted_s1` | 6.003 | 1 | +0.001143 | +0.000790 | +0.001145 | -0.001572 | +0.000689 | -0.007795 |
| `m1312_prior_r0.25_filter_s1` | 6.003 | 1 | +0.001219 | +0.000690 | +0.001235 | -0.002788 | +0.000671 | -0.022157 |

No M1312 variant beats fixed `signed_sum_s1`.

The most conservative filter, `m1312_prior_r1_filter_s1`, keeps only
`2.648` atoms/query and reduces MRR harm from `-0.000961` to `-0.000379`, but
it gives up too much Recall/NDCG gain and remains below the fixed source.

The mid-risk weighted variant, `m1312_prior_r0.5_weighted_s1`, improves CUB and
keeps some MAP/NDCG, but still has MRR harm and remains weaker than
`signed_sum_s1`.

The looser priors keep more atoms but worsen MRR harm. This matches the
M1311 failure mode: the available signal can reduce movement volume, but it
does not identify rank-safe movement.

## Interpretation

M1312 is a useful negative source-construction result.

It rules out a simple global atom prior as the missing interface. The M1309
all-safe labels do contain useful supervision, but compressing them into
global atom move/abstain counts loses the context that matters for rank safety.

This narrows the next step:

- do not continue post-hoc selector/gate variants from M1310/M1311;
- do not continue global atom prior filters from M1312;
- keep `signed_sum_s1` as the retained movement target;
- require the next source to include richer context than global atom counts.

The next candidate source needs query-local structure, such as action-source
identity, native rank boundary state, document/context witnesses, or
pairwise movement features, before replay. A source that only knows whether an
atom was historically positive or risky is too coarse.

## Decision

Stop M1312 at risk7.

Do not scale this global prior to full `shared15`.

Next valid branch:

1. build a context-conditioned source, not a global atom prior;
2. require target/harm separability before native replay;
3. run risk7 smoke before full `shared15`;
4. only scale if it beats fixed `signed_sum_s1` while reducing MRR/NDCG harm.

The retained conclusion after M1311-M1312 is:

> safe movement is not observable from the current source summaries or global
> atom priors. The missing signal is likely query-local/action-local context,
> not another selector or scalar calibration over the same source.
