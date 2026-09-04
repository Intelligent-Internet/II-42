# SAE Phase 5 Gap Exploration Report

## Configuration

- `data_root`: `/tmp/ii42_sae_quality_matrix`
- `datasets`: `['scifact', 'scidocs', 'nfcorpus', 'arguana', 'fiqa']`
- `run_name`: `sae_8192_64`
- `top_k`: `100`
- `score_mode`: `normalized_idf_dot`

## Baseline Dependency Probe

- `FlagEmbedding`: `False`
- `BGE-M3 artifact dirs`: `9`
- `SPLADE`: `not_run; no local SPLADE artifact or dependency was selected for this unattended pass`

## Score Contract Mean Quality

| Run | `recall_at_20` | `recall_at_100` | `mrr_at_20` | `ndcg_at_10` | `map_at_100` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `normalized` | 0.7045 | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `raw` | 0.6431 | 0.7470 | 0.6315 | 0.5457 | 0.4522 |
| `fixed_saturation` | 0.6971 | 0.7934 | 0.6742 | 0.5952 | 0.4987 |

## Tri-Hybrid Mean Quality

| Run | `recall_at_20` | `recall_at_100` | `mrr_at_20` | `ndcg_at_10` | `map_at_100` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `bm25_dense` | 0.6947 | 0.7817 | 0.6860 | 0.5984 | 0.5010 |
| `bm25_sae` | 0.7045 | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_dense_sae` | 0.7141 | 0.8019 | 0.6927 | 0.6168 | 0.5147 |

## Query-Dimension Budget

| Run | `recall_at_100` | `mrr_at_20` |
| --- | ---: | ---: |
| `4` | 0.7651 | 0.6116 |
| `8` | 0.7724 | 0.6392 |
| `12` | 0.7801 | 0.6404 |
| `16` | 0.7833 | 0.6606 |
| `32` | 0.7889 | 0.6735 |
| `64` | 0.7947 | 0.6835 |

## Document-Dimension Budget

| Run | `recall_at_100` | `mrr_at_20` |
| --- | ---: | ---: |
| `16` | 0.7799 | 0.6552 |
| `32` | 0.7799 | 0.6690 |
| `48` | 0.7904 | 0.6764 |
| `64` | 0.7947 | 0.6835 |

## Corpus Bigram Vocabulary

| Run | `recall_at_100` | `mrr_at_20` |
| --- | ---: | ---: |
| `bm25_bigram` | 0.6949 | 0.5570 |
| `bm25_bigram_sae` | 0.7943 | 0.6708 |

## Physical Probe Mean Counters

| Counter | Mean |
| --- | ---: |
| `sae_posting_p95` | 194.6000 |
| `query_sae_touch_p95` | 8476.4000 |
| `doc_vector_pairs_mean` | 64.0000 |

## Real Workload Readiness

- `matched_artifact_count`: `0`
- `status`: `no canonical arxiv/pubmed/commons qrels matrix is wired into this Phase 5 runner`

## Interpretation

- Fixed-saturation quality is the key native-scorer gate because it is compatible with exact upper bounds.
- Query and document budget rows identify whether the SAE scorer is representation-limited or only implementation-limited.
- Bigram lexical expansion tests whether a cheaper corpus-specific sparse vocabulary can cover part of the semantic gap.
- Physical counters keep the Seismic/SINDI lessons tied to the current payload: high fanout and resident doc-vector size must both be visible before mutable index work starts.

