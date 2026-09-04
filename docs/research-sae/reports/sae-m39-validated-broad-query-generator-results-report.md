# SAE M39 Validated Broad-Query Generator Results Report

Status: closed after one validated generator and one training run.

## Summary

M39 tested whether a distribution-matched broad-query generator can solve the
M36/M38 blocker.

Result:

```text
M39 does not pass the full robustness gate.
```

However, unlike M38, it produces a useful partial signal. At lower fixed SAE
weight, M39 improves `trec-covid` MAP beyond M36/M38/M37:

```text
M39 fixed_w0p25 trec-covid: NDCG@10 0.6594, MAP@100 0.4822
```

This is not promotable because aggregate quality and other hard datasets drop
at that weight. But it is evidence that the validated broad-query generator is
closer to the missing supervision than prior M35/M38 routes.

## Artifact Validation

Artifact:

```text
/Volumes/Betty/Tmp/ii42_sae_m39_broad_query_generator/m39-broad-neighborhood-q200
```

Generated dataset:

```text
m39-trec-covid-broad-neighborhood
```

Generation stats:

| Metric | Value |
| --- | ---: |
| Candidate queries | 184,599 |
| Selected queries | 200 |
| Teacher qrel pairs | 20,000 |
| Mean content DF | 0.1966 |
| Min content DF | 0.1844 |
| Max content DF | 0.2119 |
| Top-term share | 0.0770 |

M36 validation accepted the artifact:

| Artifact | Dataset | Decision | Queries | Mean content DF | Long share | Duplicate-like rate | Top term | Top term share | Failures |
| --- | --- | --- | ---: | ---: | ---: | ---: | --- | ---: | --- |
| `m39` | `m39-trec-covid-broad-neighborhood` | `passed_validation` | 200 | 0.1985 | 1.0000 | 0.0000 | `evidence` | 0.0750 | `none` |

Validation decision:

```text
synthetic_training_allowed
```

This is a meaningful improvement over M35/M35b, which failed due to content DF
mismatch and/or low diversity.

## Training Scope

Training root:

```text
/Volumes/Betty/Tmp/ii42_sae_m39_train_merged/m39-broad-plus-nfcorpus-expanded
```

Training data:

| Metric | Value |
| --- | ---: |
| Train datasets | 9 |
| Train documents | 146,353 |
| Train queries | 4,981 |
| Train qrel pairs | 146,212 |
| Eval datasets | 15 |
| Eval queries | 1,342 |

Hard-bucket reweighting was off:

```text
mean example weight = 1.000
```

This isolates the generated query-distribution signal.

## Full15 Matrix

| Run | Best | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidate docs | SAE postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | `m31_fixed_w0p5` | 0.8694 | 0.8780 | 0.7734 | 0.7449 | 2964.8 | 2778.6 |
| M36 nfcorpus-expanded | `m31_fixed_w0p5` | 0.8671 | 0.8795 | 0.7731 | 0.7429 | 2923.6 | 2384.2 |
| M38 hard-bucket | `m31_fixed_w0p5` | 0.8670 | 0.8794 | 0.7730 | 0.7432 | 2925.6 | 2399.9 |
| M39 broad-generator | `m31_fixed_w0p5` | 0.8670 | 0.8813 | 0.7707 | 0.7435 | 2922.9 | 2430.6 |

The collapse-aware best source remains `m31_fixed_w0p5`. It is not better than
M36 in aggregate and slightly increases SAE postings.

## Hard Dataset Table

### Collapse-Aware Best: `m31_fixed_w0p5`

| Dataset | Recall@100 | NDCG@10 | MAP@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| `trec-covid` | 0.1406 | 0.6332 | 0.4559 | -0.2019 | -0.3080 |
| `msmarco` | 0.7937 | 0.6242 | 0.8841 | -0.1007 | -0.0512 |
| `dbpedia-entity` | 0.8545 | 0.6767 | 0.7413 | -0.0090 | -0.0412 |
| `nfcorpus` | 0.5378 | 0.4892 | 0.2894 | +0.0657 | +0.0629 |

This does not pass the no-collapse gate.

### Lower SAE Weight Signal: `m31_fixed_w0p25`

| Dataset | Recall@100 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: |
| `trec-covid` | 0.1445 | 0.6594 | 0.4822 |
| `msmarco` | 0.7793 | 0.6108 | 0.8614 |
| `dbpedia-entity` | 0.8356 | 0.6475 | 0.7164 |
| `nfcorpus` | 0.5056 | 0.4689 | 0.2714 |

This is the important M39 signal:

- `trec-covid` improves meaningfully, especially MAP.
- Other hard datasets and full15 aggregate regress.
- Therefore M39 is not a model promotion, but it identifies a scale/intent
  interaction: broad queries benefit from different SAE intervention strength.

## Comparison To Prior Best TREC Signals

| Run/source | `trec-covid` NDCG@10 | `trec-covid` MAP@100 | Full15 NDCG@10 | Full15 MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M36 `fixed_w0p5` | 0.6429 | 0.4516 | 0.7731 | 0.7429 |
| M37 transformer `fixed_w0p25` | 0.6620 | 0.4689 | 0.6680 | 0.6275 |
| M38 `fixed_w0p5` | 0.6427 | 0.4540 | 0.7730 | 0.7432 |
| M39 `fixed_w0p25` | 0.6594 | 0.4822 | 0.7347 | 0.7028 |
| M39 `fixed_w0p5` | 0.6332 | 0.4559 | 0.7707 | 0.7435 |

M39 is the best `trec-covid` MAP signal so far, but only under a lower SAE
weight that is too costly for aggregate ranking quality.

## Decision

M39 is closed as:

```text
failed product gate, positive supervision signal
```

Do not promote:

- M39 `fixed_w0p5`;
- M39 `fixed_w0p25` as a global setting;
- another blind synthetic-query volume increase without a calibration plan.

Keep:

- `research_sae_m39_broad_query_generator.py`;
- validation-before-training workflow;
- M39 broad-query artifact as a useful hard-bucket training source;
- evidence that broad queries need different semantic intervention strength.

## Next Direction

The next step should not be another plain generator run. M39 showed that the
new broad-query distribution moves the right hard bucket, but global SAE scale
cannot satisfy all datasets.

The next viable route is:

```text
M39 broad-query source
+ query-type-aware scale / residual calibration
+ collapse-aware selection
  -> recover trec-covid without hurting msmarco/dbpedia/full15
```

This should be treated as a calibration/mixture problem over a better
supervision source, not as a larger-model problem yet.
