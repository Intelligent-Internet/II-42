# II-42 M364 Dense Autoencoder Gate Report

## Goal

M364 continues the dense-only route from M363. It still excludes BM25, qrels,
scorer features, and admission objectives.

The goal is to turn dense-only sparse atom training into a real checkpointed
gate:

- train sparse atom codes from existing dense embeddings;
- add code-space score preservation without using BM25;
- select best checkpoint by exact dense top-k overlap, not by final loss;
- measure multi-seed stability before deciding whether the route is real.

## Implementation

Script:

- `scripts/research_sae_m364_dense_autoencoder_gate.py`

Key changes over M362/M363:

- saves a best checkpoint per variant;
- chooses best checkpoint by `(Exact O@10, Exact O@20, Exact O@100)`;
- adds code-space Gram preservation loss;
- adds a lightweight decoder coherence loss;
- fixes sampling reproducibility by using Python `random.Random`, matching the
  earlier M362 sampling convention.

## Main Local Gate

All runs are local CPU jobs over the M80 dense embeddings:

- docs: 4096
- queries: 128
- latent dims: 2048
- active atoms: 256
- epochs: 20 for seed matrix
- evaluation: exact sparse atom ranking overlap against exact dense ranking

## Multi-Seed Result

| Seed | Variant | Best epoch | Exact O@10 | Exact O@20 | Exact O@100 | Candidate O@10 | NDCG tax |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 361 | recon | 10 | 0.6455 | 0.6000 | 0.6155 | 0.6636 | -0.2712 |
| 361 | recon + gram/coh | 20 | 0.6364 | 0.6364 | 0.6114 | 0.6409 | -0.3053 |
| 362 | recon | 10 | 0.7304 | 0.6913 | 0.6591 | 0.7304 | -0.1972 |
| 362 | recon + gram/coh | 20 | 0.7478 | 0.7109 | 0.6530 | 0.7522 | -0.1937 |
| 363 | recon | 15 | 0.6900 | 0.6550 | 0.6385 | 0.6950 | -0.2427 |
| 363 | recon + gram/coh | 10 | 0.7150 | 0.6700 | 0.6475 | 0.7200 | -0.2097 |
| 364 | recon | 10 | 0.6480 | 0.6100 | 0.6184 | 0.6520 | -0.2761 |
| 364 | recon + gram/coh | 15 | 0.7240 | 0.6580 | 0.6708 | 0.7240 | -0.2116 |

## Aggregate

| Variant | Mean O@10 | Min O@10 | Max O@10 | Mean O@100 | Mean NDCG tax |
| --- | ---: | ---: | ---: | ---: | ---: |
| recon | 0.6785 | 0.6455 | 0.7304 | 0.6329 | -0.2468 |
| recon + gram/coh | 0.7058 | 0.6364 | 0.7478 | 0.6457 | -0.2301 |

## Interpretation

The coherence/Gram route is a real but small improvement:

- mean O@10 improves by +0.0273;
- mean O@100 improves by +0.0128;
- mean NDCG tax improves by +0.0167;
- 3 of 4 seeds improve on O@10;
- seed 361 regresses, so the method is not yet robust.

This does not pass the M363 promotion threshold:

- target O@10: >= 0.85;
- target O@100: >= 0.72;
- observed best O@10: 0.7478;
- observed best O@100: 0.6708.

The current dense-only representation route is therefore promising but still not
strong enough to begin text-to-atom distillation or qrel contrastive fine-tuning.

## Root Cause Update

The project-level suspicion remains supported: downstream BM25/admission/ranking
objectives were introduced before dense representation fidelity was high enough.

However, M364 adds an important correction:

- a single lucky dense-only result is not enough;
- this gate has high seed/sample variance;
- first-stage representation training needs robust multi-seed promotion, not a
  single best run.

## Next Step

M365 should stay dense-only and attack the remaining gap directly:

1. Use a larger fixed eval gate, ideally 16k docs and 256 queries if local RAM
   permits, so the gate is less seed-sensitive.
2. Add a projection-head variant where the sparse code dot is trained against
   dense dot while reconstruction still uses a decoder.
3. Test softer training-to-hard-inference: train with softplus or Gumbel/soft
   TopK, evaluate with hard TopK.
4. Track both mean and worst-seed Exact O@10. Do not promote until both improve.

No BM25 or qrel objective should be added until this dense-only gate is much
closer to dense.
