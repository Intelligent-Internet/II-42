# SAE M34 TREC-COVID Difficulty Analysis Report

Status: diagnostic report after M33 pseudo hard-family training failed.

## Summary

`trec-covid` is hard for the current text-to-atoms student for structural
reasons, not because it needs one-off dataset-specific tuning.

The dataset combines three properties that are rare together in the current
full15 regression surface:

1. Very dense qrels: each query has hundreds of relevant documents.
2. High document-frequency content terms: the query words are broad, repeated
   across much of the corpus, and therefore weak lexical selectors.
3. Teacher advantage comes from ranking a semantic neighborhood, not from
   matching one query to one positive document.

This explains why M33 pseudo self-qrels were weak. They expose the encoder to
biomedical vocabulary, but they do not teach the distributional target:

```text
one broad natural-language query -> many graded semantically relevant documents
```

## Dataset Shape

The current regression artifact uses this `trec-covid` surface:

| Metric | Value |
| --- | ---: |
| Queries | 50 |
| Documents | 17,537 |
| Qrel pairs | 24,673 |
| Relevant docs per query, mean | 493.5 |
| Relevant docs per query, median | 478 |
| Relevant docs per query, max | 1,266 |
| Grade-2 qrels | 14,217 |
| Grade-1 qrels | 10,456 |

This is very different from most full15 datasets. For example, the mean
relevant documents per query are:

| Dataset | Mean qrels/query | Mean max possible Recall@100 |
| --- | ---: | ---: |
| `trec-covid` | 493.5 | 0.2674 |
| `msmarco` | 95.4 | 0.8726 |
| `nfcorpus` | 38.2 | 0.9742 |
| `dbpedia-entity` | 34.6 | 0.9774 |
| most other datasets | 1-19 | about 1.0000 |

For `trec-covid`, Recall@100 has a low ceiling because there are far more than
100 relevant documents per query. The teacher's Recall@100 `0.2081` is already
close to the median top-100 capacity implied by qrel density. The student
around `0.145` is not just missing a few exact positives; it is failing to
cover the broad relevant neighborhood.

## Query Term Selectivity

After removing common stopwords and numeric tokens, `trec-covid` has the
highest query-term document frequency in the current full15 surface:

| Dataset | Content query mean DF fraction | Content terms with DF >= 10% |
| --- | ---: | ---: |
| `trec-covid` | 0.1909 | 0.42 |
| `cqadupstack` | 0.1066 | 0.25 |
| `fiqa` | 0.0738 | 0.21 |
| `webis-touche2020` | 0.0615 | 0.19 |
| `scifact` | 0.0571 | 0.18 |
| `msmarco` | 0.0239 | 0.01 |
| `dbpedia-entity` | 0.0202 | 0.03 |

Representative high-frequency `trec-covid` query terms:

| Term | Query frequency | Document frequency fraction |
| --- | ---: | ---: |
| `covid` | 31 | 0.7452 |
| `coronavirus` | 8 | 0.5134 |
| `sars` | 8 | 0.4487 |
| `cov` | 7 | 0.4225 |
| `patients` | 3 | 0.3744 |
| `infected` | 3 | 0.1455 |
| `complications` | 4 | 0.0395 |

This means lexical evidence is noisy: many query terms are true topic
indicators, but they appear in a large portion of the corpus. BM25 can rank
some obvious documents, but the hard part is choosing the right semantic slice
inside a broad biomedical topic.

## Quality Gap

Aggregate `trec-covid` results:

| Source | Recall@100 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: |
| BM25 | 0.1384 | 0.6645 | 0.4693 |
| Teacher fixed-doc | 0.2081 | 0.8351 | 0.7639 |
| M32 teacher-anchor best fixed | 0.1435 | 0.6226 | 0.4545 |
| M33 pseudo x1 best fixed | 0.1458 | 0.6391 | 0.4691 |
| M33 pseudo x3 best fixed | 0.1459 | 0.6208 | 0.4707 |

M33 did not solve the problem. The x1 pseudo run recovers MAP to roughly BM25,
but remains far below teacher. The x3 pseudo run barely moves MAP and worsens
NDCG. That is evidence against the hypothesis that simple biomedical
vocabulary exposure is enough.

## Query-Level Failure Pattern

The largest teacher-vs-student AP gaps are broad, natural-language biomedical
or public-health queries:

| Query | Qrels | BM25 AP | Teacher AP | Student AP |
| --- | ---: | ---: | ---: | ---: |
| `what is known about those infected with Covid-19 but are asymptomatic?` | 849 | 0.3247 | 0.9659 | 0.2812 |
| `what are the guidelines for triaging patients infected with coronavirus?` | 415 | 0.1200 | 0.7883 | 0.1049 |
| `What new public datasets are available related to COVID-19?` | 218 | 0.1128 | 0.7676 | 0.1442 |
| `what is the origin of COVID-19` | 637 | 0.2958 | 0.7916 | 0.1808 |
| `what kinds of complications related to COVID-19 are associated with hypertension?` | 371 | 0.1984 | 0.9788 | 0.4079 |

Across the 50 queries, the average teacher-student AP gap is `0.3094`; the
median gap is `0.2863`. The student beats teacher on only one query for AP.

The important part is that the teacher is not just adding a small semantic
rerank. It is finding a much better relevant document neighborhood under a
fixed top-100 budget.

## Why Pseudo Self-Qrels Failed

M33 pseudo queries used title / first-sentence text and one self-document
positive. That teaches:

```text
query-like text -> same document
```

But `trec-covid` needs:

```text
broad question -> many relevant documents with graded relevance
```

Those are different training problems. Weighting the pseudo self-qrels 3x did
not help, which reinforces the interpretation that the supervision target is
wrong, not merely underweighted.

## Generalized Next Direction

Do not specialize to `trec-covid`. Use it as a diagnostic for a general class:

```text
high-DF broad natural-language query
+ many relevant documents
+ teacher has large semantic-neighborhood advantage over BM25
```

The next training direction should be dataset-agnostic teacher-neighborhood
distillation:

1. Select or synthesize broad queries across multiple corpora, not just
   `trec-covid`.
2. Use the teacher top-k distribution as the target, not self-document pseudo
   qrels.
3. Include BM25 hard negatives and teacher near-misses so the model learns the
   boundary of the semantic neighborhood.
4. Keep fixed teacher doc atoms and the final `BM25 + SAE` ranking objective.
5. Evaluate the same no-collapse gate on `trec-covid`, `msmarco`, and
   `dbpedia-entity`; do not add dataset IDs or runtime dataset-specific logic.

This keeps the product goal unchanged:

```text
text -> atoms -> unified sparse evidence engine
```

but shifts the blocker from "more biomedical text" to "learn broad-query
teacher neighborhoods."
