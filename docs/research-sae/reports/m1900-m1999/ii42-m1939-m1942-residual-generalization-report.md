# II-42 M1939-M1942 Residual Generalization Report

Date: 2026-07-13

Decision: **retain M1934 `b1.125` as the validated learned-sparse quality
candidate and P1 as the stronger semantic-capacity reference. Do not promote
M1940, M1941, or M1942. M1942 establishes a real monotonic residual mechanism,
but the current local candidate supervision collapses onto universal query
latents and does not generalize across sources. Close loss, step, rank, and
budget sweeps on this surface.**

## Product Question

The mature learned-sparse route is attractive because it offers a small
encoder, exact lexical evidence, semantic postings, and one additive inverted
index. M1934 already proved that a frozen global `b1.125` semantic budget
transfers to three unseen corpora and reproduces through the native PostgreSQL
path. The remaining question was whether training could make this route more
BM25-complementary without paying for another global posting expansion.

M1939-M1942 isolated that question in four stages:

1. verify that lexical residual errors and a stronger teacher signal exist;
2. fine-tune the shared query/document parent against those errors;
3. freeze documents and fine-tune only the query path;
4. freeze the entire parent and train a bounded head that can only add query
   postings.

No BEIR qrels or dataset-specific thresholds were used. The development
surface contains 2,048 train and 512 query-disjoint heldout rows from Fever,
HotpotQA, and NQ. It is a mechanism surface, not a native product benchmark.

## Literature Calibration

The sequence used four established ideas without assuming that any paper had
already solved this exact product problem:

- [CLEAR](https://arxiv.org/abs/2004.13969) motivates residual learning on
  failures of a lexical first stage rather than generic dense imitation.
- [BM25 Query Augmentation](https://arxiv.org/abs/2305.14087) supports a
  transferable query-only semantic augmentation path.
- [Unified LSR](https://arxiv.org/abs/2303.13416) shows that asymmetric query
  and document weighting can preserve effectiveness while reducing query
  work.
- [DF-FLOPS](https://arxiv.org/abs/2505.15070) predicts that average nnz is not
  enough: a few dimensions with corpus-wide activation can dominate inverted
  traversal.

The experiments confirm parts of all four ideas, but also expose the missing
piece: useful residual posting selection must be query-specific and
cross-corpus observable.

## M1939: Residual Capacity Exists

M1939 built one Lucene-style BM25 surface over 40,659 unique candidate
documents and audited the fixed M1914 and PPLX score surfaces.

| Heldout measure | Result |
| --- | ---: |
| BM25 error rows | 133 / 512 |
| M1914 rescue given BM25 error | 0.563910 |
| PPLX rescue given BM25 error | 0.691729 |
| BM25 + M1914 joint errors | 58 / 512 |
| PPLX rescue given joint error | 0.431034 |
| M1914 harm when BM25 is correct | 0.065963 |

All nine observability gates passed. This justified a bounded training canary,
but did not establish that a deployable model could select the residual
movement.

The opportunity is uneven. NQ contributes 103 of the 133 heldout BM25 error
rows and 56 of the 58 BM25+M1914 joint errors. Fever and HotpotQA are already
rescued by M1914 at 94.74% and 90.91%. Source-balanced training and equal-source
macro gates are therefore mandatory.

## Training Results

The common parent baseline on the fixed selected-candidate surface is:

| Rescue | Harm | Pairwise | Positive top1 | Positive MRR |
| ---: | ---: | ---: | ---: | ---: |
| 0.556391 | 0.068602 | 0.853423 | 0.833984 | 0.893348 |

| Route and diagnostic checkpoint | Rescue delta | Harm delta | Pair delta | Top1 delta | MRR delta | Structural result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| M1940 shared q/doc, step 256 | -0.007519 | -0.007916 | +0.001628 | +0.003906 | +0.002555 | Safer and smaller, but loses residual admission |
| M1941 query-only fine-tune, step 64 | +0.000000 | -0.002639 | -0.000163 | +0.001953 | +0.000800 | Freezing documents removes geometry drift, not the admission block |
| M1942 monotonic +8 head, step 768 | **+0.007519** | **+0.000000** | **+0.000279** | **+0.001953** | **+0.001139** | First safe new rescue, below promotion floors |

M1940 and M1941 selected no checkpoint. Their common outcome is important:
ordinary fine-tuning learns conservative score calibration, even when document
postings are frozen. The residual problem is not primarily caused by shared
query/document updates.

## M1942: First Positive Structural Signal

M1942 freezes the complete 30.35M-parameter parent and trains an 810,384
parameter rank-16 head. The head can only add eight non-negative query
postings. It cannot remove or change the parent's top-50 query support, and it
cannot alter document postings.

The run used 1,024 updates with batch size two: 2,048 balanced row exposures,
four times the M1940/M1941 update depth.

| Step | Rescue | Harm | Pairwise | MRR | Query load | Residual maxDF |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 0.556391 | 0.068602 | 0.853423 | 0.893348 | 1.0000x | 0.0000 |
| 128 | 0.556391 | 0.068602 | 0.853423 | 0.893348 | 1.0019x | 0.7012 |
| 256 | 0.556391 | 0.068602 | 0.853423 | 0.893348 | 1.0021x | 0.9922 |
| 512 | **0.563910** | 0.068602 | 0.853539 | 0.894324 | 1.0148x | 0.9980 |
| 768 | **0.563910** | 0.068602 | **0.853702** | 0.894487 | 1.0209x | 0.9434 |
| 1024 | **0.563910** | 0.068602 | 0.853539 | **0.894650** | 1.0429x | 0.9902 |

This is a real mechanism result:

- one additional BM25-error row crosses the boundary;
- no BM25-correct row is harmed;
- pairwise, top1, and MRR improve;
- support cosine remains 0.999834 at the step-768 Pareto point;
- document nnz, maxDF, FLOPS, and postings are exactly unchanged.

It is not a promotion result:

- global rescue gain is `+0.007519`, below the required `+0.010`;
- equal-source macro rescue gain is `+0.003236`, below `+0.005`;
- the entire gain is one NQ row; Fever and HotpotQA do not move;
- no checkpoint passes, so no deployable checkpoint is selected;
- the run has not entered native replay or unseen-corpus evaluation.

## Why More Of The Same Is Not Justified

M1942 separates training depth from structure. Rescue first improves at step
512 and then remains flat through step 1,024, while query load rises from
`1.0148x` to `1.0429x`. Mean residual loss also plateaus around `0.97-0.99`.
Longer training is therefore increasing impact without finding more useful
boundaries.

The decisive failure is usage shape. Each query has exactly eight residual
postings, but residual maxDF reaches `0.99`: almost every query receives at
least one of the same universal latents. This repeats the M1903 lesson at the
query side. Fixed top-k controls nnz, not population-wide activation. The
head learns global priors that can move one dominant NQ boundary, not a
diverse query-conditioned residual vocabulary.

Changing the loss weight, head rank, posting count, or number of steps would
not answer that problem. A stronger DF penalty alone is also insufficient: it
can suppress universal dimensions without proving that query-specific useful
alternatives are observable.

## Relation To P1 And The Current Product Frontier

M1942 does not beat P1. M1930A remains the relevant capacity comparison:

| Parent | Semantic-only @100 | BM25-miss recovery | Union R@100 | Union CUB@1000 |
| --- | ---: | ---: | ---: | ---: |
| P1 | **12.5594%** | **54.8173%** | **0.770815** | **0.890982** |
| M1914 | 9.8328% | 50.0623% | 0.743704 | 0.874436 |

P1 remains the stronger semantic-capacity reference. The learned-sparse route
remains valuable for a different reason: it is a small, mature, directly
indexable encoder with a validated one-index lexical/semantic operating point.
M1934 `b1.125`, not M1942, is its current generalizing candidate:

- all NDCG, MAP, Recall, and MRR macros improved on three unseen corpora;
- exact native SciDocs replay passed;
- the cost is about +6.25% entries, +7.61% posting touches, and +8.75% p95
  latency over `b1`.

The project has therefore improved its scientific understanding, but has not
yet produced a learned-sparse candidate that is globally more complete than
P1.

## Next Justified Route

The next stage should be an audit, not another training recipe:

1. Build native BM25 + M1914 candidate rows across the four M1933 selection
   corpora and the three M1934 unseen corpora.
2. For each lexical residual error, derive positive-versus-hard-negative
   latent utility under the frozen document index.
3. Measure whether useful target latents are visible from inference-time query
   features: frozen query-tail rank, lexical coverage, IDF/fanout, source,
   margin, and candidate context.
4. Report target/harm separability, target-set entropy, per-latent query usage,
   maxDF, and leave-one-corpus-out stability before training.
5. Train a load-balanced or contrastive residual source only if that audit
   demonstrates a qrels-free, cross-corpus observable target. The objective
   must make latent diversity intrinsic rather than add a post-hoc gate.
6. Require native one-index replay before broader promotion.

This route directly addresses the observed failure. If the target latents are
not query-time observable across held-out corpora, the bounded neural residual
head should be closed and M1934 `b1`/`b1.125` retained as the product frontier.

## Artifacts

- `docs/research-sae/reports/m1900-m1999/ii42-m1939-bm25-residual-observability-contract.md`
- `ii42-m1939-bm25-residual-observability.json`
- `docs/research-sae/reports/m1900-m1999/ii42-m1940-clear-residual-canary-contract.md`
- `ii42-m1940-clear-residual-canary.json`
- `docs/research-sae/reports/m1900-m1999/ii42-m1941-asymmetric-query-residual-contract.md`
- `ii42-m1941-asymmetric-query-residual.json`
- `docs/research-sae/reports/m1900-m1999/ii42-m1942-monotonic-query-expansion-contract.md`
- `ii42-m1942-monotonic-query-expansion.json`
- ClearML M1940 task `d54a609ab7c246aca2a9b523c60d5043`
- ClearML M1941 task `5c6ac478d734475d9cf71e87e7ca7055`
- ClearML M1942 task `0fd8a7c63f22415bb272a0fb0b2ae7f9`
