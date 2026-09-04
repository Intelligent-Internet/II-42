# M514 Load-Balanced Route LoRA Report

M514 tests whether an explicit coordinate-load objective can improve the M512/M513
route frontier.  It keeps the same no-BM25/no-qrels training setup and adds
soft coordinate losses during the route phase:

- preserve positive query-doc coordinate overlap;
- suppress negative query-doc coordinate overlap;
- penalize concentrated document coordinate load.

## Run

| Run | Host | Task | Route docs | Presets | Output |
| --- | --- | --- | ---: | --- | --- |
| `fiqa_load_balanced_route_lora` | `spark-1` | `FiQA2018` | `4096/57638` | `baseline,load_light,load_medium,load_preserve` | `outputs/m514/fiqa_load_balanced_route_lora/m514_fiqa_load_balanced_route_lora.json` |

This is a route-subset gate, not a full-corpus BEIR score.  The subset includes
qrels positives and dense teacher neighbours for heldout eval queries.

The run includes an internal same-run baseline because the live spark-1 Python
environment produced a different base loss scale than the older M512/M513
artifacts.  The M514 decision therefore uses only same-run comparisons.

## Result Matrix

| Preset | Prefix | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline` | 64 | 0.46533 | 0.84751 | 0.57371 | 0.41287 | 0.82344 | 0.51522 |
| `baseline` | 96 | 0.46636 | 0.87786 | 0.57180 | 0.41645 | 0.89047 | 0.60588 |
| `baseline` | 128 | 0.46747 | 0.88620 | 0.57175 | 0.41862 | 0.92859 | 0.67009 |
| `load_light` | 64 | 0.44176 | 0.79349 | 0.52814 | 0.39137 | 0.74734 | 0.58229 |
| `load_light` | 96 | 0.47147 | 0.86146 | 0.57346 | 0.41830 | 0.86906 | 0.72038 |
| `load_light` | 128 | 0.46931 | 0.88359 | 0.57267 | 0.41779 | 0.93750 | 0.81672 |
| `load_medium` | 64 | 0.36924 | 0.61235 | 0.49640 | 0.31475 | 0.56047 | 0.59588 |
| `load_medium` | 96 | 0.43203 | 0.78136 | 0.53852 | 0.38181 | 0.71219 | 0.74205 |
| `load_medium` | 128 | 0.44427 | 0.82708 | 0.53117 | 0.39751 | 0.81328 | 0.83771 |
| `load_preserve` | 64 | 0.36763 | 0.61856 | 0.48276 | 0.31550 | 0.58969 | 0.62218 |
| `load_preserve` | 96 | 0.43258 | 0.76391 | 0.55145 | 0.37535 | 0.74906 | 0.77239 |
| `load_preserve` | 128 | 0.46374 | 0.81042 | 0.58210 | 0.41043 | 0.84484 | 0.85907 |

## Training Signal Check

The load objective did affect the internal load metric:

| Preset | Load trace | Positive overlap trace | Negative overlap trace |
| --- | ---: | ---: | ---: |
| `baseline` | 313.32152 | 1.64007 | 0.24290 |
| `load_light` | 237.43989 | 1.18577 | 0.19957 |
| `load_medium` | 121.00995 | 2.62447 | 0.10685 |
| `load_preserve` | 174.04683 | 1.99733 | 0.13689 |

So the loss is not a no-op.  The issue is that reducing global coordinate load
does not preserve the useful posting candidates.  Stronger load pressure kills
candidate recall; weaker load pressure can improve quality but raises Touch.

## Verdict

M514 does not beat the same-run baseline frontier.

- Best quality point: `load_light` p96 has NDCG@10 0.47147, but candidate recall
  falls to 0.86906 and Touch rises to 0.72038.
- Best candidate preservation among load variants: `load_light` p128 reaches
  Candidate R@100 0.93750, but Touch rises to 0.81672.
- Best compression point remains the same-run baseline p128/p96 tradeoff.

This rejects further scalar load-balance tuning as the next main path.  The
better next step is query-adaptive prefix/fanout: keep the learned support shape
and select p64/p96/p128 per query.
