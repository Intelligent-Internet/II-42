# II-42 M363 Dense-Only Atom Posting Report

## Question

The core concern is that the project may have optimized the wrong objective
from the beginning. If SAE atoms and atom postings are trained to preserve dense
semantics first, then atom-only retrieval should approach dense retrieval without
BM25.

M363 therefore removes BM25, qrels, scorer features, and admission objectives
from the representation test. The only target is dense embedding fidelity.

## Experiment Surface

- Local only; no spark-1/spark-2 jobs were touched.
- Inputs: `/Volumes/Betty/Tmp/psql_bm25s_sae_m80/neutral-stage-a-v0`
- Sample: 4096 docs, 128 queries.
- Dense teacher: exact query-document dense cosine over the sampled corpus.
- Gate: sparse atom score top-k overlap against dense score top-k.
- Primary metric: exact dense top-k overlap, especially O@10.

## Result Summary

### Dense Equivalence Ceiling

If each dense coordinate is treated as a signed atom and all coordinates are
posted, atom posting is exactly dense cosine.

| Baseline | Exact O@10 | Exact O@20 | Exact O@100 |
| --- | ---: | ---: | ---: |
| signed identity atoms | 1.0000 | 1.0000 | 1.0000 |

This is not production-sparse, but it proves the representation form is not the
fundamental blocker. The hard problem is sparse compression while keeping the
dense score surface.

### Dense-Only Reconstruction Scaling

These runs use the M362 hard-TopK reconstruction objective. Evaluation is still
atom-score overlap against exact dense rankings, not reconstruction error.

| Model | Latent dims | Active atoms | Exact O@10 | Exact O@20 | Exact O@100 | Candidate O@10 | NDCG tax |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| recon | 1024 | 16 | 0.5174 | 0.5239 | 0.4483 | 0.5391 | -0.3901 |
| recon | 1024 | 32 | 0.5696 | 0.5630 | 0.5130 | 0.5870 | -0.3394 |
| recon | 1024 | 64 | 0.6609 | 0.6283 | 0.5491 | 0.6609 | -0.2584 |
| recon | 1024 | 128 | 0.6913 | 0.6761 | 0.6074 | 0.6913 | -0.2458 |
| recon | 768 | 256 | 0.7261 | 0.6630 | 0.6352 | 0.7304 | -0.2059 |
| recon | 2048 | 256 | 0.7739 | 0.6804 | 0.6435 | 0.7739 | -0.1695 |
| recon | 2048 | 512 | 0.7391 | 0.6913 | 0.6609 | 0.7391 | -0.1960 |
| recon | 4096 | 256 | 0.7217 | 0.6826 | 0.6417 | 0.7261 | -0.2050 |

This is the first clean evidence in the recent line that dense-only sparse atoms
can move substantially toward dense. It also explains why the M320 token/posting
route looked wrong: M361 measured only about 0.10 to 0.14 sparse-cosine Top10
overlap for the M320 atom route, while the dense-only reconstruction route is
already at 0.77 on the same style of overlap gate.

The 768/256 and 4096/256 follow-ups did not improve over 2048/256. More latent
capacity alone is not the lever, and matching the dense dimension exactly is not
enough either. The next lever is a better dense-only autoencoder objective,
especially score-preserving constraints that keep sparse code dot aligned with
dense dot.

### Direct Sparse Score Matching

M363 also tested direct sparse atom score matching. It trained top-k atom codes
to fit dense query-document scores directly.

| Variant | Mode | Active atoms | Exact O@10 | Exact O@20 | Exact O@100 | Candidate O@10 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| positive cosine MSE/KL | non-negative | 128 | 0.4850 | 0.4600 | 0.5380 | 0.4950 |
| positive cosine + recon | non-negative | 128 | 0.4400 | 0.4475 | 0.4995 | 0.4500 |
| signed cosine MSE/KL | signed | 128 | 0.4550 | 0.4350 | 0.4920 | 0.4700 |
| signed cosine + recon | signed | 128 | 0.4300 | 0.4300 | 0.5130 | 0.4400 |

This direction is currently weaker than reconstruction scaling. The likely
reason is optimization, not representation impossibility: hard top-k score
matching with only 128 sampled query rows gives sparse atoms that overfit local
candidate scores without learning a stable dense basis.

## Interpretation

The original suspicion is substantially correct, but the precise diagnosis is:

- The first-stage representation objective must be dense-only.
- BM25/admission/ranking objectives should not shape the atom basis until dense
  top-k fidelity is already high.
- Direct score matching is not currently the best first-stage loss.
- Hard-TopK reconstruction over normalized dense embeddings is the strongest
  current route and shows a monotonic compression trend up to active-k 256.
- Pure dense-teacher imitation cannot exceed dense. It can only approach or
  equal dense. To exceed dense, a later no-BM25 stage must add qrel/contrastive
  task supervision while preserving the dense-overlap gate.

## Proposed New Route

### Stage 0: Dense-Only Atom Autoencoder

Train query/doc dense embeddings into sparse atom codes with:

- normalized dense embeddings as input;
- shared query/doc encoder;
- hard-TopK sparse code;
- reconstruction/cosine reconstruction as the primary loss;
- exact dense top-k overlap as the promotion gate;
- no BM25, no qrel, no scorer, no admission features.

Promotion target for the next canary:

- Exact O@10 >= 0.85 on the 4096-doc local gate;
- Exact O@100 >= 0.72;
- active atoms <= 256 if possible, <= 512 if needed.

### Stage 1: Text-to-Atom Distillation

Only after Stage 0 passes, train the text encoder to emit the Stage 0 sparse atom
codes or rankings directly.

The gate remains dense-only:

- sparse atom ranking vs exact dense ranking;
- no BM25 blending;
- no scorer rescue.

### Stage 2: No-BM25 Supervised Improvement

Only after dense preservation works, add qrel/contrastive supervision to exceed
the original dense model where possible.

This stage still excludes BM25. It should optimize:

- dense top-k preservation;
- qrel positives above dense false positives;
- hard negatives from dense neighborhoods;
- strict regression guard so qrel gains do not destroy dense fidelity.

## Decision

The current high-value project direction should pivot away from BM25-assisted
rescue and back to dense-only representation learning.

The immediate next experiment should be M364: scale the dense-only atom
autoencoder with a real checkpoint/export path and larger heldout gates, then
only proceed to text-to-atom distillation if dense overlap reaches the promotion
threshold.
