# SAE Retrieval-Aware Training Report

Date: 2026-05-11

## Objective

This phase tests whether retrieval-aware training can improve the SAE path
beyond pure dense-embedding reconstruction.

The target is not only quality. The real target is:

```text
dense-level semantic recall with sparse-impact selectivity
```

If retrieval-aware training improves recall but makes almost every document a
candidate, it is useful evidence but not yet a database-index breakthrough.

## Prototype

New script:

```text
scripts/research_sae_retrieval_train.py
```

The script trains a top-k SAE with:

- query-positive contrastive loss;
- optional dense-teacher document-neighbor pairs;
- reconstruction loss over documents and train queries;
- activation L1;
- soft activation-mass load balancing;
- train/test query split.

The split is important. The first all-query result reached perfect recall, but
that was memorization. All meaningful numbers below use a `70/30` query split.

Dataset:

```text
dataset: sampled BEIR SciDocs
documents: 2000
queries: 100
positive qrel pairs: 492
embedding dims: 768
base model: Snowflake/snowflake-arctic-embed-m-v2.0
```

## Baseline

From the existing quality matrix:

| Source | Recall@20 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: |
| dense | `0.5280` | `0.6920` | `0.7120` |
| SAE autoencoder | `0.5135` | `0.7025` | `0.5973` |
| BM25+SAE autoencoder | `0.5175` | `0.7040` | `0.6624` |

The existing autoencoder SAE already has good Recall@100, but weaker MRR.

## Qrels-Only Retrieval Training

Command shape:

```bash
python3 scripts/research_sae_retrieval_train.py \
  --documents /tmp/ii42_sae_quality_matrix/scidocs/documents.jsonl \
  --queries /tmp/ii42_sae_quality_matrix/scidocs/queries.jsonl \
  --output-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_split \
  --latent-dims 8192 \
  --active-dims 64 \
  --epochs 20 \
  --batch-size 64 \
  --train-fraction 0.7 \
  --device mps
```

Result:

| Split | Source | Recall@20 | Recall@100 | MRR@20 |
| --- | --- | ---: | ---: | ---: |
| test | dense | `0.5300` | `0.6933` | `0.7014` |
| test | retrieval SAE | `0.3817` | `0.5850` | `0.3998` |
| train | retrieval SAE | `1.0000` | `1.0000` | `0.9714` |

Conclusion:

Qrels-only contrastive training overfits. With only `70` train queries and
`346` train qrel pairs, the model learns the training queries but generalizes
poorly.

This route is not sufficient.

## Dense-Teacher Training

To avoid only memorizing qrels, I added dense-teacher document-neighbor pairs:

```text
teacher-doc-neighbors: 5
qrel pairs: 346
teacher pairs: 10000
```

Command shape:

```bash
python3 scripts/research_sae_retrieval_train.py \
  --documents /tmp/ii42_sae_quality_matrix/scidocs/documents.jsonl \
  --queries /tmp/ii42_sae_quality_matrix/scidocs/queries.jsonl \
  --output-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_teacher5 \
  --latent-dims 8192 \
  --active-dims 64 \
  --teacher-doc-neighbors 5 \
  --epochs 20 \
  --batch-size 128 \
  --train-fraction 0.7 \
  --device mps
```

Result:

| Split | Source | Recall@20 | Recall@100 | MRR@20 |
| --- | --- | ---: | ---: | ---: |
| test | dense | `0.5300` | `0.6933` | `0.7014` |
| test | retrieval SAE | `0.5183` | `0.7400` | `0.4315` |
| train | retrieval SAE | `0.9829` | `1.0000` | `0.9571` |
| all | retrieval SAE | `0.8435` | `0.9220` | `0.7994` |

This is the first useful positive signal:

```text
held-out Recall@100 improves over dense: 0.7400 vs 0.6933
```

But it also exposes the main systems problem:

```text
unique latent dims: 664
max_df_ratio: 1.0
mean_df_ratio: 0.0937
```

The model collapses into broad high-DF semantic dimensions. That is good for
recall, but bad for an impact-ordered sparse index.

## Soft Load-Balance Attempt

The first load-balance implementation used a hard `(sparse > 0)` mask, which
does not provide useful gradient through top-k selection. I replaced it with a
soft activation-mass load penalty.

Command shape:

```bash
python3 scripts/research_sae_retrieval_train.py \
  --documents /tmp/ii42_sae_quality_matrix/scidocs/documents.jsonl \
  --queries /tmp/ii42_sae_quality_matrix/scidocs/queries.jsonl \
  --output-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_teacher5_softlb20 \
  --latent-dims 8192 \
  --active-dims 64 \
  --teacher-doc-neighbors 5 \
  --load-balance-weight 0.2 \
  --epochs 20 \
  --batch-size 128 \
  --train-fraction 0.7 \
  --device mps
```

Result:

| Split | Source | Recall@20 | Recall@100 | MRR@20 |
| --- | --- | ---: | ---: | ---: |
| test | dense | `0.5300` | `0.6933` | `0.7014` |
| test | retrieval SAE | `0.5300` | `0.7267` | `0.4822` |
| train | retrieval SAE | `0.9829` | `1.0000` | `0.9690` |
| all | retrieval SAE | `0.8470` | `0.9180` | `0.8230` |

This improves MRR over the first dense-teacher version, but still has broad
latent usage:

```text
unique latent dims: 714
max_df_ratio: 1.0
mean_df_ratio: 0.0875
```

Soft activation-mass balance is not strong enough to make the selected top-k
dimensions selective.

## BM25+SAE and Bound Simulation

Using the soft load-balanced dense-teacher latents:

```bash
python3 scripts/research_sae_dense_replacement_eval.py \
  --work-dir /tmp/ii42_sae_retrieval_matrix_scidocs_a64_softlb20 \
  --output-dir /tmp/ii42_sae_dense_replacement_scidocs_retrieval_softlb20 \
  --datasets scidocs \
  --latent-dims 8192 \
  --active-dims 64 \
  --score-mode normalized_idf_dot \
  --bm25-mode plain \
  --top-k 100
```

All-query result, which includes train queries and should be treated as an
upper-bound quality check:

| Source | Recall@100 | MRR@20 | Mean candidates | Mean postings touched |
| --- | ---: | ---: | ---: | ---: |
| dense | `0.6920` | baseline | n/a | n/a |
| best fixed BM25+SAE | `0.9140` | `0.7881` | `2000.0` | `36821.6` |

The quality is high, but the fanout is maximal.

Block-bound simulation at top-20:

```text
best layout: sae_pair
block size: 16
exact matches: 100/100
opened docs: 0.864
candidate docs: 1.000
Recall@20: 0.6505
MRR@20: 0.7853
```

So retrieval-aware training improves quality, but it still does not create a
fast index. The physical scan still opens most of the corpus.

## DF-Aware Selection Training

The next phase adds a selection-time document-frequency control to the same
top-k SAE structure.

The important distinction is that the DF cost changes which latents enter the
top-k set, but it does not rewrite the selected activation weights. This keeps
retrieval scores comparable while making high-fanout dimensions less likely to
occupy sparse slots.

Two controls were added:

- `df_selection_weight`: subtracts an EMA document-frequency cost before top-k
  selection.
- `df_selection_mask_ratio`: optionally hard-masks near-global dimensions after
  warmup, forcing the encoder to fill the top-k slots with alternate latents.

Command shape:

```bash
python3 scripts/research_sae_retrieval_train.py \
  --documents /tmp/ii42_sae_quality_matrix/scidocs/documents.jsonl \
  --queries /tmp/ii42_sae_quality_matrix/scidocs/queries.jsonl \
  --output-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_teacher5_dfcost10_mask999 \
  --latent-dims 8192 \
  --active-dims 64 \
  --teacher-doc-neighbors 5 \
  --load-balance-weight 0.2 \
  --df-selection-weight 0.10 \
  --df-selection-mask-ratio 0.999 \
  --epochs 20 \
  --batch-size 128 \
  --train-fraction 0.7 \
  --device mps
```

Result summary:

| Variant | Test Recall@100 | Test MRR@20 | All Recall@100 | All MRR@20 | Mean active dims | Max DF ratio | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dfcost05` | `0.7333` | `0.4725` | `0.9200` | `0.8251` | `26.03` | `1.0000` | `52064` |
| `dfcost10` | `0.7267` | `0.4909` | `0.9140` | `0.8089` | `15.05` | `1.0000` | `30090` |
| `dfcost05_mask999` | `0.7267` | `0.4811` | `0.9180` | `0.8193` | `28.82` | `0.5435` | `57643` |
| `dfcost10_mask999` | `0.7133` | `0.4553` | `0.9120` | `0.8053` | `16.20` | `0.1785` | `32408` |

Dense-replacement and bound simulation:

| Variant | Best fixed BM25+SAE Recall@100 | Best fixed MRR@20 | Mean SAE candidates | Mean SAE postings touched | Best bound opened docs | Bound Recall@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dfcost05` | `0.9200` | `0.7967` | `2000.0` | `7709.7` | `0.760` | `0.6520` |
| `dfcost10` | `0.9240` | `0.7609` | `2000.0` | `3514.2` | `0.575` | `0.6705` |
| `dfcost05_mask999` | `0.9200` | `0.7852` | `1952.9` | `9148.7` | `0.784` | `0.6580` |
| `dfcost10_mask999` | `0.9180` | `0.7729` | `1023.1` | `1958.6` | `0.561` | `0.7240` |

Interpretation:

```text
dfcost05 is the quality-favoring point.
dfcost10 is the best postings-reduction point without hard masking.
dfcost10_mask999 is the first strong index-selectivity signal.
```

`dfcost10_mask999` is especially important because it cuts mean SAE candidates
from the full corpus to about half of the corpus while preserving better
all-query Recall@100 than dense. It does pay a held-out quality cost on this
small split, so it should be treated as an efficiency-first research branch,
not the final model.

## Expected-Postings Fanout Loss

The next training step adds a differentiable fanout regularizer:

```text
mean(selected activation mass * normalized document-frequency cost)
```

This is different from selection cost. Selection cost changes top-k membership
directly. Fanout loss keeps the selected top-k path, but pushes high-DF
selected activations down through gradient updates. The goal is to make high
fanout latents less attractive without relying only on hard masks.

Command shape:

```bash
python3 scripts/research_sae_retrieval_train.py \
  --documents /tmp/ii42_sae_quality_matrix/scidocs/documents.jsonl \
  --queries /tmp/ii42_sae_quality_matrix/scidocs/queries.jsonl \
  --output-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_teacher5_dfcost10_fanout10_mask999 \
  --latent-dims 8192 \
  --active-dims 64 \
  --teacher-doc-neighbors 5 \
  --load-balance-weight 0.2 \
  --df-selection-weight 0.10 \
  --df-selection-mask-ratio 0.999 \
  --fanout-loss-weight 0.10 \
  --epochs 20 \
  --batch-size 128 \
  --train-fraction 0.7 \
  --device mps
```

Result summary:

| Variant | Test R@100 | Test MRR@20 | All R@100 | All MRR@20 | Max DF ratio | Mean DF ratio | SAE candidates | SAE postings touched |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dfcost10_mask999` | `0.7133` | `0.4553` | `0.9120` | `0.8053` | `0.1785` | `0.0163` | `1023.1` | `1958.6` |
| `dfcost10_fanout05_mask999` | `0.7067` | `0.5087` | `0.9120` | `0.8013` | `0.3165` | `0.0153` | `1118.3` | `2164.5` |
| `dfcost10_fanout10_mask999` | `0.7467` | `0.4577` | `0.9220` | `0.7881` | `0.9475` | `0.0162` | `1905.1` | `3737.8` |
| `dfcost10_fanout10_mask90` | `0.7067` | `0.4749` | `0.9075` | `0.8008` | `0.3935` | `0.0159` | `1111.5` | `2244.3` |
| `dfcost05_fanout10_mask999` | `0.7400` | `0.4606` | `0.9200` | `0.8232` | `0.3145` | `0.0259` | `1864.6` | `7468.9` |

Bound simulation summary:

| Variant | Best opened docs | Bound Recall@20 | Bound MRR@20 |
| --- | ---: | ---: | ---: |
| `dfcost10_mask999` | `0.561` | `0.7240` | `0.7729` |
| `dfcost10_fanout05_mask999` | `0.551` | `0.7190` | `0.7668` |
| `dfcost10_fanout10_mask999` | `0.548` | `0.6755` | `0.7531` |
| `dfcost10_fanout10_mask90` | `0.547` | `0.7235` | `0.8003` |
| `dfcost05_fanout10_mask999` | `0.759` | `0.6620` | `0.7636` |

Interpretation:

```text
fanout loss can improve held-out Recall@100.
fanout loss does not reliably improve physical selectivity.
lower mask thresholds reduce DF but can damage held-out recall.
dfcost10_mask999 remains the best efficiency baseline.
dfcost05_fanout10_mask999 is the best quality/selectivity compromise so far.
```

The most interesting positive result is `dfcost05_fanout10_mask999`: it keeps
test Recall@100 at `0.7400`, keeps all-query Recall@100 at `0.9200`, and
reduces max DF ratio from `1.0` to `0.3145`. The cost is that candidate
coverage remains high, so it is not enough for a fast native sparse index.

The negative result is also useful: simply adding fanout loss does not solve
candidate coverage. It can encourage alternative broad dimensions, so the next
phase needs either query/document asymmetry or listwise dense-teacher
distillation with an explicit candidate-budget term.

## Asymmetric Query Encoder and In-Batch Listwise Distillation

This phase tests whether query/document asymmetry can separate two jobs:

```text
document encoder: keep sparse impact selectivity
query encoder: learn dense-teacher semantic matching
```

I added:

- `query_active_dims`: query-side top-k can be smaller than document top-k.
- `listwise_weight`: in-batch dense-teacher distribution distillation.
- `separate_query_encoder`: optional query-only encoder head over the same
  latent space.
- `listwise_query_only`: detach document scores for the listwise loss when
  using the separate query head.

Command shape:

```bash
python3 scripts/research_sae_retrieval_train.py \
  --documents /tmp/ii42_sae_quality_matrix/scidocs/documents.jsonl \
  --queries /tmp/ii42_sae_quality_matrix/scidocs/queries.jsonl \
  --output-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_q32_sepq_listwise05_dfcost10_mask999 \
  --latent-dims 8192 \
  --active-dims 64 \
  --query-active-dims 32 \
  --separate-query-encoder \
  --teacher-doc-neighbors 5 \
  --listwise-weight 0.5 \
  --listwise-query-only \
  --df-selection-weight 0.10 \
  --df-selection-mask-ratio 0.999 \
  --epochs 20 \
  --batch-size 128 \
  --device mps
```

Result summary:

| Variant | Test R@100 | Test MRR@20 | All R@100 | All MRR@20 | Query dims | Doc dims | Max DF ratio | SAE candidates | SAE postings touched |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dfcost10_mask999` | `0.7133` | `0.4553` | `0.9120` | `0.8053` | `26.53` | `16.20` | `0.1785` | `1023.1` | `1958.6` |
| `shared_q32_lw05` | `0.7467` | `0.5597` | `0.9160` | `0.8271` | `32.00` | `63.34` | `0.9455` | `1990.8` | `10402.3` |
| `shared_q32_lw05_l1e3` | `0.7200` | `0.5081` | `0.9140` | `0.7937` | `32.00` | `63.58` | `0.8795` | `1987.9` | `11110.2` |
| `sepq_q32_lw05` | `0.6317` | `0.4086` | `0.8875` | `0.7876` | `32.00` | `12.77` | `0.0855` | `521.6` | `680.6` |
| `sepq_q64_lw05` | `0.6517` | `0.4251` | `0.8935` | `0.8109` | `64.00` | `14.60` | `0.0995` | `979.8` | `1610.4` |

Bound simulation:

| Variant | Best opened docs | Bound Recall@20 | Bound MRR@20 |
| --- | ---: | ---: | ---: |
| `dfcost10_mask999` | `0.561` | `0.7240` | `0.7729` |
| `shared_q32_lw05` | `0.842` | `0.6445` | `0.7587` |
| `shared_q32_lw05_l1e3` | `0.831` | `0.6405` | `0.7799` |
| `sepq_q32_lw05` | `0.463` | `0.7595` | `0.7686` |
| `sepq_q64_lw05` | `0.708` | `0.7275` | `0.7874` |

Interpretation:

```text
shared query/document listwise improves held-out quality.
shared listwise destroys document-side selectivity.
higher activation L1 does not fix shared-listwise fanout.
separate query encoder preserves document selectivity.
separate query encoder currently underfits held-out query recall.
```

The key positive signal is the shared `q32 + listwise0.5` quality result:
test Recall@100 reaches `0.7467` and test MRR@20 reaches `0.5597`. That is the
best held-out ranking signal so far.

The key systems signal is the separate-query result: document max DF ratio can
drop below `0.10`, and SAE candidates can drop to about `522/2000`. This is the
first result with genuinely good sparse-engine fanout, but query recall is not
good enough yet.

So the next training target is now sharper:

```text
keep separate document selectivity
improve separate query-head generalization
```

The likely next move is not more L1. It should be a better query-head teacher:
precompute dense top-k teacher lists for each train anchor and train the query
head against real teacher neighborhoods, not only in-batch negatives.

## Snowflake Query-Prefix Follow-Up

Snowflake Arctic Embed uses asymmetric input formatting: query embeddings are
produced with the `query: ` prefix, while indexed document embeddings are not.
The local BEIR export path already does this correctly. The training problem is
therefore not data export correctness; it is whether the sparse encoder should
share one encoder for both distributions.

This round added three prototype controls:

- separate query-active top-k and query encoder state saved in `sae.pt`;
- dense top-k query-teacher distillation for query-side sparse scores;
- optional query-prefixed pseudo-query records generated from document text.

It also fixed an important training-path distinction: when
`separate_query_encoder` is enabled, document-document teacher anchors should
use the document encoder, not the query encoder. The earlier behavior mixed
document embeddings into the query head and produced optimistic but muddy
signals.

Command shape for pseudo-query generation:

```bash
python3 scripts/research_sae_make_pseudo_queries.py \
  --documents /tmp/ii42_sae_quality_matrix/scidocs/documents.jsonl \
  --output /tmp/ii42_sae_quality_matrix/scidocs/pseudo_queries.jsonl \
  --embedding-dim 768 \
  --batch-size 32
```

Result summary:

| Variant | Test R@100 | Test MRR@20 | All R@100 | Query dims | Doc dims | Max DF ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dfcost10_mask999` | `0.7133` | `0.4553` | `0.9120` | `26.53` | `16.20` | `0.1785` |
| `fixed_anchor_q32_lw05` | `0.3033` | `0.0246` | `0.7870` | `20.87` | `22.77` | `0.6525` |
| `fixed_anchor_q64_lw05` | `0.3017` | `0.0668` | `0.7905` | `40.56` | `23.57` | `0.2985` |
| `freezedoc_q32_teacher_e5` | `0.6733` | `0.4524` | `0.9000` | `29.15` | `16.20` | `0.1785` |
| `pseudo_q32_lw05` | `0.7133` | `0.4164` | `0.9140` | `22.30` | `24.43` | `0.6850` |
| `pseudo_q64_lw05` | `0.6783` | `0.4898` | `0.9035` | `26.55` | `23.92` | `0.7885` |
| `freezedoc_pseudo200_q32_e5` | `0.6733` | `0.4138` | `0.8955` | `31.94` | `16.20` | `0.1785` |
| `freezedoc_pseudo500_q32_e5` | `0.6467` | `0.4100` | `0.8815` | `32.00` | `16.20` | `0.1785` |

The fixed-anchor correction is the clean semantic model, but it exposes a data
scarcity problem: there are only 70 train queries in this SciDocs split, so the
query head underfits badly when it is no longer trained on document anchors.

Naive pseudo-query self-pairs are also not sufficient. They calibrate the
query-prefix distribution but overweight the task "query-prefixed document text
finds itself", which is narrower than real retrieval. With an unfrozen document
encoder, pseudo-query training recovers Recall@100 but destroys document-side
selectivity. With a frozen document encoder, selectivity is preserved but
held-out Recall@100 drops.

The useful positive signal is narrower:

```text
fixed document SAE + small query head can preserve document fanout
dense top-k query-teacher improves query-head MRR without moving doc postings
query-active dims can be reduced, but not enough to create a quality win
```

The next useful training target should be dense-neighborhood distillation, not
self-pair pseudo-query training. For each real or synthetic query anchor, train
against a teacher distribution over multiple dense-nearest documents and add an
explicit candidate-budget penalty on selected query dimensions.

## Dense-Neighborhood Query Distillation

The next prototype changes pseudo queries from direct self-pairs into teacher
anchors:

```text
real query or query-prefixed pseudo anchor
  -> dense top-k document neighborhood
  -> sparse query/document score distribution
```

This added:

- `skip_pseudo_pairs`: load pseudo queries but do not add self-pairs to the
  contrastive pair loader.
- `query_teacher_include_pseudo`: include pseudo-query anchors in dense top-k
  query-teacher lists.
- `query_teacher_pseudo_fraction`: fixed pseudo/real sampling ratio for
  teacher batches, so pseudo anchors calibrate the query-prefix distribution
  without dominating real query supervision.
- `query_fanout_weight`: query-side document-frequency penalty. This is a
  first proxy for candidate-budget pressure, but it is not yet a hard
  selection-time budget.

Representative command:

```bash
python3 scripts/research_sae_retrieval_train.py \
  --documents /tmp/ii42_sae_quality_matrix/scidocs/documents.jsonl \
  --queries /tmp/ii42_sae_quality_matrix/scidocs/queries.jsonl \
  --pseudo-queries /tmp/ii42_sae_quality_matrix/scidocs/pseudo_queries.jsonl \
  --output-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_q32_freezedoc_neighborhood_pseudofrac0p10_fanout0p1_e5_dfcost10_mask999 \
  --latent-dims 8192 \
  --active-dims 64 \
  --query-active-dims 32 \
  --separate-query-encoder \
  --init-model-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_teacher5_dfcost10_mask999/model \
  --freeze-document-model \
  --freeze-selection-state \
  --skip-pseudo-pairs \
  --query-teacher-include-pseudo \
  --query-teacher-pseudo-fraction 0.10 \
  --query-teacher-weight 1.0 \
  --query-teacher-top-k 100 \
  --query-teacher-sample-k 64 \
  --query-fanout-weight 0.1 \
  --epochs 5 \
  --device mps
```

Direct SAE ranking:

| Variant | Test R@100 | Test MRR@20 | All R@100 | Query dims | Doc dims | Max DF ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dfcost10_mask999` | `0.7133` | `0.4553` | `0.9120` | `26.53` | `16.20` | `0.1785` |
| `q32_real_teacher_e5` | `0.6733` | `0.4524` | `0.9000` | `29.15` | `16.20` | `0.1785` |
| `q32_pooled_pseudo` | `0.6733` | `0.3769` | `0.9000` | `31.84` | `16.20` | `0.1785` |
| `q32_pseudo_frac10` | `0.7000` | `0.5072` | `0.9020` | `30.86` | `16.20` | `0.1785` |
| `q32_pseudo_frac25` | `0.7000` | `0.4591` | `0.9020` | `31.18` | `16.20` | `0.1785` |
| `q16_pseudo_frac10` | `0.6667` | `0.3956` | `0.8920` | `15.90` | `16.20` | `0.1785` |
| `q64_pseudo_frac10` | `0.6933` | `0.5254` | `0.9000` | `56.13` | `16.20` | `0.1785` |
| `q32_pseudo_frac10_fanout1` | `0.6933` | `0.5072` | `0.9000` | `30.97` | `16.20` | `0.1785` |

Unified BM25+SAE cost check:

| Variant | Best boundable R@100 | Best boundable MRR@20 | Mean SAE candidates | Mean SAE postings |
| --- | ---: | ---: | ---: | ---: |
| `dfcost10_mask999` | `0.9180` | `0.7797` | `1023.1` | `1958.6` |
| `q16_freezedoc_e5` | `0.9160` | `0.7247` | `891.7` | `1481.9` |
| `q32_pseudo_frac10` | `0.9120` | `0.6924` | `1212.4` | `2379.0` |
| `q64_pseudo_frac10` | `0.9120` | `0.7008` | `1304.0` | `2755.4` |

Interpretation:

```text
natural pooled pseudo anchors are too noisy.
fixed pseudo fraction is necessary.
10% pseudo anchors gives the first clean MRR improvement in direct SAE ranking.
document-side selectivity remains intact because the document SAE is frozen.
query fanout loss is currently too weak to control physical postings.
unified BM25+SAE cost is worse for q32/q64 despite direct-ranking gains.
```

This is a useful research step but not yet a native-index design win. The next
cost-control step should move the query budget into selection itself, not only
into a differentiable post-selection fanout loss:

```text
query latent selection score =
    activation
    - lambda * normalized_doc_frequency
    - mu * expected_candidate_increment
```

## Selection-Time Query Budget

The next prototype implements the first half of that formula directly in the
query top-k gate:

```text
query selection score =
    activation - query_selection_cost_weight * normalized_log_df
```

This is different from `query_fanout_weight`. The earlier fanout loss penalizes
latents after they have already been selected, so it has weak control over
top-k membership. `query_selection_cost_weight` changes the selection score
before top-k, so it directly changes which query latents can open postings.

The document encoder and document selection state remain frozen. Only the query
side receives this additional budget cost, and the resulting
`query_selection_cost.npy` is saved next to the model so query export uses the
same gate.

Representative command:

```bash
python3 scripts/research_sae_retrieval_train.py \
  --documents /tmp/ii42_sae_quality_matrix/scidocs/documents.jsonl \
  --queries /tmp/ii42_sae_quality_matrix/scidocs/queries.jsonl \
  --pseudo-queries /tmp/ii42_sae_quality_matrix/scidocs/pseudo_queries.jsonl \
  --output-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_q32_freezedoc_neighborhood_pseudofrac0p10_qsel0p10_e5_dfcost10_mask999 \
  --latent-dims 8192 \
  --active-dims 64 \
  --query-active-dims 32 \
  --separate-query-encoder \
  --init-model-dir /tmp/ii42_sae_retrieval_train_scidocs_768_a64_teacher5_dfcost10_mask999/model \
  --freeze-document-model \
  --freeze-selection-state \
  --skip-pseudo-pairs \
  --query-teacher-include-pseudo \
  --query-teacher-pseudo-fraction 0.10 \
  --query-teacher-weight 1.0 \
  --query-selection-cost-weight 0.10 \
  --epochs 5 \
  --device mps
```

Direct SAE ranking:

| Variant | Test R@100 | Test MRR@20 | All R@100 | Query dims |
| --- | ---: | ---: | ---: | ---: |
| `q32_pseudo_frac10` | `0.7000` | `0.5072` | `0.9020` | `30.86` |
| `q32_qsel005` | `0.6933` | `0.4680` | `0.9020` | `32.00` |
| `q32_qsel010` | `0.6933` | `0.4486` | `0.9080` | `31.90` |
| `q32_qsel025` | `0.4783` | `0.2471` | `0.6300` | `31.77` |
| `q64_pseudo_frac10` | `0.6933` | `0.5254` | `0.9000` | `56.13` |
| `q64_qsel005` | `0.6867` | `0.4515` | `0.9040` | `63.99` |
| `q64_qsel010` | `0.7000` | `0.4433` | `0.9080` | `62.65` |

Unified BM25+SAE cost check:

| Variant | Best boundable R@100 | Best boundable MRR@20 | Mean SAE candidates | Mean SAE postings |
| --- | ---: | ---: | ---: | ---: |
| `dfcost10_mask999` | `0.9180` | `0.7797` | `1023.1` | `1958.6` |
| `q16_freezedoc_e5` | `0.9160` | `0.7247` | `891.7` | `1481.9` |
| `q32_pseudo_frac10` | `0.9120` | `0.6924` | `1212.4` | `2379.0` |
| `q32_qsel005` | `0.9100` | `0.6987` | `1446.6` | `3221.8` |
| `q32_qsel010` | `0.9080` | `0.7128` | `922.9` | `1558.7` |
| `q64_pseudo_frac10` | `0.9120` | `0.7008` | `1304.0` | `2755.4` |
| `q64_qsel005` | `0.9060` | `0.7108` | `1691.9` | `4809.4` |
| `q64_qsel010` | `0.9080` | `0.7189` | `1015.4` | `1803.4` |

Interpretation:

```text
selection-time query cost has real physical effect.
too much cost destroys direct SAE ranking.
0.10 is the first useful scale for reducing q32/q64 postings.
the reduced-cost variants still trail the older q16_freezedoc_e5 cost/quality
point, so this is not the final route yet.
normalized DF is a crude proxy; the next version should estimate incremental
candidate openings conditioned on already selected query dimensions.
```

## Fixed-Recipe Generalization Matrix

SciDocs-only tuning is not enough evidence for a general retrieval direction,
so I added a fixed-recipe multi-dataset runner:

```text
scripts/research_sae_generalization_matrix.py
```

The runner applies the same recipe to sampled BEIR-style datasets:

```text
scifact, scidocs, nfcorpus, arguana, fiqa
```

This is stronger than judging only on SciDocs, but it is still not true
zero-shot transfer. Each dataset gets its own document SAE baseline. The point
of the matrix is to test whether the same training and query-budget ideas move
quality and physical cost in a consistent direction.

Aggregate result:

| Variant | Direct test R@100 | Direct test MRR@20 | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline_dcost10_mask999` | `0.7898` | `0.4928` | `0.9291` | `0.7642` | `1313.1` | `3146.6` |
| `q16_real_teacher` | `0.7621` | `0.4513` | `0.9028` | `0.7299` | `981.7` | `1714.1` |
| `q32_pseudo_frac10` | `0.7613` | `0.4449` | `0.8971` | `0.7148` | `1110.9` | `2317.0` |
| `q32_qsel010` | `0.7386` | `0.4228` | `0.8875` | `0.7146` | `901.7` | `1695.6` |
| `q64_pseudo_frac10` | `0.7686` | `0.4727` | `0.8991` | `0.7208` | `1176.0` | `2603.9` |
| `q64_qsel010` | `0.7508` | `0.4387` | `0.8941` | `0.7197` | `978.3` | `1907.8` |

Interpretation:

```text
the baseline document SAE remains the best aggregate quality point.
q16_real_teacher is the best simple cost-reduction point.
q32/q64 pseudo-query teacher variants do not generalize enough.
q32/q64 query-selection cost has real physical effect.
the current normalized-DF query cost is too blunt and loses quality.
```

Compared with the baseline, `q16_real_teacher` reduces mean SAE postings by
about `46%`, but loses `0.0263` unified Recall@100 and `0.0343` unified
MRR@20. `q32_qsel010` has slightly better cost than q16, but the quality loss
is larger. This means the selection-time query budget is a real systems lever,
but the current cost signal is not the final one.

Dataset-level behavior is also important:

- SciFact is saturated: most variants keep unified Recall@100 at `1.0000`, so
  it is not discriminative enough for this question.
- SciDocs confirms the earlier local signal: qsel can reduce postings while
  keeping unified Recall@100 close to baseline.
- NFCorpus is the hard failure case: query-budget variants cut candidates
  aggressively, but lose too much recall.
- Arguana keeps unified Recall@100 saturated, but pseudo-query teacher loss is
  high and direct MRR weakens.
- FiQA shows q64 pseudo-query training can improve direct R@100/MRR, but it
  increases postings and does not create a better unified-index point.

The resulting direction is clear: do not continue by simply tuning
`query_active_dims`, pseudo fraction, or normalized DF weight. The next useful
step is an incremental candidate-budget gate:

```text
query latent selection score =
    semantic activation
    - lambda * estimated incremental candidate openings
    - mu * normalized document-frequency cost
```

That gate should estimate the marginal postings opened by each candidate query
latent conditioned on the latents already selected. The current qsel penalty
only knows whether a latent is globally common; it does not know whether it
adds new candidates for this query.

## Incremental Candidate-Budget Gate

I implemented that next gate as a query-time evaluation prototype:

```text
scripts/research_sae_candidate_budget_gate_eval.py
```

The gate keeps the document SAE fixed and changes only query latent selection:

```text
query selection score =
    activation
    - static_selection_cost
    - lambda * normalized_incremental_candidate_openings
```

The `weight=0` path now exactly reproduces the existing query export semantics,
including the subtle case where zero-activation latents can consume top-k slots
after selection cost is applied.

Five-dataset baseline document SAE result:

| Weight | Unified R@100 | Unified MRR@20 | SAE candidates | SAE postings |
| ---: | ---: | ---: | ---: | ---: |
| `0.000` | `0.9291` | `0.7642` | `1313.1` | `3146.6` |
| `0.005` | `0.9296` | `0.7663` | `1279.6` | `3037.0` |
| `0.010` | `0.9308` | `0.7669` | `1249.2` | `2930.6` |
| `0.020` | `0.9303` | `0.7681` | `1177.4` | `2697.8` |
| `0.050` | `0.9270` | `0.7703` | `990.7` | `2139.4` |

Interpretation:

```text
small incremental candidate budgeting is better than static DF-only qsel.
0.010 is the best balanced next default.
0.020 is a cost-favoring profile.
0.050 reduces cost aggressively but starts to look too lossy.
q32/q64 asymmetric query encoders still trail the baseline document SAE.
```

This is the first result in this line that improves average unified quality
and reduces average postings at the same time. The next step should integrate
the same incremental gate into query-teacher training, rather than using it
only as a post-training query encoder.

## Current Interpretation

This phase gives a clearer split:

```text
dense-teacher retrieval training helps recall generalization.
top-k SAE still learns broad semantic dimensions.
soft balancing alone does not create index selectivity.
DF-aware selection cost can reduce postings.
hard DF masking can reduce candidate coverage, but needs better training.
fanout loss improves recall in some settings, but is not a complete
selectivity control.
shared listwise improves quality but breaks document fanout.
separate query encoder fixes fanout but needs stronger query-head teaching.
Snowflake query/document asymmetry is real, but naive pseudo self-pairs do not
solve query-head generalization.
dense-neighborhood query distillation improves direct-ranking MRR, but current
query fanout loss does not yet preserve unified-index cost.
selection-time query budget can preserve unified-index cost, but normalized DF
alone is too blunt and hurts direct-ranking quality across datasets.
incremental query-time candidate budgeting is a better cost signal than static
DF-only qsel, but it still needs training-aware integration and a faster
physical approximation.
```

The next improvement should not be another fusion-weight sweep. The remaining
problem is representation learning plus physical-index co-design.

The next improvement should make selective latents retrieval-aware instead of
only suppressing broad latents after they appear.

## Concrete Next Ideas

1. Retrieval-aware selective-latent training

The DF-cost prototype is still a heuristic. A stronger version should train
selectivity directly:

Target:

```text
held-out Recall@100 >= dense
max_df_ratio << 1.0
mean candidate coverage below 0.3
```

The useful loss should combine:

```text
retrieval score margin
- lambda * expected postings fanout
+ reconstruction stability
```

2. Query/document asymmetric encoder

Use different encoders or different top-k gates:

```text
document encoder: 64 or 128 dims for recall storage
query encoder: 8, 16, or 32 dims selected by expected postings cost
```

The query-side gate should optimize:

```text
semantic hit score - lambda * expected_postings_fanout
```

This directly addresses the failure of naive query top-k and naive DF stoplist.

3. Dense-teacher listwise distillation

Current dense-teacher training uses nearest-neighbor positives with in-batch
negatives. It improves recall but not ranking. A better target is to distill a
small dense top-k distribution per anchor:

```text
KL(sparse scores over teacher top-k || dense teacher distribution)
```

This should improve MRR without needing qrels for every query.

4. Impact-ordered WAND simulator

The current bound simulator uses row blocks. A native sparse index will be
closer to impact-ordered postings with MaxScore/WAND. Implementing that
simulator is the next systems gate. If WAND still touches most postings, the
representation is the bottleneck. If WAND succeeds, the physical index design
becomes much more concrete.

## Candidate-Budget Training Follow-Up

The follow-up phase in
`sae-candidate-budget-training-and-physical-report.md` completed the four
directions above on all current benchmark corpora:

- training-aware query encoder with the same candidate-budget gate used at
  export;
- train/export consistency checks across `scifact`, `scidocs`, `nfcorpus`,
  `arguana`, and `fiqa`;
- impact-ordered MaxScore-style physical traversal simulation;
- baseline-vs-budget physical traversal comparison.

The result is useful but not yet a replacement for the post-training gate:

| Path | Unified R@100 | Unified MRR@20 | SAE postings |
| --- | ---: | ---: | ---: |
| post-training gate `0.010` | `0.9308` | `0.7669` | `2930.6` |
| trained query encoder | `0.9255` | `0.7559` | `2481.6` |

The trained query encoder learns cheaper query latents, but the current dense
teacher objective pulls the query geometry away from the stronger baseline.
That makes it an efficiency signal, not the current quality/default path.

The impact-ordered simulator also clarifies the systems bottleneck:

| Path | Recall@100 | MRR@20 | Postings touched | Touch ratio |
| --- | ---: | ---: | ---: | ---: |
| baseline | `0.9262` | `0.7685` | `2786.0` | `0.889` |
| candidate-budget `0.010` | `0.9252` | `0.7709` | `2605.2` | `0.893` |

So the next high-value iteration should combine:

```text
retention-aware query distillation
+ qrel-positive margin where labels exist
+ candidate-budget selection during training
+ block/impact-max pruning simulation
```

Plain dense-teacher KL is no longer enough. The next training objective must
preserve the original strong budget-gated scores while learning cheaper query
selections.

## Block-Max Physical Follow-Up

The next physical simulator is now `scripts/research_sae_block_max_sim.py`.
Unlike the earlier impact-ordered traversal, it uses safe per-block upper
bounds for SAE-only scores and verifies exact top-k equality.

Five-dataset aggregate, best opened-doc layout per dataset:

| Path | Recall@100 | MRR@20 | Opened docs | Scored docs | Exact candidates | Exact |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | `0.9260` | `0.7737` | `0.524` | `0.434` | `0.651` | `500/500` |
| candidate-budget `0.010` | `0.9242` | `0.7709` | `0.505` | `0.410` | `0.619` | `500/500` |
| retention `50`, 2 epochs | `0.9234` | `0.7676` | `0.490` | `0.389` | `0.585` | `500/500` |

This is the strongest physical-index signal so far. It shows that SAE dominant
dimension layout plus block-max metadata can prune safely and materially. It
does not yet make the system extreme: NFCorpus still opens about `0.828` of
the corpus, so representation selectivity and better block layout remain the
main bottlenecks.

The next layout iteration added `sae_signature`, `simhash`, and `sae_tree`.
`sae_tree` recursively splits documents by balanced latent presence and weight,
then uses top-latent signatures as a leaf fallback.

Updated five-dataset aggregate:

| Path | Layout set | Recall@100 | MRR@20 | Opened docs | Scored docs | Exact |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| baseline | learned layouts | `0.9260` | `0.7737` | `0.471` | `0.403` | `500/500` |
| candidate-budget `0.010` | learned layouts | `0.9242` | `0.7709` | `0.454` | `0.382` | `500/500` |
| retention `50`, 2 epochs | learned layouts | `0.9234` | `0.7676` | `0.435` | `0.358` | `500/500` |

This is now the strongest physical-index signal. The improvement is independent
of the query encoder: learned layouts reduce opened docs for baseline, budget,
and retention paths without changing exactness.
