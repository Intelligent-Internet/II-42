# M1030 Direct Posting Objective Plan

Date: 2026-06-16

## Goal

M1020 showed that transformer token hidden states alone do not automatically
produce a healthy BM25-compatible SAE atom vocabulary. The atom df distribution
was too head-heavy, and simple clipping or batch-level usage regularization did
not fix it.

M1030 therefore changes the training target. Instead of:

```text
hidden-state reconstruction -> post-hoc atom BM25
```

we test:

```text
qrel positives + BM25 hard negatives
    -> train pooled token atoms for admission/ranking
    -> evaluate the learned atoms as BM25 posting terms
```

This is still a smoke, not a production route. The purpose is to check whether
direct retrieval supervision can create a more useful token atom space than the
M1020 reconstruction-first route.

## Direct Objective

For each training query:

1. choose qrel-positive documents;
2. choose BM25 hard negatives from lexical top-ranked documents that are not
   relevant;
3. encode query and documents from transformer token hidden states;
4. pool token atoms into sparse vectors;
5. optimize a pairwise margin loss:

```text
score(query, positive) > score(query, hard negative)
```

Regularization remains posting-aware:

- atom activation L1;
- batch head-usage penalty;
- active top-k bottleneck;
- post-pool clipping at evaluation.

The first smoke deliberately avoids a learned fusion head. If direct training
cannot improve the atom posting surface by itself, adding a fusion head would
only hide the representation failure.

## M1030.0 Canary

Dataset:

- official `nfcorpus`

Split:

- deterministic train/heldout query split;
- default train fraction: `0.7`;
- report metrics on all queries, train queries, and heldout queries.

Rows:

- lexical BM25;
- direct-trained token SAE-BM25;
- unified lexical + direct-trained token SAE-BM25.

Pass signal:

- heldout unified row approaches or exceeds M1020.0 unified;
- SAE-only improves over M1020.0 SAE-only without expanding fanout;
- atom df/head ratio improves materially.

Fail signal:

- train rows improve but heldout collapses;
- SAE-only stays weak and unified gains come only from lexical BM25;
- df/head ratio remains near whole-corpus coverage.

## Current Status

Implementation added in:

- `scripts/research_sae_m1030_direct_posting_objective.py`

Execution target:

- `spark-1`
- output root:
  `/home/huoju/leask/runs/ii42-m1030-direct-posting-objective-v1`

M1030 should not run BEIR15 until the `nfcorpus` heldout signal is positive.

## M1030.0 Result

Status: completed for official `nfcorpus`.

Artifacts:

- Remote:
  `/home/huoju/leask/runs/ii42-m1030-direct-posting-objective-v1/nfcorpus_m1030_direct_posting_objective.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/nfcorpus_m1030_direct_posting_objective.json`

Configuration:

- 70/30 deterministic query split;
- `226` train queries and `97` heldout queries;
- `1740` pairwise train triples;
- qrel positives vs lexical BM25 hard negatives;
- pretrain epochs `2`;
- direct ranking epochs `3`;
- learning rate `0.0007`;
- margin `0.1`.

Result:

| Row | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| lexical BM25 | all | 0.2179 | 0.4870 | 0.2814 | 0.1212 |
| direct token SAE-BM25 | all | 0.4006 | 0.6207 | 0.3947 | 0.2276 |
| unified scale 1 | all | 0.4294 | 0.6564 | 0.4141 | 0.2190 |
| unified scale 2 | all | 0.4356 | 0.6745 | 0.4340 | 0.2361 |
| direct token SAE-BM25 | train | 0.4780 | 0.7505 | 0.4797 | 0.2889 |
| unified scale 2 | train | 0.5035 | 0.7493 | 0.4909 | 0.2823 |
| direct token SAE-BM25 | heldout | 0.2202 | 0.3184 | 0.1964 | 0.0848 |
| unified scale 1 | heldout | 0.2920 | 0.5413 | 0.3372 | 0.1473 |
| unified scale 2 | heldout | 0.2773 | 0.5003 | 0.3014 | 0.1284 |

Diagnostics:

- active atoms: `2259`;
- max df ratio: `0.9970`;
- head 1% df ratio: `0.8560`;
- average SAE postings touched: about `34,479`;
- average touched docs: about `3630`.

Interpretation:

- Direct qrel/BM25 hard-negative supervision is powerful enough to move the
  atom space.
- The apparent all-query win is mostly train-query overfit.
- Heldout unified scale 1 is only competitive with M1020.0; SAE-only heldout is
  weak.
- Fanout is still too high: nearly the whole `nfcorpus` corpus is touched per
  query.

Decision:

- Do not promote M1030.0.
- Run one conservative update canary to check whether the direct objective can
  generalize with lower update strength.

## M1030.1 Conservative Update Result

Status: completed for official `nfcorpus`.

Artifacts:

- Remote:
  `/home/huoju/leask/runs/ii42-m1030-direct-posting-objective-v1/nfcorpus_m1030_direct_posting_objective_cons_v1.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/nfcorpus_m1030_direct_posting_objective_cons_v1.json`

Configuration changes:

- direct ranking epochs `1`;
- learning rate `0.0002`;
- margin `0.05`;
- head usage penalty `0.005`.

Result:

| Row | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| direct token SAE-BM25 | all | 0.1597 | 0.1882 | 0.1108 | 0.0530 |
| direct token SAE-BM25 | heldout | 0.1569 | 0.2319 | 0.1393 | 0.0560 |
| unified scale 0.5 | all | 0.2542 | 0.5192 | 0.3013 | 0.1344 |
| unified scale 0.5 | heldout | 0.2363 | 0.4785 | 0.3024 | 0.1240 |
| unified scale 1 | heldout | 0.2273 | 0.4763 | 0.3053 | 0.1247 |

Diagnostics:

- active atoms: `917`;
- max df ratio: `1.0`;
- head 1% df ratio: `0.99997`;
- average SAE postings touched: about `66,621`;
- average touched docs: full corpus.

Interpretation:

- Lower update strength does not solve generalization.
- It also worsens atom df/fanout.
- Simple pairwise qrel-vs-BM25-negative training is not enough as the final
  direct target.

## M1030 Decision

M1030 confirms that direct target training matters, but the current target is
still incomplete.

What worked:

- Direct retrieval supervision can strongly change the atom space.
- Train-query performance can become much stronger than both M1020 and lexical
  BM25.

What failed:

- Heldout generalization is not reliable.
- The atom vocabulary is still high-fanout.
- Pairwise hard-negative loss optimizes local query-document preferences but
  does not create a globally useful posting vocabulary.

Next route if we continue:

1. move from pairwise query-doc scoring to **posting-level utility training**;
2. make atom df/fanout part of the loss after document pooling, not only a
   batch-level proxy;
3. use multiple query families or cross-fold training surfaces so the atom
   vocabulary cannot specialize to one train split;
4. train lexical and SAE evidence jointly as a unified sparse objective, rather
   than training SAE atoms alone and adding lexical BM25 afterwards.

This should be treated as a new M1040/M1100 route, not another M1030 scalar
sweep.
