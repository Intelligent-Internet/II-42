# M1175 Oracle Tail Rank-Teacher Ceiling

M1175 measures the upper bound of a perfect selector that applies `tail_full`
only on M1174 `tail_rank_teacher` rows and keeps protected direct action on all
other rows.

Artifacts:

- Script: `scripts/audit_m1175_oracle_tail_rank_teacher_ceiling.py`
- JSON: `runs/m1175_oracle_tail_rank_teacher_ceiling_v1/oracle_tail_rank_teacher_ceiling.json`
- Run summary: `runs/m1175_oracle_tail_rank_teacher_ceiling_v1/summary.md`

## Result

| View | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| direct_protect | +0.000721 | +0.013801 | +0.013618 | +0.008751 | +0.000000 |
| oracle_rank_teacher | +0.000721 | +0.013801 | +0.025819 | +0.035452 | +0.033791 |
| oracle_minus_direct | +0.000000 | +0.000000 | +0.012201 | +0.026700 | +0.033791 |

The oracle chooses 51/138 rows and is row-floor clean across the selected
surface.  It preserves CUB/Recall and adds substantial MAP/NDCG/MRR.

Selected movement:

- Relevant rank gains: `466`.
- Relevant rank losses: `187`.
- Relevant entrants: `0`.
- Relevant exits: `0`.

## Dataset Read

The ceiling is positive on all datasets where rows are selected:

- `arguana`: dMAP +0.110648, dNDCG +0.214306, dMRR +0.129167.
- `dbpedia-entity`: dMAP +0.013870, dNDCG +0.028445, dMRR +0.040693.
- `fiqa`: dMAP +0.250000, dNDCG +0.184535, dMRR +0.250000.
- `msmarco`: dMAP +0.004662, dNDCG +0.006115.
- `nfcorpus`: dMAP +0.003089, dNDCG +0.006114.
- `trec-covid`: dMAP +0.000852, dNDCG +0.033875, dMRR +0.052083.

## Interpretation

This is the first clean signal after the tail-router failures:

- `tail_full` should not be used as raw runtime route.
- `tail_full` should not be treated as a Recall boundary expansion teacher.
- `tail_full` can be used as a ranking teacher if the objective learns only
  safe rank movements and penalizes harm rows.

This result is still oracle-only.  The labels use qrels/metric movement, so the
next step must test deployable approximation:

1. Can native/query-time features identify enough of these 51 rows without
   selecting M1174 `tail_harm` rows?
2. If row selection is weak, can a rank-distillation objective learn the
   positive pair/listwise movement directly while applying explicit harm
   penalties?
3. If neither approximation works, stop the tail-rank teacher branch.
