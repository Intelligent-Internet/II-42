# M1640 Learned-Sparse Engine Control Report

Date: 2026-07-11

Decision: **retain the vocabulary-sparse representation mechanism; close the
current contiguous fixed-block engine proxy and do not authorize a pooled-dense
output-head continuation.**

## Question

M1630 showed that exact signed PCA postings can preserve dense scores but have
the wrong access distribution. M1640 tested whether an established
retrieval-trained vocabulary output changes that conclusion before any new
model training.

Two fixed public checkpoints were evaluated without pruning or adaptation:

- `naver/splade-v3-distilbert`, revision
  `2db06b86d65e316e2ca9907aa1aa8be6f8c4e739`;
- `opensearch-project/opensearch-neural-sparse-encoding-v2-distill`, revision
  `269e6638b2c4f648996691f6d751495285d8f330`.

The OpenSearch checkpoint was predeclared as the single independent
Apache-2.0 control after the Naver model missed one shared row.

## Shared3 Representation Result

| Model | Dataset | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 | Doc nnz | Query nnz | Max DF |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Naver | NFCorpus | 0.421740 | 0.207145 | 0.316356 | 0.679398 | 0.3245 | 280.9 | 17.4 | 0.9006 |
| Naver | SciFact | 0.780632 | 0.755618 | 0.958000 | 0.770331 | 0.3958 | 295.2 | 112.9 | 0.8255 |
| Naver | FiQA | 0.621689 | 0.564381 | 0.858687 | 0.663262 | 0.4411 | 271.5 | 22.5 | 0.9440 |
| OpenSearch | NFCorpus | 0.417417 | 0.206131 | 0.330782 | 0.695324 | 0.3520 | 232.2 | 111.1 | 0.5889 |
| OpenSearch | SciFact | 0.806301 | 0.781043 | 0.978000 | 0.787799 | 0.4154 | 238.0 | 135.0 | 0.7420 |
| OpenSearch | FiQA | 0.642541 | 0.589346 | 0.874615 | 0.693783 | 0.4860 | 227.9 | 129.5 | 0.4920 |

The Naver root retained only `81.8%` of dense NFCorpus Recall@100 and failed
the locked 85% row gate. The OpenSearch root passed NDCG and Recall retention
on all three rows. Every shared exact-block ranking had top100 parity `1.0`.

This is a real mechanism result: normalized vocabulary impacts avoid the
universal max-DF collapse seen in the old SAE/codebook roots while retaining
substantial dense and retrieval quality. It is not a clean-heldout project
winner because these public models have their own training provenance.

## Complete FiQA Exact Engine

The complete surface contains 57,638 documents and 648 evaluated queries.
Mean document/query supports are `228.76/129.00`, with document max DF
`0.48768`. Exact sparse retrieval produced:

| NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 | Posting-touch UB |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0.370244 | 0.310860 | 0.656042 | 0.456006 | 0.849637 | 0.999228 |

The two exact block shapes expose a cost frontier:

| Block | Parity | Decoded/full | Word/full | Finalized docs | Opened blocks | Metadata/doc |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 64 | 1.000000 | 0.699170 | 0.180883 | 68.60% | 617.8 / 901 | 1,598.6 B |
| 16 | 1.000000 | 0.132932 | 0.462425 | 11.31% | 407.3 / 3,603 | 2,590.7 B |

Block64 passes metadata work but fails the locked 60% traversal gate. The
single paper-native block16 correction passes traversal by a wide margin but
fails the 30% metadata-work gate. It also preserves exact top100 for all 648
queries. No b8/b32 search is justified: smaller blocks trade decoded postings
for more block-range operations rather than removing the structural trade-off.

## Interpretation

Three conclusions must remain separate:

1. **Representation mechanism passes.** A retrieval-trained normalized
   vocabulary carries useful semantic quality in one posting map.
2. **The Python fixed-block proxy fails.** Its uncompressed range metadata and
   source document order cannot satisfy both cost gates at once.
3. **Learned sparse engines are not disproved.** BMP uses BP document ordering,
   quantized impacts, compressed range maxima, SIMD aggregation, and a hybrid
   block-forward layout. The current simulator implements only the safe bound
   shape, not that complete system.

The result therefore does not authorize model loss to compensate for an engine
gap. A product claim requires a real BMP/BP-ordered implementation or the
native PostgreSQL plugin path with measured latency, index bytes, and exactness.

## Route Decision

- Stop M1641's proposed pooled-dense vocabulary head. M1540 already closed the
  generic output-head family, and M1640 did not provide an engine pass that
  justifies reopening it.
- Stop block-size grids, static TopK, DF thresholds, and approximate admission.
- Retain the Apache OpenSearch root as the fixed sparse-foundation control.
- Permit M1650 S1 only: audit whether a frozen BGE dense teacher and the frozen
  sparse root provide a complementary candidate distribution. This is not yet
  training or promotion.
- If S1 passes, one bounded continuation of the complete retrieval-trained
  sparse checkpoint is allowed. Deployment remains one sparse encoder and one
  inverted index; dense is a training-only teacher.

## Literature Alignment

- BMP (`2405.01117`) predicts the observed block-size trade-off and supplies
  the missing BP ordering/compression mechanisms.
- Vocabulary Transfer (`2607.00004`) supports normalized vocabulary alignment
  and warns that head-only adaptation is inferior to complete alignment.
- Dense-to-sparse probabilistic expansion control (`2402.17535`) supports a
  frozen dense teacher while identifying co-activation as a primary risk.
- The OpenSearch method (`2411.04403`) supplies the stable single-index route:
  a dense+sparse ensemble teacher, normalized scores, and IDF-aware sparse
  training.

The II-Commons arXiv cutoff for this review was `2026-07-10`.

## Artifacts

- Contract: `docs/research-sae/reports/m1600-m1699/ii42-m1640-vocabulary-sparse-control-contract.md`
- Block16 contract: `docs/research-sae/reports/m1600-m1699/ii42-m1640c-paper-native-block16-contract.md`
- Evaluator: `scripts/research_sae_m1640_splade_engine_control.py`
- Runner: `scripts/run_m1640_splade_engine_control_spark.sh`
- Full block64 ClearML: `9e30930eb1dd4cacb9169f3d2ef2b7ed`
- Full block16 ClearML: `8c6ebf497d264845abf5334c2cac1ab3`

Focused tests, Python compilation, and shell syntax checks passed before the
complete runs. No M1640 process remains on spark-1.
