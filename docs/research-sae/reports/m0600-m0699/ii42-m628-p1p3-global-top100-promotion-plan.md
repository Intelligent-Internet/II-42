# M628 / P1.3 Global Top100 Promotion Plan

Status: proposed next objective after M606-M627.

## Current Frozen State

Keep these frozen as the starting point:

- First-stage candidate: `P1.3-a010`
- Atom root:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1/`
- Native benchmark:
  `runs/m608_p1p3_native_shared15_aligned_dense_v1/`
- Scorer-gap root:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1/`
- Dense/reference diagnostics:
  `runs/m608_p1p3_aligned_dense_rankings_shared15_v1/`

Do not promote:

- M605/M605.2 scorer grid,
- M617/M619/M621 admission probes,
- M623-M626 weak/cross teacher lines,
- additional dense-only P1 head micro-tuning.

## Problem Statement

P1.3 solved the main first-stage dense-equivalent score-interface problem.
The remaining bottleneck is top100 promotion:

| Category | P1.3-a010 |
| --- | ---: |
| top100 hit, qrels-weighted | 0.34266 |
| candidate present but under-ranked, qrels-weighted | 0.48835 |
| candidate miss, qrels-weighted | 0.16899 |

M615 shows most under-ranked positives are aligned-dense misses:

| Macro | Under-ranked dense-miss share |
| --- | ---: |
| Micro | 0.980678 |
| Dataset macro | 0.944688 |

Therefore the next breakthrough is not making P1 more dense-like.  The next
stage must learn a global retrieval promotion model over the P1.3 native
candidate pool.

## Design Constraints

- Use P1.3-a010 as the frozen candidate pool.
- Train a global model only; no dataset-specific thresholds, alpha, or
  feature switches.
- Qrels may be used for this second-stage objective, because the problem is
  relevance promotion, not dense-equivalence preservation.
- Do not use dataset id as a model feature.
- Preserve head precision: NDCG@10 and MRR@20 cannot materially regress.
- Final acceptance must come from the native DB/plugin evaluation path.
- Offline JSONL replay is allowed only as a debug/proxy gate.

## Stage Plan

### M628-A / Recovery Dataset V2

Build a native candidate-present training table from P1.3 M604 JSONL exports.

Inputs:

- P1.3 M604 JSONL rows.
- Query and document text from P1.3 atom roots.
- Native BM25/P1/fused ranks and scores.
- Atom-sign and lexical diagnostics from existing M620/M623 code paths.

Rows:

- positives in top100,
- positives below top100,
- top100 blockers,
- sampled tail negatives from rank 101-1000.

Split:

- stable query-level split,
- stratified by dataset only for reporting,
- no dataset id feature.

Required output:

- `runs/m628_p1p3_top100_promotion_dataset_v1/`
- JSONL training rows.
- JSON summary and Markdown audit.

### M628-B / Feature Surface

Start with interpretable, inference-available features:

- `p1_score`, `p1_zscore`, `p1_rank`, `p1_rr`,
- `bm25_score`, `bm25_zscore`, `bm25_rank`, `bm25_rr`,
- `fused_score`, `fused_zscore`, `fused_rank`, `fused_rr`,
- score margins: P1-BM25, fused-P1, fused-BM25,
- query-local z-score/percentile within candidate set,
- lexical coverage and IDF-weighted overlap,
- atom interaction summaries:
  - signed atom conflict rate,
  - shared atom count,
  - positive/negative atom mass,
  - high-IDF atom overlap,
- query/doc length and term coverage.

Audit-only features:

- dense-hit/dense-miss tags,
- current qrels category,
- dataset id.

These audit-only fields must not enter the deployed model.

### M628-C / Model Ladder

Train the smallest global model that can pass the gate.

Order:

1. Calibrated linear/logistic pairwise ranker.
2. Shallow tree model using sklearn only.
3. Optional LambdaMART/GBDT if a stable dependency is already available.
4. Lightweight listwise neural scorer only if the above fail but feature audit
   shows separability.

Do not start with a large neural reranker.  The current failure is not lack of
capacity alone; it is poor same-query separability and head-preservation risk.

### M628-D / Guarded Replay

Replay candidate ordering without touching the native index first.

Policies:

- preserve top-k head, grid `k in {20, 50, 80, 95}`,
- rerank only rank window `<= 200`, `<= 500`, `<= 1000`,
- compare model score blend versus replacement.

Acceptance for proxy replay:

- Recall@100 improves.
- MAP@100 improves or is neutral.
- NDCG@10 regression <= `0.001`.
- MRR@20 regression <= `0.001`.
- Gains are not isolated to one dataset.
- Under-ranked-positive rate decreases on M604 audit categories.

### M628-E / Native Integration

Only after M628-D passes:

- export model artifact as JSON or a small deterministic scorer,
- add an explicit scorer name/version,
- wire it into the native query/evaluation path,
- produce native matrix rows:
  - BM25,
  - dense,
  - P1.3-a010,
  - P1.3-a010+M628.

Required output:

- `docs/research-sae/reports/m0600-m0699/ii42-m628-p1p3-global-top100-promotion-report.md`
- native JSON/Markdown matrix,
- updated scorer-gap audit.

## Expansion Strategy

1. Smoke on 2 datasets:
   - `nfcorpus`,
   - `cqadupstack` or `trec-covid`.
2. Expand to shared15 eval split.
3. Run full shared15 native matrix.
4. Only then consider official1024/native engineering matrix.

Each expansion requires the previous stage to pass the guard.  Do not scale a
model whose gains come from one dataset or from damaging NDCG/MRR.

## Stop Conditions

Stop the M628 family if any of these happen:

- same-query separability remains near random after adding atom/lexical
  features,
- proxy replay improves Recall@100 but damages NDCG@10 or MRR@20,
- native DB/plugin replay loses the offline gain,
- gains are concentrated in a single dataset,
- the model needs dataset-specific thresholds or alpha,
- candidate misses dominate the residual after replay.

## Success Criteria

Promote M628 only if it satisfies all:

- improves macro Recall@100 over P1.3-a010,
- improves or preserves macro MAP@100,
- does not materially regress NDCG@10/MRR@20,
- reduces M604 under-ranked-positive rate,
- works through native DB/plugin evaluation,
- is globally configured and auditable.

## Engineering Notes

Reuse existing code where possible:

- `scripts/build_m616_scorer_recovery_dataset.py` as the dataset builder base,
- `scripts/train_m617_recovery_ranker.py` as a weak baseline and comparison,
- M620 atom interaction code for feature expansion,
- M623 lexical feature code for query/document coverage,
- M604 audit format for replay acceptance.

The new work should extend these pieces rather than create a parallel
evaluation stack.
