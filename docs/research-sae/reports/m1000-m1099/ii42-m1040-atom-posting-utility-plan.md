# M1040 Atom Posting Utility Plan

Date: 2026-06-16

## Goal

M1030 proved that direct retrieval supervision can move token-hidden SAE atoms,
but the learned atom vocabulary still behaved badly as an inverted-index term
space: heldout quality was unstable and high-df atoms touched most of the
corpus.

M1040 changes the objective from local pairwise dense-style matching to
posting-term utility:

```text
query atom postings should hit qrel positives,
avoid BM25 hard negatives,
and keep corpus/batch df low enough for IDF to matter.
```

This is still a canary. The first target is official `nfcorpus` with a
deterministic train/heldout query split.

## Difference From M1030

M1030 optimized normalized pooled atom dot products:

```text
score(q, d) = cosine(pool_atoms(q), pool_atoms(d))
```

That target can overfit query-document pairs without producing useful posting
terms.

M1040 instead scores with a differentiable BM25-like posting surface:

```text
score(q, d) =
    sum_atom normalized_q_tf(atom) * normalized_doc_tf(atom) * batch_idf(atom)
```

The training batch contains positives and BM25 hard negatives. Atom IDF is
estimated from batch document activation, detached from gradients, then used in
the score. The loss adds explicit fanout penalties for atoms that appear in too
many documents or are selected by too many queries.

## Objective

For each training batch:

1. encode queries, qrel positives, and lexical BM25 hard negatives from
   transformer token hidden states;
2. pool token-level top-k atom activations into query/document atom vectors;
3. compute batch df/idf from positive and negative document atom activations;
4. optimize positive score above negative score;
5. penalize high-df atom usage and broad query-document overlap.

The target is not a learned scorer. The output must still work as real
atom-BM25 postings.

## Evaluation

Rows:

- lexical BM25;
- M1040 atom-BM25;
- unified lexical + M1040 atom-BM25 at fixed SAE scales.

Metrics:

- Recall@100;
- MRR@20;
- NDCG@10;
- MAP@100;
- train/heldout split metrics;
- atom df diagnostics;
- average touched docs/postings.

## M1040.0 Pass/Fail

Pass signal:

- heldout atom-BM25 beats M1030.0 heldout atom-BM25;
- heldout unified row improves without relying only on lexical BM25;
- atom df head ratio decreases materially from M1030;
- touched docs/postings move down or quality gain clearly offsets cost.

Fail signal:

- train improves while heldout remains weak;
- head atoms still cover nearly all documents;
- unified quality comes only from lexical BM25 fallback.

If M1040.0 fails, the next route should not be another scalar fusion tweak.
It should move to a larger cross-query/multi-dataset posting utility surface or
an architecture change that produces healthier atom df before ranking.

## Current Status

Implementation:

- `scripts/research_sae_m1040_atom_posting_utility.py`
- `scripts/run_m1040_atom_posting_utility_spark.sh`

Execution target:

- `spark-1`
- remote output root:
  `/home/huoju/leask/runs/ii42-m1040-atom-posting-utility-v1`

## M1040.0 Result

Status: completed for official `nfcorpus`.

Artifacts:

- Remote:
  `/home/huoju/leask/runs/ii42-m1040-atom-posting-utility-v1/nfcorpus_m1040_atom_posting_utility.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/nfcorpus_m1040_atom_posting_utility.json`

Configuration:

- 70/30 deterministic query split;
- `226` train queries and `97` heldout queries;
- `1740` pairwise train triples;
- qrel positives vs lexical BM25 hard negatives;
- pretrain epochs `2`;
- posting utility epochs `3`;
- learning rate `0.0005`;
- fanout penalty `0.04`;
- no background corpus negatives.

Result:

| Row | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| lexical BM25 | all | 0.2179 | 0.4870 | 0.2814 | 0.1212 |
| M1040 atom-BM25 | all | 0.2820 | 0.2781 | 0.1675 | 0.0932 |
| unified scale 1 | all | 0.3304 | 0.5480 | 0.3249 | 0.1545 |
| M1040 atom-BM25 | train | 0.3160 | 0.3106 | 0.1909 | 0.1113 |
| M1040 atom-BM25 | heldout | 0.2030 | 0.2024 | 0.1129 | 0.0510 |
| unified scale 1 | heldout | 0.2870 | 0.4997 | 0.2792 | 0.1313 |

Diagnostics:

- active atoms: `770`;
- max df ratio: `1.0`;
- head 1% df ratio: `0.9994`;
- average SAE postings touched: about `49,283`;
- average touched docs: full corpus.

Interpretation:

- The objective improved all-query unified recall, but it did not create a
  healthy posting vocabulary.
- Heldout atom-BM25 is weaker than train and weaker than the useful M1030
  train-side signal.
- Batch pos/neg IDF is insufficient: it never exposes atoms to a realistic
  background-corpus fanout penalty.

Decision:

- Do not promote M1040.0.
- Run M1040.1 with random background corpus documents inside each training
  batch. The point is to penalize query-background overlap and estimate IDF
  against non-candidate documents, not only qrel positives and BM25 hard
  negatives.

## M1040.1 Background-Corpus Posting Utility

Change:

- add `background_docs`;
- add `background_per_batch`;
- add `background_overlap_weight`;
- compute batch IDF from positives, hard negatives, and background documents;
- penalize query overlap with background documents.

Expected improvement:

- lower head atom df;
- lower touched docs/postings;
- heldout atom-BM25 should not collapse relative to M1040.0.

Stop rule:

- If head df remains near `1.0` and heldout atom-BM25 does not improve, stop
  this loss family and move to a representation/pooling architecture change.

## M1040.1 Result

Status: completed for official `nfcorpus`.

Artifacts:

- Remote:
  `/home/huoju/leask/runs/ii42-m1040-atom-posting-utility-v1/nfcorpus_m1040_atom_posting_utility_bg_v1.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/nfcorpus_m1040_atom_posting_utility_bg_v1.json`

Configuration changes from M1040.0:

- `background_docs=2048`;
- `background_per_batch=32`;
- `background_overlap_weight=0.05`.

Result:

| Row | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| M1040.1 atom-BM25 | all | 0.2926 | 0.3202 | 0.1997 | 0.1081 |
| M1040.1 atom-BM25 | heldout | 0.2125 | 0.1977 | 0.1076 | 0.0475 |
| unified scale 1 | all | 0.3222 | 0.5486 | 0.3276 | 0.1565 |
| unified scale 1 | heldout | 0.2752 | 0.5161 | 0.2845 | 0.1351 |

Diagnostics:

- active atoms: `1117`;
- max df ratio: `1.0`;
- head 1% df ratio: `0.9843`;
- average SAE postings touched: about `48,416`;
- average touched docs: full corpus.

Interpretation:

- Background documents slightly improved df head ratio, but the vocabulary is
  still not a useful inverted-index term space.
- The rank loss stayed nearly flat, which means the reconstruction-pretrained
  atom geometry resisted the posting utility objective.
- Unified heldout is still worse than M1030.0 unified heldout.

Decision:

- Do not promote M1040.1.
- Test whether reconstruction pretraining itself is the source of high-df
  head atoms by running a direct-from-scratch canary.

## M1040.2 Direct-From-Scratch Posting Utility

Status: completed for official `nfcorpus`.

Artifacts:

- Remote:
  `/home/huoju/leask/runs/ii42-m1040-atom-posting-utility-v1/nfcorpus_m1040_atom_posting_utility_scratch_v1.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/nfcorpus_m1040_atom_posting_utility_scratch_v1.json`

Configuration changes from M1040.1:

- `pretrain_epochs=0`;
- `rank_epochs=8`;
- `learning_rate=0.001`.

Training signal:

- rank loss dropped from about `0.731` to `0.242`;
- fanout loss dropped from about `30-40` to `0.329`.

Result:

| Row | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| M1040.2 atom-BM25 | all | 0.3158 | 0.5001 | 0.3452 | 0.2089 |
| M1040.2 atom-BM25 | train | 0.4293 | 0.6943 | 0.4804 | 0.2908 |
| M1040.2 atom-BM25 | heldout | 0.0512 | 0.0476 | 0.0300 | 0.0182 |
| unified scale 1 | all | 0.3968 | 0.6606 | 0.4402 | 0.2405 |
| unified scale 1 | train | 0.4763 | 0.7484 | 0.5235 | 0.2967 |
| unified scale 1 | heldout | 0.2117 | 0.4560 | 0.2461 | 0.1096 |

Diagnostics:

- active atoms: `3963`;
- max df ratio: `0.1704`;
- head 1% df ratio: `0.0888`;
- average SAE postings touched: about `224`;
- average touched docs: about `209`.

Interpretation:

- This is the first M1000-series token route that creates a healthy low-fanout
  atom posting space.
- It does not generalize: heldout atom-BM25 collapses, so the low-df postings
  are query-specific rather than reusable semantic terms.
- The result separates two facts that were previously entangled:
  reconstruction pretraining causes high-df atom fanout, while direct posting
  training can control fanout but overfits when supervision is too narrow.

Decision:

- M1040.2 is not a candidate model.
- It is strong evidence that the next useful route must train atom posting
  terms on a much broader cross-query/cross-dataset surface, or introduce
  a query/document asymmetric vocabulary that prevents memorized query-specific
  atoms.
- Do not spend more time tuning `nfcorpus`-only penalties.
