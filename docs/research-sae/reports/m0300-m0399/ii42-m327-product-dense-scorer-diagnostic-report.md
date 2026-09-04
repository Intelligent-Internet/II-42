# M327 Product-Dense Scorer Diagnostic Report

## Status

M327 checked whether the product VectorChord dense baseline should be used as a
teacher/control for the M322 candidate-pool scorer, and whether an explicit
BM25-supported positive rescue constraint can fix top-rank failures.

The answer is negative for both as a mainline promotion:

- Product dense teacher is slightly better than exact dense teacher on top-rank
  metrics, but the gain is small and does not close the dense gap.
- BM25-positive rescue increases admission pressure but does not improve
  MRR/NDCG/MAP. It should remain a diagnostic-only option for now.

## Compared Runs

All runs use the same 5-dataset cached surface:

- `nfcorpus`
- `scifact`
- `fiqa`
- `arguana`
- `scidocs`

Common surface:

- Checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m326-expansion5-candidate-cache-v1`
- Candidate config:
  `candidate_k=160`, `doc_post_active_k=96`, `query_post_active_k=80`,
  `max_length=256`, `max_atom_df_ratio=0.25`

Run outputs:

- Exact dense teacher diagnostic:
  `/home/huoju/leask/runs/ii42-m327-exact-teacher-diagnostic-v1/m327_exact_teacher_diag_tw015_fpw030_seed1050.json`
- Product VectorChord dense teacher diagnostic:
  `/home/huoju/leask/runs/ii42-m327-product-dense-scorer-v1/m327_product_dense_teacher_tw015_fpw030_seed1050.json`
- BM25-positive rescue diagnostic:
  `/home/huoju/leask/runs/ii42-m327-bm25-rescue-v1/exact_rescue025.json`

## Aggregate Metrics

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Exact dense teacher scorer | 0.7258 | 0.3616 | 0.3312 | 0.2408 |
| Product dense teacher scorer | 0.7236 | 0.3654 | 0.3339 | 0.2424 |
| Exact teacher + BM25 rescue 0.25 | 0.7271 | 0.3595 | 0.3300 | 0.2399 |

The product teacher improves MRR/NDCG/MAP slightly, but loses Recall. The
BM25 rescue run raises Recall slightly, but hurts all top-rank metrics.

## Failure Diagnostics

| Run | BM25-supported qrel suppressed | Qrel admitted but low-ranked | Semantic false positive over-ranked | Teacher qrel top10 lost |
| --- | ---: | ---: | ---: | ---: |
| Exact dense teacher scorer | 339 | 160 | 277 | 165 |
| Product dense teacher scorer | 340 | 161 | 273 | 158 |
| Exact teacher + BM25 rescue 0.25 | 341 | 165 | 272 | 158 |

The diagnostic categories barely move. This is the important result: the issue
is not a single missing pairwise constraint. The scorer is still learning a
score surface that cannot reliably separate:

- BM25-supported relevant documents;
- semantic-only false positives;
- dense-near but unjudged documents;
- qrel positives that are present in the candidate pool but low-ranked.

## Engineering Changes

The scorer script now supports:

- cache-only candidate-surface evaluation without loading the HF model or SAE;
- optional heldout query diagnostics;
- an experimental BM25-positive rescue loss, disabled by default.

The cache-only path matters because scorer-loss sweeps should not invalidate or
rebuild frozen candidate surfaces.

## Decision

Do not promote product dense teacher as the main training target. It is useful
as a product baseline/control, but the exact dense teacher should remain the
oracle label surface.

Do not promote BM25-positive rescue loss. It is a useful diagnostic knob, but
the first canary shows it trades top-rank quality for admission pressure.

## Next Direction

The next useful step is not another scalar loss-weight sweep. It should be a
stronger admission/ranking objective that directly models the final decision:

1. Train on query-level win/loss examples from the final top-k boundary.
2. Use runtime-safe features only: BM25 score/rank, atom score/rank, overlap,
   source class, DF/fanout summaries, and query/document atom statistics.
3. Treat exact dense as an oracle label source and VectorChord dense as a
   product baseline, not as interchangeable teachers.
4. Preserve the candidate cache and add richer query diagnostics before broader
   runs.

The practical M328 candidate is a boundary-aware scorer: learn to decide which
candidate should enter the final top-10/top-20/top-100, instead of only shaping
pairwise qrel-vs-negative margins inside the candidate pool.
