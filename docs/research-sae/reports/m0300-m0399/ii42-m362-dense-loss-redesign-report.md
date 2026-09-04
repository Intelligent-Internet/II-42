# M362 Dense Loss Redesign Report

## Question

The working suspicion was that the SAE route may have been wrong from the
training loss itself. M362 tests that directly on dense embeddings before any
BM25, scorer, admission, token pooling, or qrel objective is involved.

The clean question:

> Can a sparse projector preserve dense neighborhoods if the loss is designed
> from first principles?

## Setup

Data:

- M80 dense embedding artifact:
  `/Volumes/Betty/Tmp/psql_bm25s_sae_m80/neutral-stage-a-v0`
- documents: 4096 sampled 768-dimensional rows
- queries: 128 sampled 768-dimensional rows
- evaluation split: non-train query rows from the sampled query surface

Model:

- shared dense-embedding sparse projector
- latent dims: 1024
- hard TopK evaluation
- no BM25
- no scorer
- no qrel admission

Primary gate:

- exact dense top-k overlap over the sampled document set

## Objective Matrix

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m362_dense_loss_redesign.py \
  --output-dir /tmp/ii42-m362-dense-loss-redesign \
  --max-docs 4096 \
  --max-queries 128 \
  --teacher-top-k 64 \
  --hard-negative-k 64 \
  --random-negative-k 64 \
  --latent-dims 1024 \
  --active-dims 64 \
  --epochs 20 \
  --batch-rows 16 \
  --device cpu
```

| Variant | Train mode | Exact O@10 | Exact O@20 | Exact O@100 | Candidate O@10 | Candidate NDCG tax | Candidate recall@10 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `recon_baseline` | `hard` | 0.6609 | 0.6283 | 0.5491 | 0.6609 | -0.2584 | 0.6609 |
| `hard_kl_pair` | `hard` | 0.4304 | 0.4522 | 0.4561 | 0.4435 | -0.4845 | 0.4435 |
| `soft_kl_pair` | `soft` | 0.1957 | 0.2391 | 0.3017 | 0.2652 | -0.6880 | 0.2652 |
| `soft_kl_pair_ce` | `soft` | 0.2087 | 0.2391 | 0.2800 | 0.2696 | -0.6953 | 0.2696 |

## Support Sweep

Same setup, but `recon_baseline` only:

| Active dims | Exact O@10 | Exact O@20 | Exact O@100 | Candidate O@10 | Candidate NDCG tax |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | 0.5174 | 0.5239 | 0.4483 | 0.5391 | -0.3901 |
| 32 | 0.5696 | 0.5630 | 0.5130 | 0.5870 | -0.3394 |
| 64 | 0.6609 | 0.6283 | 0.5491 | 0.6609 | -0.2584 |
| 128 | 0.6913 | 0.6761 | 0.6074 | 0.6913 | -0.2458 |

## Interpretation

The result does **not** support the simple claim that reconstruction loss is
inherently wrong. On dense embeddings, hard-TopK reconstruction is currently
the strongest preservation signal tested here.

The result does support a sharper claim:

> The project likely went wrong when representation training was turned into
> posting/ranking/admission optimization without a dense-neighborhood
> preservation gate.

Evidence:

- M320 current token/posting atom route had sparse-cosine Top10 overlap of only
  0.098 on nfcorpus and 0.138 on scifact.
- M362 dense-embedding reconstruction reaches 0.661 Top10 overlap at k64 and
  0.691 at k128 on the same style of exact dense-neighborhood gate.
- Dense-kNN KL/pairwise objectives help less than reconstruction in this setup,
  which means sampled ranking loss can itself distort the global neighborhood.
- Soft sparse training, as implemented here, does not help. It underperforms
  hard TopK reconstruction, likely because the soft train path is too far from
  the hard deployed path.

## New Training Design

The next clean route should be staged:

1. **Stage 0: dense-embedding sparse autoencoder.**
   Train on normalized dense query/doc embeddings with hard TopK in the forward
   path. Primary loss is dense embedding reconstruction or cosine
   reconstruction. Exact dense top-k overlap is the promotion gate.

2. **Stage 0b: neighborhood regularization only after reconstruction works.**
   Add dense-neighborhood KL/pairwise as a small auxiliary, not the main loss,
   and keep exact overlap gates. Reject any objective that improves qrel/candidate
   metrics while lowering exact dense overlap.

3. **Stage 1: text-to-sparse distillation.**
   Once the dense-embedding sparse code is good, train the text/token encoder to
   reproduce that sparse code or its rankings. Do not train text atoms directly
   against BM25/qrel/admission first.

4. **Stage 2: downstream admission/ranking.**
   Only after atom-only dense overlap is materially higher should BM25, scorer,
   and qrel objectives be reintroduced.

## Practical Gate

A useful next checkpoint should report at least:

- exact dense overlap@10/@20/@100;
- qrel metrics as secondary;
- sparse support budget and posting fanout;
- cosine sparse score as the primary representation metric;
- raw-dot sparse score only as a norm-contamination diagnostic.

The current target should not be "beat dense" yet. The target is first to move
atom-only exact dense overlap from M320's ~0.10 level toward M362's 0.65+ level
on the same evaluation surface.
