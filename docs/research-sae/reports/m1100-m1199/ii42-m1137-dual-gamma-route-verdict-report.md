# ii42 M1137 Dual-Gamma Route Verdict

## Status

M1137 completed the full shared15 replay on spark-1.

- Run dir:
  `/home/huoju/leask/runs/ii42-m1137-dual-gamma-shared15-v1`
- Local mirror:
  `/Volumes/Betty/Tmp/ii42-m1000/m1137-dual-gamma-shared15-v1`
- Datasets: 15/15 shared15 rows completed.
- Training pairs: `base_pairs=6052`, `semantic_pairs=0`.
- Training loss:
  - epoch 1: `2.017708`
  - epoch 4: `0.228948`
- Checkpoint:
  `m1137_dual_gamma_shared15_s1050.pt`
- Final JSON:
  `m1137_dual_gamma_shared15_s1050.json`
- Ranking exports: 15/15 candidate-score and query JSONL files.

## Objective

M1137 tested whether the shared5/shared8 positive signal from dual-gamma
training generalizes to the broader shared15 surface.

The training configuration kept the native/inference posting score linear while
using a concave score only inside the listwise loss:

- `POSTING_SCORE_GAMMA=1.00`
- `LISTWISE_SCORE_GAMMA=0.50`
- `LISTWISE_RANK_WEIGHT=0.50`
- `TOP_RANK_PRESERVE_WEIGHT=0.0`
- `SEMANTIC_PAIRS_ENABLED=0`

The intent was to keep coverage from the linear native score while getting a
better training gradient from the concave listwise score.

## Shared15 Results

### Additive alpha

`docs/research-sae/reports/m1100-m1199/ii42-m1137-shared15-dual-gamma-additive-alpha-fine-grid-report.md`

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| heldout-best alpha 0.40 | 0.766031 | 0.756179 | 0.691217 | 0.589373 |
| alpha 0.50 | 0.771768 | 0.752251 | 0.688795 | 0.587888 |
| alpha 0.60 | 0.775801 | 0.747724 | 0.687443 | 0.583750 |
| alpha 1.00 | 0.785097 | 0.706468 | 0.652522 | 0.550436 |

Verdict: additive dual-gamma is not promotable. Higher alpha buys Recall but
spends MRR, NDCG, and MAP.

### Score-shape transform

`docs/research-sae/reports/m1100-m1199/ii42-m1137-shared15-dual-gamma-score-shape-transform-report.md`

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| selected `a0.75_g0.75` | 0.785958 | 0.744507 | 0.683239 | 0.581610 |
| heldout-best `a0.50_g0.50` | 0.782350 | 0.762162 | 0.698423 | 0.598732 |
| fixed `a0.50_g1.00` | 0.771768 | 0.752251 | 0.688795 | 0.587888 |
| fixed `a0.60_g1.00` | 0.775801 | 0.747724 | 0.687443 | 0.583750 |

The best score-shape row recovers most of the NDCG loss and improves Recall,
but it still does not preserve the M1129 shared15 rank metrics.

## Baseline Comparison

Current M1129 shared15 fixed-alpha baseline:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M1129 fixed alpha 0.60 | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| M1137 best score-shape | 0.782350 | 0.762162 | 0.698423 | 0.598732 |
| Delta | +0.009952 | -0.017610 | -0.001542 | -0.007453 |

M1137 improves Recall@100 but fails the promotion gate because MRR@20,
NDCG@10, and MAP@100 regress. This is the same failure pattern seen in the
direct additive grid: broader coverage is bought by damaging top-rank quality.

## Cross-Surface Interpretation

| Run | Surface | Main signal |
| --- | --- | --- |
| M1133 | shared5 | direct concave listwise improved rank signal but lost Recall |
| M1134 | shared5 | top-rank preserve did not rescue concave inference |
| M1135 | shared5 | dual-gamma improved rank metrics but kept a Recall tradeoff |
| M1136 | shared8 | score-shape `a0.60_g0.50` beat M1128 on all macro metrics |
| M1137 | shared15 | score-shape improved Recall but did not preserve rank metrics |

The route has real local signal, but it does not generalize cleanly from
shared8 to shared15. It is not just an alpha-selection issue: the shared15
heldout-best score-shape still has the same Recall-vs-rank tradeoff.

## Verdict

Stop the dual-gamma route as a promotion candidate.

Preserve the code path and reports as diagnostics:

- `--posting-score-gamma`
- `--listwise-score-gamma`
- `SEMANTIC_PAIRS_ENABLED=0`

Do not continue by sweeping more gamma/alpha combinations. The evidence says
the route is redistributing score mass in a way that helps top100 coverage but
weakens the top-rank geometry that drives MRR, NDCG, and MAP.

## Next Step

Return to M1129 as the current shared15 training baseline and audit the
M1129-vs-M1137 query-level differences. The next useful question is not
"which gamma is better", but:

1. Which datasets and query profiles produce M1137-only Recall gains?
2. Which datasets and query profiles pay MRR/MAP damage?
3. Are the gains tied to recoverable candidate evidence, or to a broad score
   flattening that cannot be safely selected?

If the M1137-only gains are separable from its rank damage, use them as a
teacher signal for a constrained policy. If they are not separable, keep M1129
fixed-alpha as the current baseline and move to a different evidence source.
