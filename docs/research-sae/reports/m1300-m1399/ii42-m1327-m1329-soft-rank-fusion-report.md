# M1327-M1329 Soft Rank Fusion

## Question

M1325 showed that query-level source choice has strong oracle capacity.
M1327 tests a deployable operator-level approximation:

```text
rank_score = rank_source_position + w * candidate_source_position
```

This is a qrels-free rank aggregation inside one unified-posting candidate
pool. It is not a learned selector or post-hoc guard.

## M1327 Risk7 Coarse Sweep

Command:

```bash
python3 scripts/audit_m1318_two_stage_row_anatomy.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --candidate-stage low_reserve1 \
  --candidate-rank-weights 0,0.25,0.5,1,2 \
  --output-root runs/m1327_soft_rank_fusion_risk7_v1
```

Best:

| Variant | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1327_cw0p25_rank_fill_fs0p5` | 8 | +0.001239 | +0.001787 | +0.001241 | +0.000792 | +0.000071 | +0.015697 |
| `m1324_rank_fill_fs0p5` | 7 | +0.000428 | +0.001729 | +0.001290 | +0.000704 | +0.000071 | +0.011388 |

`cw0.25` improves macro score but increases dataset negatives.

## M1328 Risk7 Narrow Frontier

Command:

```bash
python3 scripts/audit_m1318_two_stage_row_anatomy.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --candidate-stage low_reserve1 \
  --candidate-rank-weights 0,0.05,0.1,0.15,0.2,0.25 \
  --output-root runs/m1328_soft_rank_fusion_frontier_risk7_v1
```

Useful frontier:

| Variant | DatasetNeg | AnyNegQ | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1327_cw0p2_rank_fill_fs0p5` | 7 | 182 | +0.001099 | +0.001769 | +0.001404 | +0.000870 | +0.000071 | +0.015422 |
| `m1327_cw0p15_rank_fill_fs0p5` | 7 | 185 | +0.001022 | +0.001743 | +0.001290 | +0.000870 | +0.000071 | +0.014732 |
| `m1324_rank_fill_fs0p5` | 7 | 184 | +0.000428 | +0.001729 | +0.001290 | +0.000704 | +0.000071 | +0.011388 |

M1328 is a genuine risk7 operator signal: `cw0.2` improves macro score without
increasing dataset-negative count.

## M1329 Full Shared15 Gate

Command:

```bash
python3 scripts/audit_m1318_two_stage_row_anatomy.py \
  --datasets arguana,climate-fever,cqadupstack,dbpedia-entity,fever,fiqa,hotpotqa,msmarco,nfcorpus,nq,quora,scidocs,scifact,trec-covid,webis-touche2020 \
  --candidate-stage low_reserve1 \
  --candidate-rank-weights 0,0.2 \
  --output-root runs/m1329_soft_rank_fusion_shared15_v1
```

Full shared15 result:

| Variant | DatasetNeg | AnyNegQ | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1324_candidate_self_low_reserve1` | 12 | 246 | +0.001562 | +0.003536 | +0.003268 | +0.002995 | +0.000092 | +0.031034 |
| `m1324_rank_prefix_uniform_l1` | 13 | 240 | +0.001087 | +0.001469 | +0.001234 | +0.001370 | +0.000092 | +0.015144 |
| `m1327_cw0p2_rank_prefix_uniform_l1` | 13 | 235 | +0.000920 | +0.001537 | +0.001345 | +0.001377 | +0.000092 | +0.014748 |
| `m1324_rank_fill_fs0p5` | 15 | 230 | +0.001112 | +0.001426 | +0.000816 | +0.001054 | +0.000092 | +0.013670 |
| `m1327_cw0p2_rank_fill_fs0p5` | 16 | 225 | +0.000956 | +0.001481 | +0.000918 | +0.001057 | +0.000092 | +0.013269 |

The risk7 soft-fusion gain does not transfer. On full shared15, candidate-self
low-reserve remains the strongest macro source, and `cw0.2` rank fusion is
weaker than its `cw0` control while increasing dataset harm.

## Interpretation

Soft rank fusion is useful as a local diagnostic but not a deployable
breakthrough.

It explains part of the M1325 oracle capacity: mixing candidate and rank-source
order can improve selected risk rows. But the same operator is not stable on
full shared15. The full gate says the broader surface prefers the simpler
candidate-self low-reserve source, not the rank-fusion source.

## Decision

Stop the rank-fusion branch:

- no finer weight grid;
- no learned scalar candidate/rank interpolation;
- no full replay for other fusion weights unless a new source changes the
  underlying choice anatomy.

Retain:

- M1325: rank-source choice has strong oracle capacity.
- M1328: soft rank fusion is a local risk7 signal.
- M1329: full shared15 rejects soft rank fusion as the next default.

Next work should target intrinsic source-choice/objective construction. The
current qrels-free rank operators are not enough.
