# M1223 CUB-Guarded Harm-Aware Native Replay

## Question

M1222 showed a mixed signal: harm-aware atom selection improved ranking
metrics on full shared15, but lost candidate upper bound and remained unsafe on
hard rows.  M1223 tests the minimal structural fix:

keep the same M1222 atom selector, but add a query-level abstain gate trained on
held-out native replay support-risk labels.

This is not another atom/lambda/scale sweep.  The base selector is fixed:

`target_score - harm_score > 0`, top8 atoms, scale `0.5`.

## Results

### Hard-Row Smoke

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | NegMetrics |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1222_harm_aware_top8_s0.5` | 0.683 | -0.000076 | -0.000118 | -0.000415 | -0.000037 | +0.000266 | 4 |
| `m1223_all_safe_p05` | 0.245 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000015 | 0 |
| `m1223_rank_safe_p05` | 0.245 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000015 | 0 |
| `m1223_cub_safe_p05` | 0.000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0 |

The guard removes the hard-row damage.  It does not produce meaningful hard-row
gain.

### Full Shared15

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | NegMetrics |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1222_harm_aware_top8_s0.5` | 1.841 | +0.000177 | +0.000121 | +0.000113 | +0.000114 | -0.000305 | 1 |
| `m1223_cub_safe_p05` | 1.016 | +0.000103 | +0.000081 | +0.000050 | +0.000028 | +0.000018 | 0 |
| `m1223_cub_recall_safe_p05` | 0.931 | +0.000126 | +0.000080 | +0.000136 | +0.000028 | -0.000133 | 1 |
| `m1223_rank_safe_p05` | 0.449 | +0.000038 | +0.000061 | -0.000029 | +0.000029 | -0.000163 | 2 |
| `m1223_all_safe_p05` | 0.328 | +0.000034 | +0.000034 | -0.000039 | +0.000010 | -0.000163 | 2 |

`m1223_cub_safe_p05` is the only variant with all five macro deltas positive.

### Per-Dataset Breakdown

`m1223_cub_safe_p05` is macro-positive, but it is not yet row-robust.

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 1 | +0.001111 | -0.000326 | +0.001447 | +0.000000 | +0.000000 |
| `nfcorpus` | 1 | +0.000182 | +0.000197 | +0.000159 | -0.000042 | +0.000320 |
| `arguana` | 0 | +0.000000 | +0.000179 | +0.000179 | +0.000179 | +0.000000 |
| `fiqa` | 0 | +0.000000 | +0.000112 | +0.000069 | +0.000000 | +0.000000 |
| `trec-covid` | 2 | -0.000026 | -0.000038 | +0.000384 | +0.000000 | +0.000268 |
| `msmarco` | 0 | +0.000000 | +0.000011 | +0.000072 | +0.000000 | +0.000000 |
| `hotpotqa` | 0 | +0.000000 | +0.000034 | +0.000000 | +0.000000 | +0.000000 |
| `scifact` | 0 | +0.000000 | +0.000012 | +0.000000 | +0.000000 | +0.000000 |
| `climate-fever` | 1 | +0.000000 | -0.000014 | +0.000000 | +0.000076 | +0.000000 |
| `webis-touche2020` | 2 | +0.000000 | -0.000120 | -0.000174 | +0.000000 | +0.000000 |
| `scidocs` | 1 | +0.000000 | +0.000519 | -0.000604 | +0.000159 | +0.000000 |
| `dbpedia-entity` | 2 | +0.000096 | +0.000450 | -0.000720 | +0.000000 | -0.000206 |

Rows not shown with nonzero movement are zero-delta rows.

Metric sign counts for `m1223_cub_safe_p05`:

- Recall@100: 3 positive, 11 zero, 1 negative.
- MAP@100: 8 positive, 3 zero, 4 negative.
- NDCG@10: 6 positive, 6 zero, 3 negative.
- MRR@20: 3 positive, 11 zero, 1 negative.
- Candidate upper bound: 2 positive, 12 zero, 1 negative.

## Interpretation

This is the cleanest result in the recent M1216-M1223 branch:

- M1220 proved retrieval-conditioned atoms expose the teacher target.
- M1221 proved harm-aware selection can separate target from harm on full
  shared15.
- M1222 proved the selector can improve ranking, but loses support.
- M1223 proves a support-risk abstain gate can preserve support while keeping a
  small positive native gain.

The result is still small and row-fragile.  It should not be called a final
breakthrough or default candidate.  The hard-row smoke is safe mostly because
the gate abstains, not because it solves the hard rows.  The full macro win is
also not uniformly distributed across datasets.

## Decision

Keep M1223 as the current best deployable-shaped signal in this branch, but do
not promote it beyond shared15.

Do not continue local lambda/threshold/scale tuning on shared15.

The next valid step is not broader validation yet.  The next step should change
the teacher/candidate construction so support risk is represented at the atom
or action level before selection:

1. Build a CUB-specific atom teacher: positive atoms from winning actions that
   preserve CUB, negative atoms from actions that reduce CUB.
2. Run an observability audit before replay.
3. Replay only if target atoms separate from CUB-harm atoms on held-out folds.
4. If CUB-specific target/harm separation fails, stop this branch and change the
   candidate source.

## Artifacts

- Script: `scripts/replay_m1223_cub_guarded_harm_aware_native.py`
- Smoke JSON: `runs/m1223_cub_guarded_harm_aware_native_smoke_v1/m1223_cub_guarded_harm_aware_native.json`
- Full JSON: `runs/m1223_cub_guarded_harm_aware_native_v1/m1223_cub_guarded_harm_aware_native.json`
