# SAE M30 Robust Query-Side Calibration Results Report

Status: M30 completed; query-side calibration improved diagnosis but did not
pass the product-research robustness gate.

M30 tested whether the M29 blocker could be solved without a deeper encoder:
keep the canonical Snowflake prefixed teacher, keep document atoms fixed to the
teacher path, and make query-side training more conservative by using
teacher-anchored residual ranking and a small runtime-safe calibration head.

The result is negative for product gating. M30 reduced some collapse severity
when the M29 Arm A query encoder was frozen and a lower fixed SAE scale was
selected, but learned calibration did not pass the full15 no-collapse gate.
Doc-side training, SQL/API productization, mutable index work, and
dense-removal claims remain blocked.

## Executive Decision

| Area | Decision | Evidence |
| --- | --- | --- |
| Collapse taxonomy | `bm25_suppression-dominant` | `120` collapsed focus queries were classified as BM25 suppression, versus `38` fanout/entropy anomalies and `5` SAE over-promotion cases |
| Residual-only training | `failed-quality` | best source `m30_fixed_w0p5`; full15 gaps versus teacher were Recall@100 `-0.0384`, MRR@20 `-0.0280`, NDCG@10 `-0.0527`, MAP@100 `-0.0708` |
| Residual + calibration head | `failed-quality` | learned scale improved residual-only but still missed teacher by Recall@100 `-0.0303`, NDCG@10 `-0.0456`, MAP@100 `-0.0616` |
| Frozen encoder + calibration head | `failed-robustness` | fixed `w0.5` kept strong aggregate quality but still collapsed on `trec-covid`, `msmarco`, and `dbpedia-entity` |

Final M30 label:

```text
query-side calibration helps but does not close robustness blocker
```

## M30.0 Collapse Taxonomy

M30 first analyzed the failed M29 Arm A strong-qrel run on the known collapse
datasets: `trec-covid`, `msmarco`, and `dbpedia-entity`.

| Category | Queries | Mean NDCG delta | Mean MAP delta |
| --- | ---: | ---: | ---: |
| `bm25_suppression` | 120 | -0.1893 | -0.2665 |
| `fanout_entropy_anomaly` | 38 | -0.0966 | -0.1725 |
| `sae_over_promotion` | 5 | -0.2343 | -0.1995 |
| `teacher_qrel_conflict` | 1 | 0.2667 | -0.1257 |

Interpretation:

- The main failure is not raw SAE over-promotion alone.
- The brittle model often suppresses BM25/teacher-supported positives after
  qrel-heavy query-side training.
- A safe M30 fix therefore had to preserve BM25 calibration, not increase
  semantic atom pressure.

Artifact:

```text
results/sae/m30/collapse-taxonomy/m30_collapse_taxonomy.md
```

## Full15 Quality Matrix

All M30 query-side runs keep document atoms fixed to teacher doc atoms. The
gate compares each run against `teacher_fixed_doc`.

| Run | Best source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| M29 Arm A strong qrel baseline | `w0p75` | 0.8673 | 0.9080 | 0.7872 | 0.7536 | failed robustness |
| M30 residual only | `w0p5` | 0.8063 | 0.8120 | 0.6963 | 0.6539 | failed quality |
| M30 residual + calibration head | `calibrated` | 0.8144 | 0.8160 | 0.7034 | 0.6631 | failed quality |
| M30 frozen encoder + calibration head | `w0p5` | 0.8649 | 0.8847 | 0.7737 | 0.7415 | failed robustness |
| Teacher fixed-doc reference | `teacher_fixed_doc` | 0.8447 | 0.8399 | 0.7490 | 0.7247 | reference |

The frozen-encoder run is the only M30 variant that stays near the M29 aggregate
frontier. It does so by not changing the query atoms. Its learned calibration
head was not selected; collapse-aware model selection preferred fixed `w0.5`.

## Gap To Teacher

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M29 Arm A strong qrel baseline | +0.0226 | +0.0681 | +0.0382 | +0.0289 |
| M30 residual only | -0.0384 | -0.0280 | -0.0527 | -0.0708 |
| M30 residual + calibration head | -0.0303 | -0.0240 | -0.0456 | -0.0616 |
| M30 frozen encoder + calibration head | +0.0202 | +0.0448 | +0.0247 | +0.0167 |

The aggregate-only view is misleading. Both M29 strong qrel and M30 frozen
calibration look strong in the mean but fail per-dataset collapse constraints.

## Physical Matrix

The query-side runs use the same fixed teacher document payload. M30 reports
candidate and postings cost from the Python sparse harness; native payload MB
and cached PostgreSQL latency were not rerun because M30 is explicitly not an
SQL/API productization milestone.

| Run | Candidate docs | BM25 postings | SAE postings | Payload |
| --- | ---: | ---: | ---: | --- |
| M29 Arm A strong qrel baseline | 2860.3 | 10185.0 | 1738.0 | fixed teacher-doc payload |
| M30 residual only | 2679.5 | 10185.0 | 248.9 | fixed teacher-doc payload |
| M30 residual + calibration head | 2706.9 | 10185.0 | 491.6 | fixed teacher-doc payload |
| M30 frozen encoder + calibration head | 2860.3 | 10185.0 | 1738.0 | fixed teacher-doc payload |
| Teacher fixed-doc reference | 2986.2 | 10185.0 | 4522.8 | fixed teacher-doc payload |

Cost did not regress versus the M29 Arm A profile. The failure is ranking
robustness, not physical cost.

## Collapse Gate

M30 acceptance required no dataset NDCG/MAP collapse worse than `-0.025` versus
teacher. The best M30 robustness candidate still fails:

| Dataset | Metric | M29 delta | M30 frozen delta |
| --- | --- | ---: | ---: |
| `dbpedia-entity` | `map@100` | -0.0869 | -0.0579 |
| `msmarco` | `ndcg@10` | -0.1569 | -0.1119 |
| `msmarco` | `map@100` | -0.1512 | -0.0888 |
| `trec-covid` | `ndcg@10` | -0.3134 | -0.1889 |
| `trec-covid` | `map@100` | -0.4350 | -0.3177 |

M30 improves collapse severity but remains far outside the gate.

## Interpretation

M30 closes a useful negative branch:

- Simply anchoring to teacher and using qrels as residual constraints is too
  weak.
- Training the Arm A encoder further under the M30 residual objective damages
  the already-strong aggregate ranking signal.
- A tiny runtime-safe calibration head can learn conservative SAE scales, but
  it does not learn enough per-query structure to remove dataset collapses.
- Frozen Arm A plus lower fixed SAE scale gives the best robustness tradeoff,
  but it is still a model-selection workaround, not a product model.

The blocker is therefore not solved by scalar calibration. The next useful
model step needs richer query understanding or richer supervision that can
distinguish when qrels should override teacher/BM25 and when they should not.

## Exit Decision

M30 does not pass the fixed-doc query-side gate:

- Recall/MRR/NDCG/MAP aggregate can be strong.
- The no-collapse requirement fails on `trec-covid`, `msmarco`, and
  `dbpedia-entity`.
- Doc-side training remains blocked.
- Direct dense-removal remains blocked.
- Teacher-path read-only harness remains allowed only as a research evaluation
  harness.

Recommended next phase:

```text
M31 should stop scalar-calibration sweeps and try richer supervision or a
stronger query representation. The minimum viable next attempt is not a bigger
generic encoder by default; it is a query-family-aware ranking model trained
with explicit BM25-preservation constraints and held-out-family validation.
```

## Artifacts

```text
scripts/research_sae_m30_collapse_taxonomy.py
scripts/research_sae_m30_query_calibration_train.py
results/sae/m30/collapse-taxonomy/m30_collapse_taxonomy.json
results/sae/m30/collapse-taxonomy/m30_collapse_taxonomy.md
results/sae/m30/query-calibration-residual/m30_query_calibration_train.json
results/sae/m30/query-calibration-head/m30_query_calibration_train.json
results/sae/m30/query-calibration-head-frozen/m30_query_calibration_train.json
```
