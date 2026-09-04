# ii42 M326 Dense-Teacher Admission Scorer Plan

## Summary

M326 continues the M310-M325 posting-level line, but changes the training
signal. M323 proved that a learned candidate-pool scorer can beat static
BM25+atom fusion. M324 and M325 showed that larger candidate pools and
source-balanced qrel hard negatives are not enough. The next blocker is that
the scorer sees runtime-safe BM25/atom features, but it is trained mostly
against sparse qrel positives rather than a dense ranking target.

M326 therefore keeps the same runtime evidence family and adds dense-teacher
and admission-impact supervision. The goal is not to train a new encoder and
not to expand top-k again. The goal is to learn when BM25 evidence, atom
evidence, and their conflict patterns should admit or suppress a document.

## Current Evidence

Best current learned scorer baseline:

| Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M323 `df<=0.25` best | 0.6642 | 0.5485 | 0.4536 | 0.3663 |
| M323 `df<=0.5` best | 0.6605 | 0.5517 | 0.4557 | 0.3614 |
| Exact dense same split | 0.7433 | 0.6152 | 0.5330 | 0.4464 |

Negative follow-ups:

| Run | Change | Result |
| --- | --- | --- |
| M324 | `candidate_k=300`, bigger scorer | Higher upper bound, worse scorer. |
| M325 | source-balanced qrel hard negatives | Slight M324 recovery, still below M323. |

Interpretation: more candidates expose positives, but current qrel-only
training cannot reliably rank them. The missing signal is dense/BM25+dense
teacher ordering and explicit admission-impact labels.

## Non-Goals

- Do not restart Stage A or train a new SAE encoder in M326.
- Do not promote a product SQL/API contract from this experiment.
- Do not use VectorChord approximate dense results as teacher labels.
- Do not increase `candidate_k` again unless the report proves a scorer-side
  gain at `candidate_k=160`.
- Do not tune dataset-specific profiles. Any adaptive behavior must use
  runtime-safe query and candidate features, not dataset id.

## Fixed Inputs

- Encoder/checkpoint: current M320/M323 evidence surface.
- Primary canary datasets: `nfcorpus`, `scifact`, `fiqa`.
- Primary scorer baseline: M323 `candidate_k=160`.
- Diagnostic high-capacity surface: M324/M325 `candidate_k=300`.
- Dense reference: exact PostgreSQL scan with index scans disabled.
- Split hygiene: same seed-1050 heldout split used by M323.

## Step 0: Freeze Baselines

Before adding training logic, write a small baseline manifest with:

- M323 best JSON path and metrics.
- M324/M325 JSON paths and metrics.
- exact dense same-split JSON path and metrics.
- surface cache path and config hash.
- checkpoint path, dataset list, split seed, and DF thresholds.

Acceptance:

- manifest reproduces the M323 aggregate values in this plan;
- no new training starts until the manifest is complete.

## Step 1: Export Teacher Ranking Targets

Build exact teacher rankings for the same split:

1. Dense exact top-k.
2. BM25+dense teacher top-k using the current mainline fusion formula.
3. Optional BM25-only and atom-only top-k for impact diagnostics.

Implementation requirements:

- Use exact PostgreSQL scan for dense labels.
- Disable approximate vector index paths during teacher export.
- Store query-level ranking arrays by stable doc id.
- Record missing qrel positives separately; do not force heldout positives into
  the candidate pool.

Output:

- `m326_teacher_rankings_seed1050.json`.
- `m326_teacher_rankings_seed1050.summary.json`.

## Step 2: Add Teacher-Supervised Scorer Loss

Extend the candidate-pool scorer to support teacher-ranking supervision.

Training labels per query:

- qrel positives as hard relevance constraints;
- dense teacher soft distribution inside the candidate pool;
- BM25+dense teacher soft distribution inside the candidate pool;
- explicit admission-impact buckets:
  - dense-hit / BM25-miss;
  - BM25-hit / dense-miss;
  - BM25 false positive;
  - atom-only semantic hit;
  - BM25+atom agreement;
  - high-DF atom noise.

Loss shape:

- listwise KL or cross-entropy to teacher distribution;
- qrel pairwise/listwise hard constraint;
- residual no-harm loss against M323 baseline score;
- source-balanced negative sampling retained only as a sampler, not as the main
  objective;
- fanout and DF-band values used as features, not hard global pruning rules.

Acceptance:

- scorer can train with `teacher_weight=0` and exactly reproduce M323 behavior;
- enabling teacher loss changes only the scorer objective, not candidate
  generation.

## Step 3: M326 Canary

Run the first canary on `nfcorpus`, `scifact`, and `fiqa`.

Primary config:

- `candidate_k=160`;
- `hidden_dim=64`;
- `residual_alpha=0.30`;
- `epochs=80`;
- `teacher=BM25+dense`;
- `teacher_weight=0.25`;
- `qrel_weight=1.0`;
- `no_harm_weight=0.10`.

One controlled retry is allowed:

- keep all inputs unchanged;
- sweep only `teacher_weight` across `0.10`, `0.25`, `0.50`;
- stop if MRR/NDCG/MAP do not move toward dense.

Promotion gate:

- beats M323 best on MRR@20, NDCG@10, and MAP@100;
- does not reduce Recall@100 by more than `0.005`;
- narrows the same-split exact dense gap;
- does not increase average candidate docs or postings versus M323.

## Step 4: Harder Canary

Only after Step 3 passes, expand to:

- `scidocs`;
- one large QA dataset, preferably `nq` or `hotpotqa`;
- one difficult entity or duplicate dataset if cached surfaces are available.

The harder canary must report per-dataset deltas, not just aggregate means.

## Step 5: Decision

If M326 passes:

- promote dense-teacher scorer supervision as the next mainline;
- prepare a full15 run with strict split and exact dense references;
- then consider compressing the scorer or moving it closer to native runtime.

If M326 fails:

- stop the current scorer feature family;
- do not keep tuning topK, scalar weights, or source balance;
- next route should add richer posting-level features or split admission and
  rank into two separate models.

## Reporting Requirements

Every M326 report must include:

- candidate upper-bound metrics;
- final scorer metrics;
- same-split exact dense metrics;
- M323 best baseline;
- candidate source ratios;
- fanout/postings/candidate-doc cost;
- per-bucket admission impact;
- strict note if any qrel positives are absent from corpus.

## Initial Execution Checklist

1. Create M326 baseline manifest.
2. Export exact dense and BM25+dense teacher rankings.
3. Add teacher-ranking loader and loss flags to the scorer.
4. Run `teacher_weight=0` parity check against M323.
5. Run the primary M326 canary.
6. Update the M320/M326 report with pass/fail and next decision.
