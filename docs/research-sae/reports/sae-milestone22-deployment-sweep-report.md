# SAE M22 Deployment Sweep Report

This report keeps quality and physical sparse cost in one place. Quality rows are full15 apples-to-apples eval-checkpoint results. Physical rows are native payload C benchmark measurements for the same active-row deployment points.

## Full15 Quality

| Active run | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `doc-80-query-96` | 0.7912 | 0.8069 | 0.6847 | 0.6385 |
| `doc-88-query-96` | 0.7905 | 0.8087 | 0.6862 | 0.6402 |
| `doc-96-query-96` | 0.7908 | 0.8088 | 0.6850 | 0.6398 |

## Physical Active8 Cost

| Active run | Recall@100 | NDCG@10 | MAP@100 | Candidates | SAE posts | BM25 posts | Payload MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `doc-80-query-96` | 0.7728 | 0.6643 | 0.5986 | 786.6 | 775.9 | 677.5 | 7.02 |
| `doc-88-query-96` | 0.7735 | 0.6668 | 0.5994 | 792.7 | 785.8 | 677.5 | 7.33 |
| `doc-96-query-96` | 0.7751 | 0.6652 | 0.5989 | 797.2 | 793.7 | 677.5 | 7.65 |

## Full15 Active-Budget Sweep

The same checkpoint was evaluated with multiple query/document active-row shapes. Each value is the best student-weight result for that metric.

| Doc active | Query active | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 80 | 64 | 0.7875 | 0.8037 | 0.6822 | 0.6364 |
| 80 | 80 | 0.7884 | 0.8033 | 0.6817 | 0.6365 |
| 80 | 96 | 0.7912 | 0.8069 | 0.6847 | 0.6385 |
| 88 | 64 | 0.7881 | 0.8063 | 0.6832 | 0.6370 |
| 88 | 80 | 0.7889 | 0.8064 | 0.6831 | 0.6378 |
| 88 | 96 | 0.7905 | 0.8087 | 0.6862 | 0.6402 |
| 96 | 64 | 0.7892 | 0.8083 | 0.6829 | 0.6374 |
| 96 | 80 | 0.7889 | 0.8068 | 0.6829 | 0.6383 |
| 96 | 96 | 0.7908 | 0.8088 | 0.6850 | 0.6398 |
| 128 | 64 | 0.7898 | 0.8084 | 0.6826 | 0.6363 |
| 128 | 80 | 0.7915 | 0.8077 | 0.6819 | 0.6364 |
| 128 | 96 | 0.7913 | 0.8093 | 0.6836 | 0.6366 |

Interpretation:

- Query 96 remains the safest default. Query 64/80 are useful cost-biased controls, but they lose enough quality that they should not become the default.
- Doc128 does not justify its larger payload: it wins tiny Recall/MRR deltas but loses NDCG/MAP against doc88/query96.
- doc88/query96 remains the best ranking/cost point until a new training loss produces a better Pareto frontier.

## Interpretation Rules

- Do not promote a row on Recall@100 alone.
- Prefer a row only if ranking quality and physical cost both move in the same direction.
- Treat five-dataset smoke results as directional; this report uses full15 quality for promotion decisions.
