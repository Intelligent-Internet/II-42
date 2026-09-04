# M1600-M1600A Retrieval-Native Balanced Discrete Final Report

## Executive Decision

**No-go for M1600A as the next pure single-encoder, single-posting-index
product route.** M1600 found a real source-capacity result, but its deployable
query router captured too little of that capacity and became worse with
training depth. The predeclared full-scale gate failed, so the program stopped
before qrels, official rows, or dataset-specific adaptation.

This does not show that unified posting is impossible. It shows that a frozen
balanced product-code source plus a static low-rank query-key router is not the
missing mechanism.

## Why M1600 Was Different

M1560-M1572 closed post-hoc frozen geometry routes, exact-term compression,
product cells, and overlapping hashes. M1600 tested a narrower literature-
supported hypothesis:

- jointly expose a balanced discrete posting geometry rather than convert a
  finished dense space;
- train against dense retrieval order rather than reconstruction;
- control document frequency directly rather than relying on generic
  sparsity;
- evaluate exact posting unions at hard fixed read budgets.

The design follows the retrieval-order finding in Distill-VQ
(`arXiv:2204.00185`), the explicit DF control in DF-FLOPS
(`arXiv:2505.15070`), and joint discrete-index lessons from
`arXiv:2105.03933` and RepCONC (`arXiv:2110.05789`).

## Evidence Chain

| Stage | Surface | Key result | Decision |
| --- | --- | --- | --- |
| S0 | 128/64 integration | exact cache, training, replay, and gate path worked | proceed |
| S1A | 4,000/500, 8x128 | training reduced O@100 0.519079 to 0.480993 | reject free joint training |
| S1B | 4,000/500, 8x512 | oracle 1.000000/0.967211 at 0.145435x | source capacity passes |
| S1C seed 1602 | 4,000/500 | +0.003080 O@100, +0.000794 O@256 | small signal |
| S1C seed 1603 | 4,000/500 | +0.002520 O@100, +0.001156 O@256 | signal replicates |
| S2 source | 10,000/1,000 | oracle 1.000000/0.992906 at 0.130539x | capacity scales |
| S2 router | 10,000/1,000 | +0.001890 O@100, +0.000945 O@256 | stop |

All values before the stop are qrels-free dense-neighborhood overlap metrics.
No BEIR relevance label selected a checkpoint, policy, source, or budget.

## Comparison With The Existing Frontier

| Source | O@100 | O@256 | Reads |
| --- | ---: | ---: | ---: |
| M1565 route2048 | 0.906235 | 0.842683 | 0.045015x |
| M1600 full, one doc key, g8/p8 | 0.831010 | 0.704906 | 0.146086x |
| M1600 full, two doc keys, g8/p8 | 0.919300 | 0.837445 | 0.299274x |
| M1600 full one-key oracle | 1.000000 | 0.992906 | 0.130539x |
| M1600 full trained router | 0.824180 | 0.708773 | 0.149978x |

The M1600 oracle is strong, but the deterministic and trained policies are not
quality-cost competitive with M1565. At almost 0.30x reads, two-key M1600
beats M1565 O@100 but still misses its O@256 while reading over six times more
postings. An oracle is therefore not a product breakthrough.

## What Was Learned

1. **Balanced posting capacity is real.** A 4,096-key unified namespace with
   max DF below 1% can contain almost all dense top256 at a 0.15x read cap.
2. **Free joint training is unsafe.** Better batch/listwise objectives and
   somewhat flatter key usage did not create useful hard collisions; the
   shared geometry moved while heldout overlap fell.
3. **Query routing is the bottleneck, but not a shallow one.** The useful key
   action depends on posting DF, already-selected unions, and the exact dense
   neighborhood. Nearest-code logits do not contain that ordering.
4. **The early train signal is real but not scalable.** It replicated across
   seeds, yet 2.5x more query rows reduced rather than amplified the gain.
5. **Training depth is contraindicated.** Every run peaked at step 200 and
   then regressed sharply while optimization loss continued to improve.

## Product Verdict

Do not promote M1600A, attach it to the text encoder, or add BM25/reranking to
hide the failed first-stage gate. Do not run official FiQA or shared rows for
this checkpoint: it already fails the qrels-free source-policy requirement.

Retain the exact evaluator, cache validation, balanced codebook audit, and
source-oracle tooling. They are useful falsification surfaces for future
mechanisms.

Any new pure-posting route must change the causal structure rather than tune
M1600A. In particular, it must make document/query posting compatibility
locally observable at encoding time, or construct source assignments whose
nearest query keys are useful without a corpus-context greedy oracle. A larger
router over the same target is explicitly not authorized.

## Reproducibility

- Branch baseline: `research/m1560-dense-neighborhood-posting` at `c701d20e`.
- Spark host: `spark-1`; container `nvcr.io/nvidia/pytorch:26.03-py3`.
- Run root:
  `/home/huoju/leask/runs/ii42-m1600-balanced-discrete-v1`.
- Full data: 10,000/1,000 disjoint MS MARCO rows, 88,992/8,988 unique docs.
- Full router ClearML task: `cef67d5599534f3a898f559333c8edfc`.
- Seed replication ClearML tasks:
  `a76f8a4060d1497e9cc36154a34f9ece` and
  `a05f9e3fe77f4f2abf018ab77cea5d1f`.
- Artifact hashes are listed in the S2 full-scale report.
