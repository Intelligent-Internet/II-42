# ii42 M337 Candidate-K Sweep Report

## Summary

M337 tests whether the M335/M336 broad-8 bottleneck is the learned scorer or
the candidate admission policy. It reuses the existing M334/M335 score-map cache
and varies only candidate admission depth.

This is not a training run. It is a cheap ceiling diagnostic before designing
the next learned-admission stage.

## Inputs

- Dataset root:
  `/home/huoju/leask/runs/ii42-m180a-stagea-gate-latest-full-v1/all-test`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m334-interaction-candidate-cache-v1`
- Output root:
  `/home/huoju/leask/runs/ii42-m337-candidate-k-sweep-v1`
- Local copy:
  `/tmp/ii42-m337-candidate-k-sweep-v1`
- Variant: `df_le_0p25`
- Candidate K sweep: `320`, `640`, `1000`
- Unified candidate scales: `0.25`, `0.5`
- Seed / split seed: `1050` / `1050`

## Results

| Dataset | K | Candidate hit | Candidate qrel recall | Cache qrel recall |
| --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | 320 | 0.6536 | 0.4825 | 0.4825 |
| `cqadupstack` | 640 | 0.6536 | 0.4825 | 0.4825 |
| `cqadupstack` | 1000 | 0.6536 | 0.4825 | 0.4825 |
| `fiqa` | 320 | 0.8093 | 0.6345 | 0.9979 |
| `fiqa` | 640 | 0.8711 | 0.6858 | 0.9979 |
| `fiqa` | 1000 | 0.8763 | 0.7125 | 0.9979 |
| `nfcorpus` | 320 | 0.8454 | 0.2679 | 0.5431 |
| `nfcorpus` | 640 | 0.8763 | 0.3208 | 0.5431 |
| `nfcorpus` | 1000 | 0.8763 | 0.3704 | 0.5431 |
| `scidocs` | 320 | 0.8567 | 0.4767 | 0.9804 |
| `scidocs` | 640 | 0.9033 | 0.5382 | 0.9804 |
| `scidocs` | 1000 | 0.9267 | 0.5713 | 0.9804 |
| `trec-covid` | 320 | 1.0000 | 0.1543 | 0.9931 |
| `trec-covid` | 640 | 1.0000 | 0.2123 | 0.9931 |
| `trec-covid` | 1000 | 1.0000 | 0.2517 | 0.9931 |
| `webis-touche2020` | 320 | 1.0000 | 0.6620 | 1.0000 |
| `webis-touche2020` | 640 | 1.0000 | 0.7561 | 1.0000 |
| `webis-touche2020` | 1000 | 1.0000 | 0.7944 | 1.0000 |

## Interpretation

The current fixed top160 admission policy is too aggressive for multi-positive
datasets. Increasing K improves qrel coverage on `fiqa`, `nfcorpus`,
`scidocs`, `trec-covid`, and `webis-touche2020`, but it does not close the
gap to cache-level qrel recall.

This means the score maps often already contain the needed evidence, but the
admission policy does not select it efficiently into a compact candidate pool.

`cqadupstack` behaves differently: K does not help because cache qrel recall is
already the limiting ceiling. That dataset needs a deeper source surface, not a
larger candidate K over the current cache.

## Decision

M335/M337 should not be treated as a failed direction. The useful next step is
M338: learn a compact admission policy from deep score-map features, then feed
that candidate pool into the existing interaction scorer.

M338 should optimize admission/coverage directly:

- positives are qrel docs present in the score map;
- negatives are high-scoring non-qrel docs from BM25, atom, and unified score
  maps;
- objective is recall-oriented admission at a fixed budget, followed by
  top-rank scoring;
- evaluate against fixed K and cache-oracle ceilings before any expensive
  encoder retraining.
