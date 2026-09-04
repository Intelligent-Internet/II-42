# SAE M31 Joint Final-Ranking Results Report

Status: first M31 training pass completed; final-ranking supervision has a
signal, but the query-side robustness gate still fails.

M31 starts from the M29/M30 conclusion that scalar calibration is insufficient.
The new training objective treats the final `BM25 + SAE` score as the training
surface instead of assuming one fixed SAE weight. The query text encoder
produces query SAE atoms, while a small runtime-safe head predicts per-query
`bm25_scale`, `sae_scale`, and residual strength. The loss adds an explicit
BM25-preservation term to address the M30 collapse taxonomy, where the dominant
failure was BM25-supported positives being suppressed.

## Data Scope

Current training still uses the prepared full15 shared artifact, not full BEIR
corpora:

| Scope | Documents | Queries | Qrel pairs |
| --- | ---: | ---: | ---: |
| Prepared full15 artifact | 49,059 | 1,342 | 39,742 |
| Local official BEIR15 files | 33,860,495 | 778,054 | not counted here |
| Prepared share | 0.14% docs | 0.17% queries | sampled/evaluation subset |

The current artifact is useful for fast apples-to-apples model iteration, but
it is not enough to claim full BEIR generalization. Most datasets use at most
100 test queries and about 2k sampled docs plus qrel-positive inclusions. The
large exceptions are `trec-covid` and `webis-touche2020`, where the prepared
query sets cover the official query count, and `trec-covid` pulls in many qrel
positive documents.

## Training Strategy Tested

M31 tested two variants:

| Variant | Encoder | Trainable parts | Purpose |
| --- | --- | --- | --- |
| Frozen encoder | M29 Arm A fixed | joint BM25/SAE scale head | isolate whether per-query weighting fixes collapse |
| Low-LR unfrozen encoder | M29 Arm A initialized | query encoder + joint scale head | test whether final-ranking supervision can move atoms usefully |

Both variants keep document atoms fixed to teacher doc atoms. This continues to
isolate the query-side blocker before any doc-side training.

## Full15 Quality

| Run | Best selected source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| M29 strong qrel baseline | `w0p75` | 0.8673 | 0.9080 | 0.7872 | 0.7536 | failed robustness |
| M30 frozen calibration | `w0p5` | 0.8649 | 0.8847 | 0.7737 | 0.7415 | failed robustness |
| M31 frozen final-rank | `w0p5` | 0.8649 | 0.8847 | 0.7737 | 0.7415 | failed robustness |
| M31 unfrozen final-rank | `w0p5` | 0.8648 | 0.8721 | 0.7653 | 0.7359 | failed robustness |
| Teacher fixed-doc reference | `teacher_fixed_doc` | 0.8447 | 0.8399 | 0.7490 | 0.7247 | reference |

The learned calibrated source in M31 frozen nearly reproduces the M29 strong
aggregate, but collapse-aware selection still prefers fixed `w0p5`. That means
the runtime-safe scale head does not yet learn a reliable collapse-avoidance
policy.

## Collapse Comparison

| Dataset | Metric | M29 strong | M30 frozen | M31 unfrozen |
| --- | --- | ---: | ---: | ---: |
| `dbpedia-entity` | `map@100` | -0.0869 | -0.0579 | -0.0310 |
| `msmarco` | `ndcg@10` | -0.1569 | -0.1119 | -0.0928 |
| `msmarco` | `map@100` | -0.1512 | -0.0888 | -0.0432 |
| `trec-covid` | `ndcg@10` | -0.3134 | -0.1889 | -0.1812 |
| `trec-covid` | `map@100` | -0.4350 | -0.3177 | -0.2991 |

The low-LR unfrozen run is informative: final-ranking supervision plus
BM25-preservation reduces `dbpedia-entity` and `msmarco` collapse, but does not
solve `trec-covid`. It also increases physical cost.

## Physical Cost

| Run | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| M29 strong qrel baseline | 2860.3 | 10185.0 | 1738.0 |
| M31 frozen final-rank | 2860.3 | 10185.0 | 1738.0 |
| M31 unfrozen final-rank | 2971.3 | 10185.0 | 2851.2 |
| Teacher fixed-doc reference | 2986.2 | 10185.0 | 4522.8 |

The unfrozen encoder moves toward teacher-like coverage, which explains both
the improved `msmarco/dbpedia` collapse and the higher SAE postings. This is a
real tradeoff, not a free win.

## Interpretation

M31 supports the user's second hypothesis:

```text
The model should learn query atoms and BM25/SAE contribution through the final
ranking objective, instead of assuming a global SAE weight.
```

But the first hypothesis remains necessary:

```text
The encoder still needs teacher anchoring. Without it, qrels can create strong
aggregate ranking while breaking dataset robustness.
```

The right next objective is therefore not "imitate teacher" or "optimize qrels"
alone. It is a three-way objective:

1. Teacher anchors semantic distribution.
2. Qrels optimize final ranking only where they do not contradict robust
   teacher/BM25 evidence.
3. BM25-preservation prevents lexical positives from being suppressed.

## Data Strategy Implication

The current 1,342-query artifact is too small to train a robust policy for
query families. The next phase should use more of the official BEIR data, but
must not contaminate evaluation.

Recommended M32 data split:

- Keep the existing prepared 15-dataset test queries as the fixed regression
  benchmark.
- Build a training artifact from official BEIR train/dev queries where
  available.
- Exclude all current test query IDs from training.
- Include much more corpus context than 2k sampled docs per dataset; use at
  least a 20k/q300-style artifact for hard negatives and BM25/SAE fanout
  behavior.
- For datasets without train/dev queries, keep them as holdout-only unless a
  defensible pseudo-query/proxy-qrel protocol is defined.

## Exit Decision

M31 does not pass the product gate, but it changes the next direction:

- scalar calibration alone is not enough;
- final-ranking training has a useful signal;
- unfreezing the query encoder under BM25-preservation improves some collapses;
- `trec-covid` remains the hard blocker;
- next work should focus on larger clean train/eval splits and query-family
  robustness, not another small fixed-query sweep.

Artifacts:

```text
scripts/research_sae_m31_joint_final_ranking_train.py
results/sae/m31/joint-final-ranking-smoke/m31_joint_final_ranking_train.json
results/sae/m31/joint-final-ranking-frozen/m31_joint_final_ranking_train.json
results/sae/m31/joint-final-ranking-unfrozen-lr1e4/m31_joint_final_ranking_train.json
```
