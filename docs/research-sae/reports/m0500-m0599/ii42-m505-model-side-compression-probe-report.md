# II-42 M505 Model-Side Compression Probe Report

## Summary

M505 tested whether PPLX can reproduce the materialized dense teacher from
model-side inference before we train any posting adapter.

The key result is that the correct surface is the official
`SentenceTransformer` int8 embedding path, not raw `AutoModel` hidden-state
mean pooling.

On a 10-task sampled gate with 512 documents and 64 queries per task,
`st_int8_official` matches the materialized teacher closely:

- Doc cosine: 0.99941 average, 0.99509 worst task;
- Query cosine: 0.99999 average;
- Score Pearson: 0.99963 average, 0.99704 worst task;
- Top100 overlap: 0.99596 average, 0.97265 worst task;
- Row-int8 Top100 overlap: 0.99587 average, 0.97286 worst task.

This is the model-side path that should be preserved and transferred into the
future posting encoder.

## Background

M504 showed that row-wise int8 output/index compression is equivalent to exact
dense and exact hybrid retrieval. M505 asked the next question:

> can we reproduce that dense surface directly from the PPLX model, then use it
> as the base for model compression and posting transfer?

The official PPLX model card states that `pplx-embed-v1-0.6B` uses mean pooling,
requires no instruction prefix, and natively emits unnormalized int8 embeddings
through the `SentenceTransformer` path. The local materialized task root also
stores integer-valued 1024-dimensional embeddings, matching that int8 surface.

## Artifacts

- Script:
  `scripts/research_sae_m505_model_side_compression_probe.py`
- FiQA raw model gate:
  `outputs/m505/fiqa_gate_512/m505_model_side_compression_probe_fiqa_gate_512.json`
- FiQA pooling gate:
  `outputs/m505/pool_gate_fiqa_512/m505_pool_gate_fiqa_512.json`
- FiQA official ST int8 gate:
  `outputs/m505/st_int8_fiqa_512/m505_st_int8_fiqa_512.json`
- Broad10 official ST int8 gate:
  `outputs/m505/st_int8_broad10_sample/m505_st_int8_broad10_sample.json`

## Raw AutoModel Gate

The first probe used `AutoModel` directly, pooled final hidden states, and
compared the result to the materialized teacher.

FiQA, 512 documents / 64 queries:

| Mode | Status | Doc Cos | Query Cos | Score Pearson | Top100 | RowInt8 Top100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `bf16_mean` | `ok` | 0.98985 | 0.99916 | 0.99323 | 0.96047 | 0.95937 |
| `fp32_mean` | `ok` | 0.98994 | 0.99926 | 0.99329 | 0.96125 | 0.96172 |
| `fp16_mean` | `failed` | 0.00000 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |
| `dynamic_int8_cpu_mean` | `failed` | 0.00000 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |
| `bnb_int8_mean` | `failed` | 0.00000 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |
| `bnb_int4_mean` | `failed` | 0.00000 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |

Failure reasons:

- `fp16_mean`: non-finite encoded vectors;
- `dynamic_int8_cpu_mean`: unsupported architecture in dynamic quantized ops;
- `bnb_int8_mean` / `bnb_int4_mean`: `bitsandbytes` unavailable.

Interpretation:

- raw `AutoModel` mean pooling is close but not close enough;
- it cannot be used as the preservation target for compression or adapter
  training;
- fp16 should remain rejected for this model path.

## Pooling Gate

M505 then tested whether the raw AutoModel gap was caused by using the wrong
pooling.

FiQA, 512 documents / 64 queries:

| Mode | Status | Doc Cos | Query Cos | Score Pearson | Top100 | RowInt8 Top100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `bf16_mean` | `ok` | 0.98985 | 0.99916 | 0.99323 | 0.96047 | 0.95937 |
| `bf16_last` | `ok` | 0.77412 | 0.50858 | 0.33494 | 0.52047 | 0.52078 |
| `bf16_cls` | `ok` | 0.67613 | 0.81313 | 0.66735 | 0.56656 | 0.56781 |
| `fp32_mean` | `ok` | 0.98994 | 0.99926 | 0.99329 | 0.96125 | 0.96172 |
| `fp32_last` | `ok` | 0.77514 | 0.50826 | 0.33738 | 0.52188 | 0.52156 |
| `fp32_cls` | `ok` | 0.67586 | 0.81325 | 0.66765 | 0.56766 | 0.56766 |

Interpretation:

- mean pooling is correct;
- last-token and CLS pooling collapse;
- the remaining gap is not a pooling choice problem.

## Official SentenceTransformer Int8 Gate

The official `SentenceTransformer` path downloads the model's remote
`st_quantize.py` and returns the native int8 embedding surface. This matches the
materialized root.

### FiQA Gate

512 documents / 64 queries:

| Mode | Status | Doc Cos | Query Cos | Score Pearson | Top100 | RowInt8 Top100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `st_int8_official` | `ok` | 0.99978 | 1.00000 | 0.99973 | 0.99750 | 0.99750 |

### Broad10 Sampled Gate

512 documents / 64 queries per task:

| Metric | Average | Worst | Best |
| --- | ---: | ---: | ---: |
| Doc cosine | 0.999411 | 0.995091 | 1.000000 |
| Query cosine | 0.999992 | 0.999921 | 1.000000 |
| Score Pearson | 0.999629 | 0.997042 | 1.000000 |
| Top100 overlap | 0.995964 | 0.972653 | 1.000000 |
| Row-int8 Top100 overlap | 0.995866 | 0.972857 | 1.000000 |

Worst task:

- `Touche2020Retrieval.v3`: Top100 0.97265, Score Pearson 0.99704.

All other tasks are at or above 0.99484 Top100 overlap.

## Decision

Promote `st_int8_official` as the model-side preservation surface.

Do not use raw `AutoModel` final hidden mean as the compression or adapter
target. It is useful for diagnostics, but it loses too much ranking
neighborhood structure compared with the official int8 surface.

M505 changes the next stage:

1. preserve the official PPLX ST/TEI int8 surface first;
2. if model-side compression is needed, compress the official path or use the
   ONNX/TEI int8 outputs, not raw hidden-state pooling;
3. only then add a posting adapter or text-to-posting head;
4. keep fp16 rejected unless a later runtime path explicitly fixes the NaNs;
5. treat int4 as a later lower-bound study, not the next default target.

## Next Stage

M506 should test a PPLX-initialized posting adapter on top of the official int8
surface:

- input: official `SentenceTransformer` or TEI int8 PPLX embedding;
- first target: reproduce existing row-int8 dense retrieval and structural
  posting targets;
- second target: learn an indexable signed-coordinate/posting representation;
- third target: add fanout/load constraints;
- only after preservation passes: add listwise or qrels-aware ranking loss.

If runtime efficiency is the immediate blocker, the best engineering route is
not model pruning yet. It is to benchmark the official ONNX/TEI int8 path and
avoid raw AutoModel pooling entirely.
