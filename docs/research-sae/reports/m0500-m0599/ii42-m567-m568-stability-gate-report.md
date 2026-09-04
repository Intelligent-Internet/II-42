# M567 Pairwise and M568 Conservative Gate Report

M567 and M568 are follow-ups to the M566 official-1024 BEIR7 gate.

- M567 tests whether the existing qrels-free dense-order pairwise loss can fix
  M566's MRR instability.
- M568 tests a qrels-free conservative source-selection rule: accept the learned
  source only when it preserves `dense_overlap_at_100` versus
  `dense_topk128_sparse`; otherwise fall back to `dense_topk128_sparse`.

## M567 Pairwise Probe

M567 used the M555 pairwise-order runner on the same official-1024 BEIR7
surface:

- `PAIRWISE_WEIGHT=0.20`
- same promoted M551 temperatures and residual scale
- same task list as M566 BEIR7

Run outputs:

- `runs/m567_pairwise_official1024_beir7_seed551/m567_pairwise_w0.20_official1024_beir7_seed551.json`
- `runs/m567_pairwise_official1024_beir7_seed553/m567_pairwise_w0.20_official1024_beir7_seed553.json`

Seed551 comparison:

| Metric | M566 Delta | M567 Delta | M567 - M566 |
| --- | ---: | ---: | ---: |
| NDCG@10 | +0.005790 | +0.006310 | +0.000520 |
| MAP@100 | +0.004250 | +0.004380 | +0.000130 |
| Recall@100 | +0.005600 | +0.005730 | +0.000130 |
| MRR@20 | +0.003340 | +0.003290 | -0.000050 |
| Overlap@100 | +0.002400 | +0.002760 | +0.000360 |

Seed553 comparison:

| Metric | M566 Delta | M567 Delta | M567 - M566 |
| --- | ---: | ---: | ---: |
| NDCG@10 | -0.002470 | -0.002160 | +0.000310 |
| MAP@100 | +0.003000 | +0.003280 | +0.000280 |
| Recall@100 | +0.005960 | +0.005890 | -0.000070 |
| MRR@20 | -0.005980 | -0.005780 | +0.000200 |
| Overlap@100 | -0.000820 | -0.000650 | +0.000170 |

Interpretation: pairwise is not worth expanding here.  It gives tiny positive
movement on NDCG/MAP/overlap, but it does not repair the bad-seed MRR problem.
For example, `webis-touche2020` seed553 remains at `dMRR=-0.03695`.

## M568 Conservative Dense-Overlap Gate

M568 is a post-hoc qrels-free selection probe over M566.  It uses the learned
M551-family source only if the task's `dense_overlap_at_100` is not lower than
the `dense_topk128_sparse` baseline.

Generated outputs:

- `outputs/m568_conservative_overlap_gate/m568_conservative_overlap_gate.json`
- `outputs/m568_conservative_overlap_gate/m568_conservative_overlap_gate.md`

Mean delta over M566 seeds:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M566 learned model | +0.002207 | +0.002997 | +0.003792 | -0.001533 | +0.001149 |
| M568 overlap-gated | +0.003528 | +0.003074 | +0.002938 | +0.000899 | +0.001981 |

Gate decisions:

| Seed | Accepted Tasks | Fallback Tasks |
| ---: | --- | --- |
| 551 | nfcorpus, fiqa, scidocs, scifact, trec-covid, webis-touche2020 | arguana |
| 552 | nfcorpus, fiqa, scidocs, scifact, trec-covid | arguana, webis-touche2020 |
| 553 | fiqa, scidocs, scifact | arguana, nfcorpus, trec-covid, webis-touche2020 |

Interpretation: the overlap gate is much more promising than pairwise tuning.
It turns M566's mean MRR delta from negative to positive while preserving
positive NDCG, MAP, Recall, and overlap.  The next production-shaped step should
make this teacher-surface risk gate explicit in the source-selection pipeline,
then re-evaluate before adding `cqadupstack`.
