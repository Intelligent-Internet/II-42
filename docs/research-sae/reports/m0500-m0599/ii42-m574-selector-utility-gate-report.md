# M574 Selector Utility Gate

M573 made the selector split conservative enough to avoid the Recall regression
seen in M570/M572, but the two-seed overlap gain was almost flat.  M574 keeps
the M573 qrels-free gate and adds a third selector utility constraint.

The gate accepts the learned source only when all three selector-side signals
pass:

- `overlap_at_200` delta is at least `+0.002`;
- `candidate_coverage_at_100` delta is at least `+0.002`;
- `rank_teacher_mass_at_100` delta is nonnegative.

`rank_teacher_mass_at_100` is computed without qrels.  For each selector query,
the dense teacher top-200 documents are converted into a softmax distribution
using the selector utility temperature.  The source receives credit for the
teacher probability mass captured by its own top-100 sparse ranking.  This adds
a distribution-sensitive signal on top of set overlap and candidate coverage.

## Smoke

Remote smoke:

- host: `spark-1`
- run: `runs/m574_smoke_scidocs_seed574/`
- task: `SCIDOCS`
- limits: `QUERY_LIMIT=120`, `DOC_LIMIT=2000`, `EPOCHS=1`

The smoke completed and verified the new JSON schema.  The learned source
improved `rank_teacher_mass_at_100` by `+0.005106`, but failed the M573 support
constraints:

| Signal | Delta |
| --- | ---: |
| `overlap_at_200` | +0.001041 |
| `candidate_coverage_at_100` | -0.002500 |
| `rank_teacher_mass_at_100` | +0.005106 |

The guarded source correctly fell back to `dense_topk128_sparse`.  This is the
desired M574 behavior: utility mass alone is not allowed to override support
regression.

## Formal Validation

Completed on the M566 official1024 shared root:

- seed574: `runs/m574_beir7_seed574/`
- seed575: `runs/m574_beir7_seed575/`
- run dir: `/home/huoju/leask/runs/ii42-m574-selector-utility-gate-v1`

The validation target is the same BEIR7 surface used by M573:
`arguana`, `nfcorpus`, `fiqa`, `scidocs`, `scifact`, `trec-covid`,
`webis-touche2020`.

Formal deltas versus `dense_topk128_sparse`:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 574 | learned | +0.002120 | +0.000390 | +0.000560 | +0.011090 | +0.003770 |
| 574 | guarded | +0.000350 | +0.000000 | +0.000000 | +0.000060 | +0.000600 |
| 575 | learned | -0.006740 | -0.000260 | +0.004840 | -0.015200 | +0.002680 |
| 575 | guarded | -0.006620 | -0.000550 | +0.002280 | -0.014900 | +0.002300 |
| 574-575 mean | guarded | -0.003135 | -0.000275 | +0.001140 | -0.007420 | +0.001450 |

Guarded accepted tasks:

| Seed | Accepted learned source |
| --- | --- |
| 574 | `scifact` |
| 575 | `scifact`, `trec-covid`, `webis-touche2020` |

M574 is not promotable.  The utility gate did not prevent seed575 from accepting
`scifact`, `trec-covid`, and `webis-touche2020`, where qrels evaluation showed
negative NDCG/MRR despite positive selector overlap, coverage, and teacher mass.
This is direct evidence that simple selector teacher-mass preservation does not
reliably predict final early-rank relevance.

## M575 Gate Sweep Follow-up

The post-hoc M575 sweep over the completed M574 runs is saved in:

- `outputs/m575_selector_gate_sweep/m575_selector_gate_sweep.json`
- `outputs/m575_selector_gate_sweep/m575_selector_gate_sweep.md`

Best qrels-free rule on seed574/575:

- `overlap@100 >= +0.004`
- `candidate_coverage@100 >= +0.002`
- no utility constraint
- `min_selector_queries >= 20`

Two-seed mean deltas for this rule:

| dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: |
| +0.000370 | +0.000556 | +0.001078 | +0.000736 | +0.000452 |

This is stable but too small to be a meaningful next milestone.  The practical
conclusion is that selector-gate micro-tuning is close to exhausted.  The next
useful stage should change the encoder/training signal while keeping M551/M573
as the safety baseline.
