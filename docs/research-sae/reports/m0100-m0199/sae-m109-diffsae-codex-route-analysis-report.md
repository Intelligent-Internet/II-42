# SAE M109 DiffSAE-Codex Route Analysis Report

Date: 2026-05-23

## Decision

`diffsae-codex` is a useful reference because its training objective, sparse
representation, and serving index use the same scoring contract. Its route is
not simply "smaller and simpler"; it is narrower and better aligned:

1. Train sparse features against the exact candidate-set retrieval rule.
2. Harden the same sparse vectors into an inverted index.
3. Evaluate the same sparse-dot scoring rule that will be served.

Our current line became complex because Stage A, Stage B, BM25/SAE hybrid
ranking, support compression, candidate generation, and runtime residual
calibration were optimized as separate layers. That creates many opportunities
for a training target to improve one surface while not improving the final
runtime retrieval contract.

The next useful reset is therefore not another scalar fine-tune. It is to build
a clean candidate bank from our data and run a DiffSAE-style end-to-end sparse
retrieval objective where the training loss and runtime sparse index are the
same object. BM25 should be introduced as candidate source and complementarity
signal after the sparse retrieval objective is stable, not as another late
ranker patch from the start.

## What Changed In DiffSAE-Codex

The inspected `diffsae-codex` checkout is clean on `main`. Recent commits show
three relevant directions:

- `Add query-only distillation data support`
- `Document asymmetric query encoder results`
- `Add unlabeled query distillation option`
- `Add BEIR sparse hard-negative refresh`
- `Port public retrieval workflows and production index export`
- `Add production native sparse search backend`

The important implementation pieces are:

- `AdaptiveTopKSAE`: learns sparse features over dense embeddings and exports
  hard TopK sparse activations.
- `DifferentiableSparseRetriever`: scores query and document sparse features
  with the same sparse dot rule used by the index.
- `retrieval_training_step`: combines recall/top-k loss, cross entropy,
  multi-positive CE, teacher KL, reconstruction, and k-budget pressure.
- `SparseInvertedIndex` and the native C++ backend: serve the same sparse dot
  representation instead of adding a separate fusion layer.
- `train_asymmetric_query_encoder.py`: freezes the document sparse index and
  trains only a query encoder, isolating the online encoder problem.

Local tests in `diffsae-codex` passed:

```text
python3 -m pytest -q tests/test_topk.py tests/test_retriever.py \
    tests/test_index.py tests/test_asymmetric.py
10 passed
```

The same test subset also passed on Spark after syncing the repo.

## Why This Route Works

### 1. It Optimizes The Actual Retrieval Contract

The core loss is not just atom imitation. It directly asks whether the sparse
query/document features retrieve the relevant document under the same sparse
dot rule that the serving index will use.

That matters because a good latent reconstruction or support-overlap metric can
still fail at search. We have seen this repeatedly: representation preservation
can pass while ranking quality or runtime candidate behavior remains blocked.

### 2. It Keeps The Candidate Geometry Fixed

The public workflow constructs fixed candidate rows with positives, dense hard
negatives, random negatives, and optional sparse hard negatives. That gives the
retrieval loss a stable shape.

Our current materialized surfaces are messier:

- Some query/document rows have missing or empty embeddings.
- Candidate row lengths are variable.
- Some rows are generated for candidate-surface evaluation, others for
  postings/runtime evaluation.
- Stage-B ranking is often trained after candidate generation, not jointly with
  the sparse retrieval representation.

This makes simple training look less effective because it is not always
optimizing the same object that later gets evaluated.

### 3. It Trains One Blocker At A Time

The asymmetric path in `diffsae-codex` freezes the document SAE/index and only
trains the query tower. That is much easier to diagnose than changing query
atoms, document atoms, candidate generation, BM25 weight, and final ranker at
the same time.

Our M29-M30 query-side work had the right instinct, but it still lived inside a
larger hybrid and residual-ranker stack. DiffSAE's query-only route is cleaner:
the document sparse space is fixed, the target is sparse retrieval quality, and
the output is directly usable by the index.

### 4. The Strong Results Use Strong Teachers

The best documented `diffsae-codex` public results are not all from the small
Snowflake-M setting. They improve substantially with stronger embedding
backbones:

| Backbone / SAE | Sparse NDCG@10 | Sparse Recall | Sparse MRR |
| --- | ---: | ---: | ---: |
| Snowflake M v2 / 4096 | 0.378 | 0.479 | 0.400 |
| Qwen3 0.6B / 8192 | 0.422 | 0.511 | 0.448 |
| Stella 1.5B / 8192 | 0.483 | 0.588 | 0.502 |
| Stella 1.5B / 16384 k=96 | 0.519 | 0.635 | 0.536 |
| Stella 16k sparse-HN continuation | 0.525 | 0.639 | 0.539 |

This means part of the gap is probably teacher/model capacity, not only the
SAE training method.

### 5. Some Reported Results Are Not Clean Held-Out Proof

The asymmetric query encoder result with unlabeled benchmark-query distillation
is useful engineering evidence, but it is not a clean product-quality proof
because it uses benchmark-query distillation. This is relevant to the
overfitting concern: the route can be valid while a particular metric is still
transductive or benchmark-shaped.

## Bridge Smoke On Our Data

I added a bridge script:

```text
scripts/research_sae_diffsae_codex_bridge_smoke.py
```

It reads our materialized `candidate_rows.jsonl`, `documents.jsonl`, and
`queries.jsonl`, filters to rows with available embeddings and fixed candidate
length, then trains the `diffsae-codex` SAE/retriever directly on those rows.

Spark command used:

```bash
ssh huoju@100.123.2.95 'rm -rf /home/huoju/leask/runs/diffsae-codex-bridge-smoke-small && cd /home/huoju/leask/ii42_sae && docker run --rm --gpus all --ipc=host --user "$(id -u):$(id -g)" -v /home/huoju/leask/ii42_sae:/workspace/ii42_sae -v /home/huoju/leask/diffsae-codex:/workspace/diffsae-codex -v /home/huoju/leask/runs:/home/huoju/leask/runs -e PYTHONPATH=/workspace/ii42_sae/scripts:/workspace/diffsae-codex/src -w /workspace/ii42_sae leask/ii42-sae:pytorch26.03-aim python3 scripts/research_sae_diffsae_codex_bridge_smoke.py --output-dir /home/huoju/leask/runs/diffsae-codex-bridge-smoke-small --max-rows 256 --n-features 2048 --feature-k 64 --steps 120 --st-after 96 --batch-size 16 --device cuda'
```

Result:

| Metric | Before | After |
| --- | ---: | ---: |
| candidate_hit@1 | 0.5776 | 0.8017 |
| candidate_hit@5 | 0.7069 | 0.8793 |
| candidate_hit@10 | 0.7931 | 0.9138 |
| candidate_mrr@10 | 0.6401 | 0.8382 |

Run facts:

- Corpus root:
  `/home/huoju/leask/runs/m97-stage-a-validation-v2/m21_payload_root/m97_stage_a_eval`
- Usable candidate rows after filtering: `116`
- Train rows: `81`
- Eval rows: `116`
- Documents with usable embeddings in the run: `12042`
- Dense dimension: `768`
- SAE: `2048` features, `k=64`
- Steps: `120`
- Final loss: `0.305`
- Elapsed: `9.47s`

This is strong evidence that the DiffSAE objective can learn on our row format
when the data is clean enough. It is not evidence that the model generalizes:
the smoke is tiny, candidate-set only, and eval rows overlap the tiny filtered
surface. It should be treated as a trainability probe, not a quality claim.

## Why Our Route Kept Turning Into Fine-Tuning

The issue is not that "simple neural training cannot work." The evidence points
to four more concrete causes.

### Cause 1: Objective Mismatch

We often trained one layer and evaluated another:

- Stage A preserved dense/teacher shape.
- Support compression optimized sparse preservation.
- Stage B learned residual ranking over generated candidate rows.
- Runtime postings then generated a different candidate distribution.

Each step was reasonable locally, but the final product objective is one
contract: text/query representation plus BM25/SAE sparse candidate generation
plus final ranking. If the loss is not tied to that contract, training becomes
patchwork.

### Cause 2: Data Surface Mismatch

`diffsae-codex` expects a clean candidate bank. Our bridge smoke had to discard
most candidate rows under strict requirements because some embeddings were
missing and candidate lengths varied. This is fine for our existing pipeline,
but it is bad for a simple differentiable retrieval trainer.

Before judging the route, we need a normalized training surface:

- non-empty query/document embeddings,
- fixed or padded candidate count,
- explicit train/validation/holdout split,
- qrel positives,
- dense hard negatives,
- BM25 hard negatives,
- SAE hard negatives,
- full-corpus/index evaluation separate from candidate-set evaluation.

### Cause 3: Product Target Is Harder Than The Public Smoke

`diffsae-codex` often reports sparse-only retrieval on prepared BEIR subsets.
Our target is a unified BM25+SAE evidence-atom engine that should work on
larger and more varied query families, with runtime postings constraints and
eventual SQL integration.

So the right conclusion is not "their route solves our product." The right
conclusion is "their aligned training contract is probably the missing
discipline in our current route."

### Cause 4: We Mixed Generalization And Local Optimization

Our later Stage-B work used many careful local fixes: surface-aware selection,
runtime-safe features, candidate residuals, source utility, and mixed-surface
selection. These are useful, but they also make overfitting risk harder to
interpret.

DiffSAE's simpler setup is more interpretable:

- candidate source is explicit,
- positive and negative sets are explicit,
- sparse score is the runtime score,
- query-only training can be isolated from document-index training.

That is a better foundation for detecting overfitting.

## Interpretation Of The Overfitting Risk

The user's concern is valid. There are two separate risks:

1. **Benchmark-query overfitting.** Some asymmetric distillation paths can learn
   benchmark query style rather than general retrieval. DiffSAE's own README
   marks some query-distillation results as not clean.
2. **Candidate-set overfitting.** A small fixed candidate bank can be solved
   without improving full-corpus retrieval. The bridge smoke proves this risk:
   it improves quickly on only `116` usable rows.

Therefore, the next run must require:

- strict held-out query splits,
- preferably dataset-family holdout,
- candidate-set metrics and full-corpus sparse-index metrics reported
  separately,
- no benchmark-query distillation counted as clean unless the queries are
  outside the eval split,
- physical cost reported with quality.

## Recommended Next Step

M109 should be reframed as a DiffSAE-style route reset rather than another
surface-aware residual ranker.

### M109.1 Normalize The Training Surface

Build a clean candidate bank from our M97/M107/M81 surfaces:

- keep only rows with query/document embeddings,
- pad or bucket candidate rows to fixed shapes,
- include qrel positives,
- include dense hard negatives,
- include BM25 hard negatives,
- include SAE/postings hard negatives,
- keep train/validation/holdout and dataset-family holdout splits.

### M109.2 Reproduce DiffSAE Objective At Real Scale

Run the same end-to-end sparse retrieval objective on the normalized bank:

- start with sparse-only SAE retrieval,
- evaluate candidate-set heldout,
- export sparse index,
- evaluate full-corpus/postings retrieval,
- report fanout, postings, payload, latency, and quality.

### M109.3 Add BM25 As Complementarity Signal

Only after sparse-only retrieval is stable:

- add BM25 candidates into the bank,
- add BM25 complementarity labels or teacher scores,
- penalize SAE atoms that only duplicate easy BM25 matches,
- measure whether SAE improves semantic/near-miss coverage without increasing
  posting fanout too much.

### M109.4 Keep Stage-B As A Control, Not The Main Route

M105-M108 remain useful evidence that residual ranking can improve a fixed
candidate pool. But if the sparse representation itself is not trained against
the runtime retrieval contract, Stage-B will keep behaving like local repair.

The new mainline should be:

```text
clean candidate bank
-> DiffSAE-style retrieval-aware sparse training
-> exact sparse index evaluation
-> BM25 complementarity
-> only then optional residual ranker
```

## Bottom Line

The `diffsae-codex` route looks promising because it has better objective
alignment, not because it avoids hard problems. It trains the representation
and the retrieval rule together, then serves the same rule. Our route has
valuable pieces, but it repeatedly separated representation, compression,
candidate generation, hybrid scoring, and residual ranking.

The immediate action is to reproduce the DiffSAE aligned objective on a clean,
held-out version of our data. If that works at real scale, it gives a much more
interpretable path toward the unified sparse engine. If it fails, the failure
will be cleaner: either the candidate bank is insufficient, the teacher/backbone
is too weak, or the sparse representation cannot preserve enough semantic
coverage under our runtime cost constraints.
