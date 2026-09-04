# SAE M50 Hard-Dataset Semantic Retention Plan

Status: active.

## Motivation

The current M46/M49 selector clearly improves the full15 aggregate over BM25,
but two hard datasets still regress on ranking quality:

| Dataset | Recall@100 delta | NDCG@10 delta | MAP@100 delta |
| --- | ---: | ---: | ---: |
| `trec-covid` | +0.002659 | -0.023116 | -0.006017 |
| `msmarco` | +0.023504 | -0.044267 | -0.018161 |

Both datasets gain recall but lose top-rank quality. This suggests the SAE path
is finding extra relevant candidates, but the exported semantic evidence and
score calibration are not strong or stable enough to preserve the right order.

## Hypothesis

The first hypothesis is semantic-retention loss, not model-capacity failure:

```text
current encoder output is useful
but export/profile selection keeps too few ranking-critical semantic atoms
for broad/high-fanout corpora
```

If that is true, increasing the query atom pool and export budget should improve
`trec-covid` and `msmarco` without retraining. If larger export budgets do not
repair NDCG/MAP, the blocker is not merely retained semantic capacity; the next
step must be objective-level training for hard-query ranking.

## M50.1 Targeted Semantic Retention Sweep

Run a focused sweep on only:

```text
trec-covid
msmarco
```

Sweep:

```text
pool_active_dims   = 96,128,160,192
export_active_dims = 48,64,80,96
fanout_power       = 0.00,0.05,0.10,0.15,0.20,0.25
df_threshold       = 0.08,0.10,0.12,0.15
low_sae_weight     = 0.30,0.35,0.40,0.45,0.55
high_sae_weight    = 0.75,1.00,1.25
```

Output:

```text
results/sae/m50/semantic-retention-target-sweep/
```

Acceptance:

- NDCG@10 and MAP@100 should improve versus current M46 on both datasets.
- Prefer rows that also beat BM25 on NDCG@10 and MAP@100.
- Candidate docs and SAE postings can increase in this diagnostic phase, but
  the report must show the cost.

## M50.2 Decision Rule

If a larger export profile repairs both datasets:

```text
promote semantic-retention selector research
```

The next selector can route high semantic-retention queries to a larger export
profile, but must keep M46 as the default for the rest of full15.

If no larger export profile repairs both datasets:

```text
park pure export expansion
move to hard-query ranking training
```

That training should keep the M46/M49 physical contract but add explicit
ranking preservation for BM25-strong and broad semantic-neighborhood queries.

## Non-Goals

- Do not introduce dataset-id routing.
- Do not freeze SQL/API.
- Do not claim dense-removal product readiness.
- Do not use this two-dataset run as a full15 product gate.
