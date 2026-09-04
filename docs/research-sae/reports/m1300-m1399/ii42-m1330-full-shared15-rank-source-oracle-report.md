# M1330 Full Shared15 Rank-Source Oracle

## Question

M1325 showed strong rank-source oracle capacity on the risk7 surface. M1330
checks whether the same capacity exists on full shared15 using the M1329 replay
rows, without running new native DB queries.

This is still an oracle capacity audit. It is not deployable and must not be
promoted as a selector.

## Command

```bash
python3 scripts/audit_m1325_rank_source_oracle.py \
  --inputs runs/m1329_soft_rank_fusion_shared15_v1/m1318_anatomy.json \
  --output-root runs/m1330_rank_source_oracle_shared15_v1
```

## Result

| Variant | NegMetrics | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_best_any` | 0 | 0 | +0.003534 | +0.006511 | +0.006980 | +0.006291 | +0.001178 | +0.064920 |
| `oracle_best_nonnegative_else_baseline` | 0 | 0 | +0.003364 | +0.006383 | +0.006496 | +0.006294 | +0.001254 | +0.062803 |

For comparison, the best single source in M1329 was
`m1324_candidate_self_low_reserve1`:

| Variant | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1324_candidate_self_low_reserve1` | 12 | +0.001562 | +0.003536 | +0.003268 | +0.002995 | +0.000092 | +0.031034 |

M1330 roughly doubles the score while removing all dataset-level negative
metrics.

## Strict Oracle Source Counts

| Source | Queries |
| --- | ---: |
| `m1327_cw0p2_rank_prefix_uniform_l1` | 890 |
| `m1324_candidate_self_low_reserve1` | 232 |
| `baseline` | 166 |
| `m1327_cw0p2_rank_fill_fs0p5` | 25 |
| `m1324_rank_prefix_uniform_l1` | 17 |
| `m1324_rank_fill_fs0p5` | 12 |

The choices are not a pure dataset router. Each of the harder datasets mixes
baseline, candidate-self, and rank-source choices. Examples:

| Dataset | Dominant Strict Choices |
| --- | --- |
| `cqadupstack` | `cw0.2 prefix` 69, `candidate-self` 16, baseline 10 |
| `dbpedia-entity` | `candidate-self` 40, `cw0.2 prefix` 25, baseline 22 |
| `nfcorpus` | baseline 35, `candidate-self` 29, `cw0.2 prefix` 28 |
| `scidocs` | `candidate-self` 40, `cw0.2 prefix` 28, baseline 26 |
| `trec-covid` | baseline 28, `candidate-self` 17, `cw0.2 prefix` 3 |
| `webis-touche2020` | `cw0.2 prefix` 22, baseline 14, `candidate-self` 9 |

## Interpretation

This is the strongest evidence in the current M132x sequence.

It says the remaining row harm is not inherent to unified posting or the
available sources. A clean full-shared15 source mixture exists. The missing
piece is making source choice observable and intrinsic.

However, prior failures still matter:

- M1245 showed learned action routers were weak.
- M1296 showed native-outcome scalar rankers over current features did not
  transfer.
- M1329 showed a simple qrels-free rank fusion operator does not pass full
  shared15.

Therefore the next work should not train a vanilla source selector over the
same features. It should first audit source-choice observability with features
that are available before qrels/outcome, then design an objective that makes
source choice part of the generated ranking/source construction.

## Decision

Keep M1330 as the current strongest direction signal.

Stop:

- more candidate-stage composition;
- more rank-fusion grids;
- simple selector/gate training without an observability audit.

Next valid step:

1. Build a qrels-free source-choice feature audit for the strict oracle labels.
2. If source choices are not separable, design a new intrinsic objective rather
   than a selector.
3. If they are separable, test a small LODO source-choice policy on risk7
   before any full shared15 replay.
