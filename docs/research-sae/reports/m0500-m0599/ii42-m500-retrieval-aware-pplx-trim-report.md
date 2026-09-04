# II-42 M500 Retrieval-Aware PPLX Trim Report

## Summary

M500 tests whether PPLX can be compressed directly, instead of training a
small encoder from scratch to rediscover the PPLX dense retrieval space.

The first stage is a non-training hidden-state probe:

- teacher: materialized PPLX dense matrices;
- model: `perplexity-ai/pplx-embed-v1-0.6B`;
- task: `FiQA2018`;
- machine: `spark-1`;
- container: `nvcr.io/nvidia/pytorch:26.03-py3`;
- run root:
  `/home/huoju/leask/runs/ii42-m500-retrieval-aware-pplx-trim-v1`.

This probe does not physically truncate the model. It tests whether an
intermediate hidden layer already preserves the dense and posting geometry
well enough to justify real layer trimming.

## Artifacts

- Plan: `docs/research-sae/reports/m0500-m0599/ii42-m500-retrieval-aware-pplx-trim-plan.md`
- Script: `scripts/research_sae_m500_retrieval_aware_pplx_trim.py`
- Smoke JSON: `outputs/m500/m500_pplx_trim_probe.json`
- Smoke report: `outputs/m500/m500_pplx_trim_probe.md`
- Tail-sweep JSON: `outputs/m500/tail_sweep_fiqa/m500_pplx_trim_probe.json`
- Tail-sweep report: `outputs/m500/tail_sweep_fiqa/m500_pplx_trim_probe.md`
- DType probe reports:
  `outputs/m500/dtype_probe_fiqa/{fp32,fp16,bf16}/m500_dtype_probe_*.md`

## Smoke Probe

Config:

- samples: 64 docs, 16 queries;
- layers: auto (`0`, `7`, `14`, `21`, `28`);
- pools: `mean`, `last`;
- top-k overlap: `10`, `50`.

| Candidate | Doc Cos | Query Cos | Top10 | Top50 | Doc Active J | Query Active J | Pearson |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `layer28_mean` | 0.98879 | 0.99921 | 0.93750 | 0.98625 | 0.90314 | 0.95164 | 0.99279 |
| `layer28_last` | 0.74882 | 0.53903 | 0.43125 | 0.85125 | 0.39915 | 0.25561 | 0.34927 |
| `layer14_mean` | 0.03255 | 0.05721 | 0.32500 | 0.81875 | 0.09390 | 0.08887 | 0.25890 |
| `layer21_mean` | 0.05296 | 0.04634 | 0.28750 | 0.81000 | 0.13111 | 0.15439 | 0.20912 |

Only the final layer with mean pooling preserves the teacher geometry.

## Tail-Layer Sweep

Config:

- samples: 256 docs, 64 queries;
- layers: `22,23,24,25,26,27,28`;
- pool: `mean`;
- top-k overlap: `10`, `50`, `100`.

| Candidate | Doc Cos | Query Cos | Top10 | Top50 | Top100 | Doc Active J | Query Active J | Pearson |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `layer28_mean` | 0.98721 | 0.99934 | 0.91250 | 0.95656 | 0.97141 | 0.90640 | 0.95512 | 0.99189 |
| `layer27_mean` | 0.19675 | 0.22521 | 0.20625 | 0.37406 | 0.51484 | 0.29044 | 0.45827 | 0.34253 |
| `layer26_mean` | 0.18142 | 0.19492 | 0.19844 | 0.38312 | 0.51734 | 0.28443 | 0.38036 | 0.35159 |
| `layer25_mean` | 0.14024 | 0.14552 | 0.17656 | 0.36406 | 0.49578 | 0.22942 | 0.30548 | 0.32781 |
| `layer24_mean` | 0.11516 | 0.11135 | 0.16250 | 0.34906 | 0.48594 | 0.18939 | 0.24885 | 0.30385 |
| `layer23_mean` | 0.09281 | 0.08740 | 0.15937 | 0.34969 | 0.48281 | 0.16269 | 0.21009 | 0.27920 |
| `layer22_mean` | 0.08362 | 0.07267 | 0.17969 | 0.36500 | 0.49875 | 0.16135 | 0.19376 | 0.26252 |

## Interpretation

The result is a strong negative signal for simple layer truncation:

- `layer28_mean` is effectively the final teacher surface.
- `layer27_mean` and earlier layers lose most dense/posting geometry.
- query-side geometry collapses especially hard before the final layer.
- last-token pooling is not a viable substitute for the teacher surface.

Therefore, the current evidence does not support "cut PPLX to 75% depth" or
"use an intermediate layer as a small encoder".

## DType Probe

After rejecting naive depth trimming, M500 ran a second non-training probe on
the same `FiQA2018` sample. This keeps only the final representation and
changes model dtype.

Config:

- samples: 256 docs, 64 queries;
- layer: `28`;
- pool: `mean`;
- dtypes: `fp32`, `fp16`, `bf16`.

| DType | Doc Cos | Query Cos | Top10 | Top50 | Top100 | Doc Active J | Query Active J | Pearson |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `fp32` | 0.98719 | 0.99925 | 0.92344 | 0.95937 | 0.96984 | 0.91375 | 0.95743 | 0.99464 |
| `bf16` | 0.98710 | 0.99916 | 0.91875 | 0.95844 | 0.97016 | 0.90997 | 0.95326 | 0.99458 |
| `fp16` | NaN | NaN | 0.04063 | 0.20781 | 0.39687 | 0.07963 | 0.55759 | NaN |

The dtype result is more promising than layer trimming:

- `bf16` preserves the final dense/posting surface almost exactly.
- `fp16` is unsafe for this model path without extra stabilization.
- low-precision compression should start from `bf16` and then test int8/QAT,
  not from plain fp16 or intermediate-layer truncation.

## Next Step

The promising compression direction is not simple layer truncation. It should
move to retrieval-aware compression inside the final model:

1. quantization probe: promote `bf16`; test INT8/INT4 or QAT against
   dense/posting preservation metrics;
2. final-block structured pruning: heads and FFN channels with retrieval-aware
   importance;
3. adapter/head distillation from the full final representation to the M396
   posting target;
4. only after this, test actual BEIR/MTEB retrieval.

M500 Stage A is complete enough to reject naive layer trimming and route the
line toward final-block pruning or quantization-aware compression.
