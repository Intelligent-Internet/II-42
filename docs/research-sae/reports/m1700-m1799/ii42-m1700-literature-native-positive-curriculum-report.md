# M1700 Literature-Native Positive Curriculum Report

## Status

M1700 replaces the closed M1691 no-positive KD branch with a published sparse
retrieval curriculum: explicit positives, a large effective in-batch negative
pool, curated hard negatives, and a sparsity constraint. It retains the M1660
product shape: one vocabulary-sparse encoder, one posting map, and one exact
BMP/native inverted index.

Current decision: **retain step 4 as a positive research result, but do not
promote it**. It improves every shared3 and full-FiQA quality metric, and the
formal Linux u32 BMP rerun now has exact `648/648` boundary parity for both root
and candidate. The candidate still fails product cost: full-FiQA document
maximum DF is `1.183x`, decoded postings are `1.367x`, and same-machine BMP
p50/mean latency are `1.174x/1.158x` the root.

## Why The Route Changed

M1691 showed a repeatable Pareto conflict. At step 1,000, both stored-score and
ensemble KD improved KL/top1/pairwise/Spearman, but document maximum DF reached
`1.469x` and `1.339x`. The cost-safe ensemble checkpoint did not preserve all
ordering gates. Pure K=8 score-spectrum KD therefore stopped; LR, temperature,
FLOPS-lambda, pruning, and threshold variants were not authorized.

This does not reject learned sparse retrieval. It rejects a supervision surface
with no explicit relevance-positive example.

## Literature And Implementation Basis

| Evidence | M1700 consequence |
| --- | --- |
| [SPLADE hard-negative study](https://arxiv.org/abs/2205.04733) | Treat hard-negative sampling and distillation as additive training-pipeline components, not architecture tweaks. |
| [GradCache](https://arxiv.org/abs/2101.06983) | Use representation-gradient caching to reproduce the exact large-batch contrastive gradient; ordinary accumulation is not equivalent. |
| [LACONIC](https://arxiv.org/abs/2601.01684) | Use a two-phase positive-pair then RLHN curriculum, 64/192 weak-pair lengths, effective negative pool 2,048, and 1+15 hard-negative rows. |
| [RLHN](https://arxiv.org/abs/2505.16967) | Use relabeled hard negatives and keep only `msmarco_passage` for the initial seen-domain stage; report strict heldout rows separately. |
| [NV-Retriever](https://arxiv.org/abs/2407.15831) | Audit negatives near the positive score because harder mining can reintroduce false negatives. |
| [Conventional Contrastive Learning Often Falls Short](https://arxiv.org/abs/2505.19274) | Treat positive-only InfoNCE as a canary, not an unlimited training objective for an already strong root; require a stronger listwise teacher before hard-negative training. |
| [Scaling Sparse and Dense Retrieval](https://arxiv.org/abs/2502.15526) | Prefer combined contrastive and knowledge-distillation supervision for sparse adaptation over either signal in isolation. |
| [SPLADE-v3](https://arxiv.org/abs/2403.06789) | Use multiple hard negatives and cross-encoder score distributions as the mature sparse-training control, while retaining broad out-of-domain evaluation. |
| [DF-FLOPS](https://arxiv.org/abs/2505.15070) | Reserve DF-aware regularization for a measured high-DF failure after relevance quality passes. |
| [BMP](https://arxiv.org/abs/2405.01117) | Require exact native-index replay before promotion. |

The pinned LACONIC repository contains a GradCache training path, and the
pinned OpenSearch sparse repository supplies the InfoNCE document-FLOPS
schedule used by the current root. M1700 composes these mechanisms; it does not
claim that an author published this exact root/data/engine combination.

## Data And Leakage Audit

| Artifact | Revision / SHA | Rows | Result |
| --- | --- | ---: | --- |
| Nomic weak pairs | `dfea387946f33796beee9b71caef61e091cfa2bf` | 9,259,451 | 46/46 LFS objects verified |
| RLHN | `d5323fad7aea636e633be28722ad15aafa6cbd54` | 648,766 | 11/11 LFS objects verified |
| Broad train | `17da77462f4fc65cc73baa8b66495c56e17d095c85dab205c50bea6077e62d00` | 102,400 | unique, all 46 shards |
| Broad validation | `1f98484284370bd104ac6cf6416b8157e7b39ad2c99c1a6e09bfda3554b75821` | 10,000 | disjoint from train/shared15 |
| RLHN 1+15 | `660d102a986fc1f39ecaf772e1a3867ad078b2b0f17963d4ea613b2451127cd0` | 4,096 | exact 15 cleaned negatives |

The broad scan covered all 9,259,451 rows, removed 135,186 duplicate query
hashes, and found zero exact shared15 query/positive-text overlap. The RLHN scan
covered all 648,766 rows and 455,748 `msmarco_passage` rows; one shared15 query
overlap was excluded. The 10,000-row validation SHA is unchanged between the
100,000 and 102,400 training manifests.

## Frozen-Root Signal Audit

| Surface | Queries | InfoNCE | Positive top1 | MRR | Margin p10 |
| --- | ---: | ---: | ---: | ---: | ---: |
| broad pool 128 | 512 | 0.006579 | 0.998047 | 0.999023 | 19.049044 |
| broad pool 512 | 2,048 | 0.014510 | 0.996094 | 0.997382 | 15.998604 |
| broad pool 2,048 | 8,192 | 0.015496 | 0.996216 | 0.997579 | 13.936862 |
| RLHN 1+15 | 1,024 | 1.666256 | 0.743164 | 0.822418 | -6.797162 |

RLHN pairwise accuracy is `0.934440`. A negative reaches at least 95% of the
positive root score on `33.50%` of rows. The broad batch-128 memory example was
saturated, but a 2,048 pool exposes a small, nonzero error surface. RLHN is the
stronger fine-grained relevance signal.

## Exact-GradCache Mechanism Canary

The fixed four-update canary used an effective batch of 2,048 and microbatch of
32. Peak reserved memory was `5.85%`; every loss and gradient was finite.

| Update | Train InfoNCE | Train top1 | Gradient norm |
| ---: | ---: | ---: | ---: |
| 1 | 0.025680 | 0.994141 | 0.458691 |
| 2 | 0.017499 | 0.996094 | 0.418126 |
| 3 | 0.010605 | 0.997070 | 0.355741 |
| 4 | 0.010120 | 0.998047 | 0.232428 |

On the unchanged validation surface, step 4 changed broad-2,048 InfoNCE from
`0.015496` to `0.011344`, top1 from `0.996216` to `0.997925`, and MRR from
`0.997579` to `0.998549`. RLHN pairwise accuracy changed from `0.934440` to
`0.935026`; RLHN top1/MRR changed by only `-0.001953/-0.001193`. The predeclared
joint gate therefore authorized one fixed 50-update broad run.

## Fixed 50-Update Broad Run

The formal short run restarted from the original root and used 102,400 unique
weak pairs, exact GradCache batches of 2,048, and the pinned OpenSearch
document-FLOPS schedule. Every update was finite. Peak reserved memory was
`7.00%`. Train document FLOPS decreased from `2.620121` at update 1 to
`1.107571` at update 50.

| Checkpoint | Broad InfoNCE | Broad top1 | Broad MRR | RLHN top1 | RLHN MRR | RLHN pairwise |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| root | 0.015496 | 0.996216 | 0.997579 | 0.743164 | 0.822418 | 0.934440 |
| 10 | 0.009310 | 0.998291 | 0.998839 | 0.738281 | 0.820075 | 0.935547 |
| 25 | 0.006938 | 0.998779 | 0.999178 | 0.736328 | 0.818584 | 0.935091 |
| 50 | 0.006447 | 0.998657 | 0.999139 | 0.734375 | 0.817021 | 0.934310 |

All fixed checkpoints pass the coarse broad and RLHN floors. Step 50 was the
loss-only selection because it had the lowest broad validation InfoNCE. The
complete ordering/cost audit below rejects that selection and demonstrates why
training loss cannot choose this root's checkpoint.

## Training-Depth And Ordering/Cost Gate

The unchanged M1691 `2,048 x 100` full-candidate surface was evaluated at the
four-update canary and at steps 10, 25, and 50. Deltas are candidate minus the
unmodified root. The maximum cost ratio is the worst predeclared sparse-cost
ratio across query and document surfaces.

| Step | KL delta | Top1 delta | Pairwise delta | Spearman delta | Max cost ratio | Gate |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 4 | -0.009537 | -0.004883 | +0.001458 | +0.003528 | 1.090341 | pass |
| 10 | +0.022123 | -0.008789 | +0.003204 | +0.007525 | 1.200319 | fail |
| 25 | +0.074878 | -0.022949 | -0.000886 | -0.000506 | 1.257485 | fail |
| 50 | +0.091493 | -0.030762 | -0.006214 | -0.011947 | 1.222536 | fail |

This is a depth curve, not a noisy checkpoint comparison. Positive-only
InfoNCE has a narrow useful window, after which broad positive loss keeps
falling while complete candidate ordering and posting cost worsen. M1700
therefore forbids scaling this objective to one million pairs or continuing it
to an RLHN InfoNCE stage.

## Native Shared3

Step 4 was replayed through the exact native block engine against the frozen
OpenSearch sparse root. All 648/648 engine comparisons on the three locked rows
had exact parity.

| Dataset | NDCG@10 delta | MAP@100 delta | Recall@100 delta | MRR@20 delta | CUB delta | Document maxDF ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| NFCorpus | +0.001226 | +0.001250 | +0.002085 | +0.003350 | +0.003807 | 1.076543 |
| SciFact | +0.002676 | +0.004401 | +0.000000 | +0.002966 | +0.000000 | 0.979784 |
| FiQA | +0.009991 | +0.004364 | +0.000000 | +0.016396 | +0.005000 | 1.157520 |

This is a real native quality signal: every row is non-negative on all five
quality fields. It is not a promotion because the FiQA shared slice already
places document maxDF slightly above the `1.15x` cost floor.

## Full-FiQA Native Replay

The full 57,638-document, 648-query FiQA replay confirms that the shared-slice
gain was not a sampling artifact.

| Metric | Root | Step 4 | Delta |
| --- | ---: | ---: | ---: |
| NDCG@10 | 0.370244 | 0.371721 | +0.001477 |
| MAP@100 | 0.310860 | 0.313862 | +0.003002 |
| Recall@100 | 0.656042 | 0.657223 | +0.001181 |
| MRR@20 | 0.456006 | 0.459958 | +0.003952 |
| Candidate upper bound | 0.849637 | 0.856234 | +0.006597 |

Exact block parity is `1.0`. The representation and metadata gates pass, but
the traversal-cost gate does not:

| Cost field | Root | Step 4 | Ratio |
| --- | ---: | ---: | ---: |
| Document mean nnz | 228.757 | 255.207 | 1.115626 |
| Document p95 nnz | 306 | 344 | 1.124183 |
| Document maxDF | 0.487682 | 0.576928 | 1.183002 |
| Decoded postings/query | 437,484 | 597,950 | 1.366793 |
| Bitset word ops/query | 93,556 | 105,599 | 1.128728 |
| Metadata bytes | 92,141,088 | 95,640,944 | 1.037984 |

The candidate's measured end-to-end block replay happened to be faster in this
run (`413.7 s` versus `470.4 s`), so proxy cost and wall time disagree. That
disagreement authorizes an official BMP measurement; it does not waive the
cost floor.

## Official BMP Exactness Boundary

The first official BMP run returned exact scores for every returned document
but had strict top-100 parity on only `647/648` queries. Query `1826` exposed
the cause: its true maximum integer dot score is `65,806`, while upstream BMP
accumulates block scores and upper bounds in `u16` with maximum `65,535`. The
top document wrapped below the boundary and was pruned.

The engine patch keeps `u8` stored impacts and the original quantization, but
accumulates scores, range upper bounds, and the top-k heap in `u32`. Two new
Rust regressions exercise score and upper-bound accumulation at `73,440`; all
three release tests pass. The patched Linux/aarch64 wheel was then used for a
sequential root/candidate rerun on the same idle spark-2 machine.

| Surface | Strict parity | Max integer score | Index bytes | BMP p50 | BMP p95 | BMP p99 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Root | 1.000000 | 63,230 | 208,532,074 | 14.740 ms | 20.303 ms | 21.481 ms |
| Step 4 | 1.000000 | 65,806 | 223,786,938 | 17.311 ms | 22.174 ms | 23.307 ms |
| Ratio | - | - | 1.073154 | 1.174474 | 1.092147 | 1.085036 |

Both runs passed exhaustive quantized parity. In particular, query `1826` now
returns the correct boundary despite its `65,806` score exceeding `u16`.
Official BMP candidate-minus-root quality deltas were all positive:

| NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Candidate upper bound |
| ---: | ---: | ---: | ---: | ---: |
| +0.000212 | +0.001821 | +0.003071 | +0.002777 | +0.002029 |

## Conclusion

M1700 establishes two durable results. First, a very short positive curriculum
can improve full native retrieval quality, while deeper positive-only training
predictably damages ordering and sparse cost. Second, standard BMP can execute
this learned-sparse surface exactly once its accumulators are widened to `u32`.

It does not establish a product default. The candidate violates the locked
`1.15x` cost policy through document maxDF, decoded postings, and BMP p50
latency. The unmodified OpenSearch root plus patched M1660 BMP therefore remains
the deployable baseline. Do not deepen positive-only InfoNCE or tune the cost
gate after observing these results.
