# SAE M35 Teacher-Neighborhood Distillation Results Report

Status: closed after full15 training and diversified pseudo-query retry.

## Summary

M35 tested the hypothesis from M34:

```text
high-DF broad query
+ many relevant documents
+ teacher semantic-neighborhood advantage
  -> train text-to-atoms on teacher top-k neighborhoods
```

The hypothesis is still plausible, but the tested implementation failed the
product gate. The failure is not just a loss-weight issue. The artifact builder
created pseudo queries from document titles and first useful sentences, then
questionized them. For `trec-covid`, this overproduced generic COVID
definition-style queries rather than real broad information needs such as
triage, asymptomatic infection, public datasets, transmission, masks, or
schools.

M35 is therefore closed as negative evidence for this specific data-generation
route:

```text
document sentence -> questionized pseudo query -> teacher neighborhood
```

Do not continue by sweeping the same pseudo-query source. The next valid move
is a query-distribution reset: use real train/dev queries and stronger
query-side ranking supervision, or introduce a better query synthesizer with a
separate validation gate before training.

## Implemented Artifacts

New script:

```text
scripts/research_sae_m35_teacher_neighborhood.py
```

The script builds BEIR-style pseudo datasets from already materialized source
artifacts:

- reads `documents.jsonl` and `shared_sae_8192_64/doc_latents.jsonl`;
- extracts title / first-sentence candidate phrases;
- scores candidates by content-token document frequency;
- encodes query text with the canonical Snowflake query prefix;
- encodes SAE query atoms;
- runs the current BM25+SAE teacher to create top-k neighborhood labels;
- writes `documents.jsonl`, `queries.jsonl`, `quality_qrels.json`,
  `qrels.jsonl`, and `shared_sae_8192_64/query_latents.jsonl`;
- marks generated labels as `quality_claim_allowed=false`.

Artifacts:

```text
/Volumes/Betty/Tmp/ii42_sae_m35_teacher_neighborhood/m35-neighborhood-q100
/Volumes/Betty/Tmp/ii42_sae_m35_teacher_neighborhood_v2/m35-neighborhood-diverse-q100
```

Training outputs:

```text
results/sae/m35/teacher-neighborhood-q100-train20k-eval-current
results/sae/m35/teacher-neighborhood-diverse-q100-train20k-eval-current
```

## Artifact Shape

The first full M35 artifact generated five teacher-neighborhood pseudo datasets
with 500 pseudo queries and 25,000 teacher-neighborhood labels:

| Dataset | Docs | Queries | Teacher qrels | Mean content DF |
| --- | ---: | ---: | ---: | ---: |
| `m35-trec-covid-neighborhood` | 17,537 | 100 | 5,000 | 0.3390 |
| `m35-nfcorpus-neighborhood` | 2,063 | 100 | 5,000 | 0.1763 |
| `m35-scifact-neighborhood` | 2,000 | 100 | 5,000 | 0.1551 |
| `m35-msmarco-neighborhood` | 4,102 | 100 | 5,000 | 0.0610 |
| `m35-dbpedia-entity-neighborhood` | 3,357 | 100 | 5,000 | 0.0656 |

The diversified retry added boilerplate stripping, a maximum content-DF filter,
and Jaccard diversity selection. It reduced `trec-covid` mean content DF from
`0.3390` to `0.2913`, but did not fix the training signal.

## Full15 Results

| Run | Best source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidate docs | SAE postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | `m31_fixed_w0p5` | 0.8694 | 0.8780 | 0.7734 | 0.7449 | 2964.8 | 2778.6 |
| M33 hard-family x1 | `m31_fixed_w0p5` | 0.8661 | 0.8749 | 0.7727 | 0.7446 | 2973.2 | 2848.6 |
| M33 hard-family x3 | `m31_fixed_w0p5` | 0.8664 | 0.8777 | 0.7734 | 0.7455 | 2964.9 | 2864.4 |
| M35 q100 | `m31_fixed_w0p5` | 0.8702 | 0.8791 | 0.7744 | 0.7465 | 2965.3 | 2893.1 |
| M35b diverse q100 | `m31_calibrated` | 0.8643 | 0.8860 | 0.7762 | 0.7481 | 2929.6 | 2621.5 |

M35 q100 slightly improves aggregate quality over M32, but the gain is small
and does not address the hard dataset collapse. M35b lowers SAE postings and
improves aggregate MRR/NDCG/MAP, but it worsens the key `trec-covid` failure.
It is not a promotable tradeoff.

## Hard Dataset Collapse

### `trec-covid`

| Run | NDCG@10 | MAP@100 | Recall@100 | NDCG delta vs teacher | MAP delta vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.6226 | 0.4545 | 0.1435 | -0.2124 | -0.3094 |
| M33 hard-family x1 | 0.6391 | 0.4691 | 0.1458 | -0.1960 | -0.2948 |
| M33 hard-family x3 | 0.6208 | 0.4707 | 0.1459 | -0.2143 | -0.2931 |
| M35 q100 | 0.6325 | 0.4558 | 0.1435 | -0.2026 | -0.3081 |
| M35b diverse q100 | 0.6041 | 0.4136 | 0.1341 | -0.2309 | -0.3503 |

M35 does not improve the core blocker. The diversified retry is actively worse
on `trec-covid`.

### `msmarco`

| Run | NDCG@10 | MAP@100 | Recall@100 | NDCG delta vs teacher | MAP delta vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.6320 | 0.8765 | 0.7894 | -0.0929 | -0.0587 |
| M33 hard-family x1 | 0.6336 | 0.8832 | 0.7923 | -0.0912 | -0.0520 |
| M33 hard-family x3 | 0.6394 | 0.8795 | 0.7914 | -0.0854 | -0.0557 |
| M35 q100 | 0.6375 | 0.8877 | 0.7937 | -0.0873 | -0.0475 |
| M35b diverse q100 | 0.6373 | 0.8809 | 0.7884 | -0.0876 | -0.0543 |

M35 q100 gives a small `msmarco` MAP improvement, but not enough to pass the
collapse gate. M35b gives that back.

### `dbpedia-entity`

| Run | NDCG@10 | MAP@100 | Recall@100 | NDCG delta vs teacher | MAP delta vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.6942 | 0.7542 | 0.8523 | 0.0085 | -0.0283 |
| M33 hard-family x1 | 0.6861 | 0.7505 | 0.8511 | 0.0004 | -0.0321 |
| M33 hard-family x3 | 0.6887 | 0.7466 | 0.8473 | 0.0031 | -0.0359 |
| M35 q100 | 0.6918 | 0.7579 | 0.8561 | 0.0062 | -0.0246 |
| M35b diverse q100 | 0.6901 | 0.7587 | 0.8640 | 0.0044 | -0.0239 |

`dbpedia-entity` is not the main M35 failure. M35 is neutral to slightly useful
there, but this does not offset `trec-covid`.

## Interpretation

The evidence rejects a narrow idea:

```text
more broad pseudo queries from document sentences will teach the missing
teacher neighborhood behavior.
```

The evidence does not reject teacher-neighborhood supervision itself. The
problem is that the pseudo-query distribution is wrong:

- extracted sentences are too definition-like and document-specific;
- questionization adds query syntax but not real information-need diversity;
- high-DF selection over-selects generic corpus theme phrases;
- the generated `trec-covid` queries do not resemble the real hard queries
  that require distinguishing semantic slices inside the COVID corpus.

The strongest remaining diagnosis is:

```text
query-side representation and supervision still do not model broad
information needs well enough.
```

This aligns with M29/M30/M31/M32: the model can produce good aggregate quality,
but it collapses on datasets where query intent is broad, graded, and
neighborhood-shaped.

## Decision

M35 is closed as failed gate.

Do not promote:

- M35 q100;
- M35b diverse q100;
- more document-sentence teacher-neighborhood pseudo-query sweeps.

Keep:

- the artifact builder as a research tool;
- the teacher-neighborhood labeling machinery;
- the conclusion that query distribution quality is now the blocker.

## Next Plan: M36 Query-Distribution Reset

M36 should not continue M35-style sentence extraction. It should test three
stricter alternatives:

1. Real-query teacher-neighborhood replay.
   Use only non-test train/dev queries from BEIR-style datasets. Keep fixed
   teacher doc atoms, and train on final BM25+SAE ranking with teacher top-k
   neighborhoods plus qrels. This tests whether the current encoder can learn
   broad-query behavior when the query distribution is real rather than
   generated from documents.

2. Query-shape controlled synthetic data.
   Before training, validate generated queries against real-query statistics:
   length, question-word distribution, content-term DF, entropy, BM25
   concentration, teacher-vs-BM25 gap, and topic diversity. Only if the
   synthetic distribution matches hard real-query buckets should it enter
   training.

3. Stronger query encoder under the same fixed-doc gate.
   If real-query replay still fails, reopen encoder capacity, but only with the
   same ranking-first objective and no-collapse gate. The goal is not a generic
   bigger model; it is better query intent representation for broad semantic
   neighborhoods.

Promotion still requires the M27/M30 gates:

- `trec-covid`, `msmarco`, and `dbpedia-entity` must not collapse;
- aggregate NDCG/MAP cannot hide hard-dataset failure;
- physical cost must stay near the current EATMH profile or form a clear
  quality/cost frontier.
