# M1913 Granite 30M Sparse Native Control Contract

Date: 2026-07-12

Status: **complete; see `docs/research-sae/reports/m1900-m1999/ii42-m1913-granite-30m-sparse-native-control-report.md`**

## Question

M1905 showed that a mature vocabulary output is substantially stronger than
the bounded from-scratch SAE-SPLADE basis. M1660 already proved that a
vocabulary-sparse surface can execute exactly in one BMP inverted index. M1913
therefore asks a narrower product question:

> Does a released, compact, dense-root learned-sparse checkpoint preserve its
> quality after its documented inference pruning and exact BMP quantization?

This is a frozen-checkpoint control. It does not train or tune on FiQA.

## Pinned Artifact

- Model: `ibm-granite/granite-embedding-30m-sparse`.
- Revision: `ad82b1fd09541c998c8d45045d601c51fdb8a9b7`.
- License: Apache-2.0.
- Architecture: six-layer RoBERTa-like masked-language model, hidden size 384,
  vocabulary size 50,265, and about 30.3M parameters.
- Published training shape: retrieval-oriented pretraining followed by
  contrastive knowledge distillation from a larger dense retriever, with FLOPS
  and total-NORM regularization.

IBM publishes the checkpoint and high-level recipe, but the original training
mix includes IBM-internal and generated data. M1913 can establish an auditable
checkpoint and engine parent; it cannot claim a from-scratch reproduction of
the proprietary training run.

## Frozen Representation

The surface must match the released Sentence Transformers modules:

1. masked-language-model logits;
2. attention-mask multiplication;
3. `ReLU`, then `log1p`;
4. maximum pooling across sequence positions;
5. top 192 active document dimensions and top 50 active query dimensions;
6. nonnegative sparse dot product.

Both query and document maximum sequence lengths are 512. No prompt, IDF,
BM25 score, lexical fusion, special-token removal, dataset-specific threshold,
or post-hoc pruning is allowed. The 192/50 limits come from the pinned model
card and are fixed before evaluation.

## Evaluation

- Dataset: complete official FiQA, 57,638 documents and 648 judged queries.
- Float metrics: NDCG@10, MAP@100, Recall@100, MRR@20, and candidate upper
  bound at 1,000.
- Surface diagnostics: document/query nnz, vocabulary utilization, maxDF,
  serialized sparse bytes, and artifact signatures.
- Native closure: existing patched M1660 BMP, block size 16, u8 document
  impacts, f32-to-u32 query impacts, exact top 100, source document order.
- Native metrics: exactness, quantized metric retention, index bytes, build and
  load time, and p50/p95 latency against exhaustive integer scoring.

FiQA is not named in the public training table, but undisclosed internal and
synthetic data prevent a strict untouched-row claim. M1913 is a mature product
control, not clean generalization evidence.

## Gates

M1913 becomes the preferred compact checkpoint parent only if all hold:

1. float Recall@100 is at least the OpenSearch-v2 value `0.656042`;
2. float NDCG@10 and MAP@100 are no more than 5% relatively below OpenSearch-v2
   values `0.370244` and `0.310860`;
3. BMP retains at least 99% of float Recall@100;
4. all strict exact-top-100 checks pass;
5. BMP p95 is lower than exhaustive sparse p95;
6. index bytes, nnz, maxDF, and latency are reported without a rescue surface.

If Granite passes, later optimization must start from this frozen checkpoint
or its published dense-root recipe and preserve M1913 as a regression anchor.
If it fails quality but passes native cost, retain OpenSearch v2 as the product
checkpoint and the Apache Unified LSR framework as the trainable parent.

## Stop Rules

- Do not search active dimensions, sequence lengths, or score transforms on
  FiQA.
- Do not compare an unpruned raw surface against the card's pruned quality
  claim.
- Do not interpret a model-card aggregate as a local native result.
- Do not call the original training fully reproducible without the unavailable
  internal data and exact sampling schedule.

## Primary Sources

- Model: <https://huggingface.co/ibm-granite/granite-embedding-30m-sparse>
- Paper: <https://arxiv.org/abs/2502.20204>
- Repository: <https://github.com/ibm-granite/granite-embedding-models>
