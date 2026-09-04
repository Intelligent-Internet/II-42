# M1962 Frozen M190 Official BMP Result

Decision: **retain_raw_m190_skip_representation_df_pruning**

## Frozen FiQA Result

| Surface | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M190 float native | 0.430176 | 0.374315 | 0.742807 | 0.514972 | 0.928521 |
| M190 u8/u32 BMP | 0.429933 | 0.375032 | 0.744710 | 0.514801 | 0.929293 |
| Delta | -0.000242 | +0.000717 | +0.001903 | -0.000171 | +0.000772 |

## Exactness And Cost

- score-multiset parity: `1.000000`
- strict-boundary parity: `1.000000`
- quantized Recall retention: `1.002562`
- BMP p50/p95: `5.647` / `7.203 ms`
- exhaustive p95: `12.779 ms`
- p95 speedup: `1.774x`
- index bytes: `69169966`
- bytes/document: `1200.1`
- build/load: `1.512` / `0.148 s`
- queries requiring u32 scores: `78`

## Interpretation

The unchanged M190 learned-impact representation passes exact BMP,
quantization, size, and latency gates. PostgreSQL posting-union touch
was therefore an executor diagnostic, not evidence that M190 support
must be pruned. Static query-DF pruning would discard potentially
salient atoms before the validated engine needs it and is paused as a
representation route.

This does not promote M190 over dense or P2.1. Broad9/full15 quality
and unseen-corpus validation remain open, and CQADupStack is already a
known negative row. M1962 only closes the FiQA engine-cost question.
