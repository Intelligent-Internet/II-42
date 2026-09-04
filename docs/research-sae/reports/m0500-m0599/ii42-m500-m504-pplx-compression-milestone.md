# II-42 M500-M504 PPLX Compression Milestone

## Summary

M500-M504 changed the center of the encoder discussion.

Earlier M409-M417 attempts showed that a small student still struggles to
rediscover the PPLX dense semantic space. M500-M504 therefore tested a different
route: compress PPLX behavior directly and use dense-equivalence as the gate
before returning to a trainable encoder.

The result is clear:

- naive PPLX layer trimming is not viable;
- the final PPLX surface is stable in `bf16`;
- row-wise int8 output/index compression is dense-equivalent across the visible
  10-task materialized retrieval root;
- row-wise int8 remains equivalent after BM25 z-blending;
- raw dimension pruning and raw topK sparsification are not the right route.

## Experiment Chain

### M500: Hidden-State and DType Probe

M500 tested whether intermediate PPLX hidden layers could replace the final
embedding surface.

Result:

- only final `layer28_mean` preserved the teacher surface;
- `layer27_mean` and earlier layers lost most dense/posting geometry;
- `bf16` final surface was stable;
- `fp16` model path produced NaNs and ranking collapse.

Decision:

- reject simple layer truncation;
- start compression from the final surface, not from intermediate hidden
  layers.

### M501: Final Surface Compression

M501 compressed the final materialized dense vectors directly.

Large FiQA sample:

| Candidate | Ratio | Top100 | Pearson | Doc Active J | Query Active J |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_fp32` | 1.000 | 1.00000 | 1.00000 | 1.00000 | 1.00000 |
| `row_int8` | 0.251 | 0.99670 | 0.99999 | 0.98949 | 0.99105 |
| `row_int4` | 0.126 | 0.95730 | 0.99747 | 0.84678 | 0.85722 |

Decision:

- promote row-wise int8;
- keep row-wise int4 as exploratory/approximate;
- reject raw shared-dimension pruning and raw row-topK sparsification.

### M502: FiQA Qrels Check

M502 tested exact dense, compressed dense, BM25, and hybrid on FiQA qrels.

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.51808 | 0.45889 | 0.83347 | 0.60380 |
| `row_int8` | 0.51753 | 0.45881 | 0.83347 | 0.60370 |
| `exact_dense_bm25_zblend` | 0.52936 | 0.46852 | 0.83244 | 0.61352 |
| `row_int8_bm25_zblend` | 0.52941 | 0.46841 | 0.82986 | 0.61342 |

Decision:

- row-int8 is effectively dense-equivalent on FiQA;
- row-int8 also stays equivalent after a small BM25 blend on FiQA.

### M503: Broad Dense-Only Validation

M503 synced the full 10-task materialized root from `spark-2` to `spark-1` and
ran broad dense-only compressed validation.

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.57268 | 0.43538 | 0.74805 | 0.65183 | 1.00000 |
| `row_int8` | 0.57270 | 0.43543 | 0.74799 | 0.65188 | 0.99664 |
| `row_int4` | 0.57364 | 0.43548 | 0.74780 | 0.65187 | 0.94834 |

Decision:

- promote `row_int8` as the M500-series mainline;
- treat `row_int4` as a secondary approximate tier, not a strict replacement;
- use row-int8 equivalence as the gate for model-side compression and future
  posting-encoder training.

### M504: Broad Hybrid Validation

M504 tested whether row-wise int8 remains equivalent after BM25 z-blending on
the same 10-task materialized root.

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 | Hybrid O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_bm25_zblend` | 0.59061 | 0.44781 | 0.75511 | 0.66395 | 0.87680 | 1.00000 |
| `row_int8_bm25_zblend` | 0.59052 | 0.44793 | 0.75495 | 0.66408 | 0.87682 | 0.99687 |
| `row_int4_bm25_zblend` | 0.59073 | 0.44760 | 0.75491 | 0.66433 | 0.86781 | 0.95471 |

Decision:

- promote row-int8 as the compressed dense component for both dense-only and
  hybrid retrieval;
- keep row-int4 as an approximate/lower-bit tier only;
- move the next stage from output compression to model-side compression.

## Scientific Readout

The important shift is this:

- From-scratch small encoder training was trying to learn too much at once:
  language semantics, dense geometry, posting geometry, index constraints, and
  ranking behavior.
- M500-M504 show that the PPLX dense surface itself can be compressed with
  almost no retrieval loss.
- Therefore, the next encoder should not start from random or a tiny generic
  student. It should start from a PPLX-derived compressed teacher or a
  PPLX-initialized model and then learn posting/index specialization.

This changes the problem from:

> train a small model to rediscover PPLX dense retrieval

to:

> preserve/compress PPLX, then transfer or adapt it into an indexable posting
> generator.

## Current Assets

- M500 plan/report/script:
  `docs/research-sae/reports/m0500-m0599/ii42-m500-retrieval-aware-pplx-trim-plan.md`,
  `docs/research-sae/reports/m0500-m0599/ii42-m500-retrieval-aware-pplx-trim-report.md`,
  `scripts/research_sae_m500_retrieval_aware_pplx_trim.py`
- M501 report/script:
  `docs/research-sae/reports/m0500-m0599/ii42-m501-final-surface-compression-report.md`,
  `scripts/research_sae_m501_final_surface_compression.py`
- M502 report/script:
  `docs/research-sae/reports/m0500-m0599/ii42-m502-compressed-surface-retrieval-report.md`,
  `scripts/research_sae_m502_compressed_surface_retrieval.py`
- M503 report/script:
  `docs/research-sae/reports/m0500-m0599/ii42-m503-broad-compressed-dense-report.md`,
  `scripts/research_sae_m503_broad_compressed_dense.py`
- M504 report/script:
  `docs/research-sae/reports/m0500-m0599/ii42-m504-broad-hybrid-compressed-dense-report.md`,
  `scripts/research_sae_m504_broad_hybrid_compressed_dense.py`
- Output matrices:
  `outputs/m500`, `outputs/m501`, `outputs/m502`, `outputs/m503`,
  `outputs/m504`

## Next Milestone

M505 should begin model-side transfer:

- weight-only int8/int4 or QAT PPLX compression;
- PPLX-initialized posting head or LoRA adapter;
- supervised dense/posting preservation first;
- only then listwise/RL ranking optimization.
