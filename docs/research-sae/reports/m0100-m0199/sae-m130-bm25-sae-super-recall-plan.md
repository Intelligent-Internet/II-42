# SAE M130 Canonical bm25sae SOTA Plan

Date: 2026-05-24

## Summary

M130 is the current completed `bm25sae` candidate baseline. It replaces the
older split `super-recall`, distributed execution, and realignment notes with
one canonical M130 plan.

After M150 started, M130 is no longer the only active project track. Its role
is now to preserve the best completed PPLX/DiffSAE BM25+SAE candidate and the
comparison gate that the broader M150 Stage-A reset must beat.

The goal is direct SOTA on the same full-corpus retrieval surface:

- use `perplexity-ai/pplx-embed-v1-0.6B` as the primary semantic
  teacher/backbone;
- use DiffSAE-style sparse training as the representation engine;
- use all available non-test documents and representation queries for Stage A;
- use all held-out-safe qrel/query supervision for Stage B;
- train BM25 lexical evidence and SAE semantic evidence as one retrieval
  system, not as disconnected post-hoc fusion;
- promote only if `BM25+SAE` beats `BM25+dense` on the same corpus, qrels,
  metrics, and top-k.

The M130 run family name is `bm25sae`. M130-related runs, ClearML tasks, tmux
sessions, logs, and output directories must begin with `bm25sae-`. Earlier
`m130-pplx-diffsae-*` artifacts are controls only, not the promoted mainline.

## Non-Negotiable Rules

1. Do not restart small candidate-surface experiments as the main task.
2. Do not initialize from M128/Snowflake checkpoints except in explicitly named
   legacy ablations.
3. Do not use `small_general`, five-dataset smoke tables, or candidate-hit
   metrics as the M130 verdict.
4. Do not change the promotion gate after seeing results.
5. If a run fails because of infrastructure, shape, memory, tracking, or script
   bugs, fix the blocker and resume the same plan. Do not reduce the data
   scope or silently switch objectives.
6. Batch size, checkpoint interval, and sharding may be adjusted for stability.
   Dataset coverage, held-out split rules, core losses, and full-corpus gate
   must not be weakened.
7. Every promoted or failed candidate must leave local artifacts, ClearML
   metrics, full-corpus quality metrics, physical-cost metrics, and taxonomy.

## Current Evidence

The M129 frontier before M130:

| Profile | SAE postings/query | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M125 teacher-coverage w1.0 | 36,743 | 0.4269 | 0.5035 | 0.3729 | 0.2388 |
| M128 recall-tune w1.0 | 35,113 | 0.4261 | 0.5077 | 0.3755 | 0.2401 |

Restricted same-docset PPLX evidence showed PPLX is a stronger teacher/control
than Snowflake:

| Row | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Snowflake dense | 0.4388 | 0.5044 | 0.3844 | 0.2584 |
| BM25+Snowflake dense | 0.4412 | 0.4960 | 0.3796 | 0.2537 |
| M128 BM25+SAE | 0.4261 | 0.5077 | 0.3755 | 0.2401 |
| PPLX dense | 0.4600 | 0.5157 | 0.4003 | 0.2712 |
| BM25+PPLX dense | 0.4547 | 0.5062 | 0.3872 | 0.2583 |

All-data PPLX artifacts already exist on `spark`:

| Artifact | Rows |
| --- | ---: |
| `documents.pplx.jsonl` | 993,336 |
| `queries.pplx.jsonl` | 205,059 |

The held-out-safe supervised surface is smaller and must not be confused with
the representation corpus:

| Surface | Rows |
| --- | ---: |
| Qrel-backed train queries | 5,620 |
| Qrel pairs | 100,060 |
| Eval queries | 886 |
| Candidate rows from first PPLX surface | 5,620 |

All-data PPLX full-corpus baseline:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.4692 | 0.3240 | 0.2960 | 0.2406 |
| PPLX dense | 0.5777 | 0.4807 | 0.4482 | 0.3859 |
| BM25+PPLX dense score fusion | 0.5775 | 0.4534 | 0.4216 | 0.3540 |

Execution correction: the existing
`/home/huoju/leask/runs/m130-pplx-all-data-corpus/quality_qrels.json` contains
the `5,620` non-eval train qrels. It is valid for training-surface diagnostics
but not for final promotion. The held-out all-data gate must use
`/home/huoju/leask/runs/m130-pplx-all-data-eval-corpus`, which combines the
`993,336` all-data document index with the `886` held-out PPLX eval queries and
qrels. Stage A is unaffected because it trains representation without qrels;
Stage B/D must use the held-out eval corpus for promotion metrics.

The first `m130-pplx-diffsae-8192k64-all-v1` run is a bridge/control. It
improved candidate-surface hit@10 but did not establish the full-corpus gate:

| Surface metric | Dense control | Bridge model |
| --- | ---: | ---: |
| hit@10 | 0.8296 | 0.8691 |
| hit@20 | 0.8612 | 0.9210 |
| mrr@10 | 0.6842 | 0.6760 |
| mrr@20 | 0.6865 | 0.6798 |

Interpretation: current evidence says the promising signal is coverage, but
the product blocker is full-corpus BM25+SAE ranking quality. The next task is
not another small sweep. It is a full `bm25sae` training line.

## Hardware And Tracking

| Host | Role |
| --- | --- |
| `spark` / `huoju@100.123.2.95` | Primary training host and ClearML/Aim monitoring |
| `homeai` / `huoju@100.86.174.112` | PPLX materialization or validation on RTX 3090 GPU1 only; do not touch GPU0 |
| `xiaoni-mbp.local` | Preprocessing, sharding, merge validation |
| `Flora` | Orchestration, reports, small validation |

ClearML:

```text
Web UI: http://100.116.110.26:8080
API: http://100.116.110.26:8008
File Server: http://100.116.110.26:8081
```

Tracking rules:

- every long training run must create a ClearML task under project
  `ii42_sae/M130`;
- every task name must start with `bm25sae-`;
- local `events.jsonl` remains mandatory even when ClearML is healthy;
- do not pass ClearML secrets on command lines in future runner scripts.

Runtime note: TEI remains a useful serving shape, but M130 quality does not
depend on TEI. PyTorch/SentenceTransformer materialization is accepted if the
result is PPLX 1024-dimensional, unnormalized embedding rows evaluated with
cosine similarity.

## Data Contract

M130 has two different data roles:

1. Representation data: all available non-test documents and representation
   queries. This includes Wikipedia/arXiv/PubMed/BEIR-derived material already
   staged into the all-data PPLX corpus. It trains the sparse semantic space.
2. Supervised ranking data: only held-out-safe query/qrel rows. This trains
   BM25/SAE complementarity and ranking calibration.

The eval query IDs remain excluded from training qrels and candidate rows.
Eval documents may remain in Stage-A/index materialization because production
must encode corpus documents; the protected signal is relevance labels, not
document existence.

## Model Contract

M130 trains a PPLX-space sparse retrieval model first. It does not claim that
runtime dense dependency is removed. The immediate product-quality question is
whether `BM25+SAE` can beat `BM25+dense` while using a unified sparse evidence
surface.

The canonical primary configuration is:

| Field | Value |
| --- | --- |
| Backbone/teacher | `perplexity-ai/pplx-embed-v1-0.6B` |
| Input dimension | 1024 |
| Sparse features | 16,384 |
| Query/doc active budget | start at 96 |
| Retrieval scoring | cosine-preserving sparse dot, normalized before final ranking |
| Run prefix | `bm25sae-pplx16384k96` |

Escalation is evidence-based, not a blind sweep:

- if taxonomy shows candidate coverage is still the bottleneck, test
  `k128` or wider fanout only after preserving physical-cost diagnostics;
- if taxonomy shows score-ordering misses dominate, keep the representation
  and train ranking/calibration harder;
- if taxonomy shows high-DF atom noise dominates, add fanout/DF regularization
  or atom utility pruning;
- if PPLX-space full-corpus BM25+SAE cannot approach BM25+dense, do not
  proceed to text-to-atoms distillation.

## Stage A: Full-Corpus DiffSAE Representation Training

Purpose: build a sparse semantic space that preserves PPLX neighborhood
structure before ranking supervision is applied.

Inputs:

- `993,336` document embeddings;
- `205,059` query embeddings;
- optional Snowflake/Stella rows only as controls, not blockers.

Loss components:

1. DiffSAE reconstruction/sparse preservation:
   - reconstruct PPLX embedding shape from top-k sparse activations;
   - preserve cosine similarity and high-value latent direction;
   - keep activation budget stable, not merely low.
2. Neighborhood distillation:
   - sample query/document anchors;
   - use PPLX dense top-k and near-miss neighborhoods as teacher targets;
   - train sparse dot ranking to preserve teacher neighborhoods.
3. Coverage objective:
   - reward qrel-positive and dense-neighborhood coverage where available;
   - do not optimize only for reconstruction if retrieval coverage degrades.
4. Fanout/DF objective:
   - maintain running atom document frequency;
   - penalize high-DF atoms that add postings without useful teacher coverage;
   - preserve useful mid-frequency semantic atoms.

Stage-A acceptance:

- sparse neighborhood overlap improves over the bridge run;
- query/document atom DF distribution is stable;
- full-corpus BM25+SAE evaluator can load the checkpoint without dimension or
  payload mismatch;
- no promotion claim is made from Stage-A-only metrics.

Primary output:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagea-v2/
```

## Stage B: BM25-Aware Ranking Training

Purpose: train the retrieval system, not the SAE alone. BM25 lexical evidence
and SAE semantic evidence are optimized together.

Candidate construction must include:

- all qrel positives;
- BM25 hard negatives;
- PPLX dense near-misses;
- Stage-A SAE near-misses;
- BM25+dense hits that BM25+SAE misses;
- random background negatives;
- high-DF atom false positives.

Loss components:

1. Listwise qrel ranking loss:
   - optimize NDCG/MAP-style ordering where human labels exist;
   - keep all positives in the candidate row, not only the first hit.
2. Teacher coverage loss:
   - preserve PPLX dense relevant neighborhoods;
   - keep dense-hit/SAE-miss examples visible.
3. BM25 complementarity loss:
   - reward SAE when it covers semantic misses BM25 does not cover;
   - avoid wasting SAE capacity on lexical matches already solved by BM25.
4. Score calibration loss:
   - learn BM25/SAE scale and semantic expansion strength at query level;
   - use runtime-safe signals only: BM25 concentration, atom entropy, top atom
     mass, predicted fanout, query length, and candidate overlap.
5. Physical-cost loss:
   - penalize postings, candidate docs, and rerank terms when quality does not
     improve;
   - do not let recall gains hide unbounded fanout.

Stage-B acceptance:

- candidate-surface quality must beat dense on hit@10/hit@20 without losing
  ranking sharpness;
- full-corpus BM25+SAE must be evaluated after every candidate winner;
- the SOTA gate is still the full-corpus gate, not Stage-B candidate metrics.

Primary output:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stageb-v1/
```

## Stage C: Full-Corpus Hard-Negative Refresh

Purpose: close the gap between candidate-surface wins and real full-corpus
index behavior.

Procedure:

1. Run the full-corpus evaluator with BM25, PPLX dense, BM25+PPLX dense, SAE,
   and BM25+SAE rows.
2. Write rankings for every eval query.
3. Run miss taxonomy against BM25+dense and PPLX dense controls.
4. Build refreshed candidate rows from:
   - BM25+dense relevant hits missed by BM25+SAE;
   - SAE candidates ranked too low;
   - high-score wrong SAE candidates;
   - high-DF noisy atoms;
   - qrel positives preserved unconditionally.
5. Continue training from the Stage-B checkpoint.
6. Re-run full-corpus quality and physical-cost evaluation.

Stage C is mandatory unless Stage B already beats BM25+dense on the full
gate. If Stage B fails, Stage C is the next step; do not jump to a new model
idea before taxonomy.

Primary output:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagec-v1/
```

## Stage D: Final Full-Corpus Gate

The evaluator must output all rows on the same corpus/qrels/top-k surface:

- BM25;
- PPLX dense;
- BM25+PPLX dense RRF;
- BM25+PPLX dense score fusion;
- SAE;
- BM25+SAE;
- M128/Snowflake control where available.

Quality metrics:

- Recall@10;
- Recall@20;
- Recall@100;
- MRR@10;
- MRR@20;
- NDCG@10;
- MAP@100.

Physical metrics:

- BM25 postings/query;
- SAE postings/query;
- accumulator/candidate docs/query;
- rerank terms/query;
- payload size;
- latency mean/p50/p95 if available.

Promotion gate:

| Metric | Requirement |
| --- | --- |
| Recall@100 | BM25+SAE > BM25+PPLX dense score fusion and PPLX dense |
| NDCG@10 | BM25+SAE > BM25+PPLX dense score fusion and close or above PPLX dense |
| MAP@100 | BM25+SAE > BM25+PPLX dense score fusion and close or above PPLX dense |
| MRR@20 | no material regression versus BM25+dense |
| Cost | report postings/candidates/latency; quality-cost Pareto must be credible |

If BM25+SAE beats BM25+dense but not PPLX dense, the result is a strong
BM25+SAE product candidate but not a dense-replacement claim. If it beats both
BM25+dense and PPLX dense on the required metrics, it becomes the M130 SOTA
checkpoint.

## Error Handling

Infrastructure failures:

- fix container mounts, Python paths, CUDA/MPS device settings, file paths,
  checkpoint shape inference, and ClearML logging immediately;
- resume the same stage from the last valid checkpoint;
- do not switch to a smaller dataset unless it is a named smoke test.

OOM or throughput failures:

- reduce batch size, increase gradient accumulation, shard the input, or stream
  JSONL instead of loading all rows;
- do not reduce corpus coverage or qrel supervision without marking the run as
  non-mainline.

Bad training curve:

- first inspect loss components, atom DF distribution, candidate source
  contribution, and taxonomy;
- adjust the responsible objective within the stage;
- do not start unrelated sweeps or new backbones before the current taxonomy is
  complete.

Evaluation failures:

- fix evaluator bugs and rerun;
- PPLX is 1024-dimensional, so legacy 768-dimensional defaults are invalid;
- every checkpoint must carry enough config to infer input dimension and sparse
  feature count.

## Execution Checklist

### 0. Lock The Mainline

- Stop all non-mainline training tasks.
- Keep existing PPLX embeddings and dense baseline artifacts.
- Ensure every new run name starts with `bm25sae-`.
- Keep `m130-pplx-diffsae-8192k64-all-v1` only as a bridge/control.

### 1. Verify Full-Data Inputs

- Confirm `documents.pplx.jsonl` row count is `993,336`.
- Confirm `queries.pplx.jsonl` row count is `205,059`.
- Confirm qrel train rows are `5,620` and eval rows are `886`.
- Confirm eval query IDs are excluded from training qrels.
- Record exact artifact paths in the final report.

### 2. Run Stage A

- Train `bm25sae-pplx16384k96-stagea-v2` from scratch.
- Log to ClearML and local `events.jsonl`.
- Save periodic checkpoints.
- Select the best checkpoint by dense-neighborhood preservation minus the full
  Stage-A loss, not by overlap alone. The initial random batch can show
  misleadingly high local overlap.
- Export atom DF/fanout summaries.
- Run smoke full-corpus load to catch shape/config bugs early.

### 3. Run Stage B

- Build BM25-aware candidate rows from Stage-A outputs and existing PPLX dense
  rankings.
- Train `bm25sae-pplx16384k96-stageb-v2`.
- Evaluate candidate-surface and full-corpus quality.
- If full-corpus gate passes, produce final report.

### 4. Run Stage C

- Run miss taxonomy on the Stage-B full-corpus output.
- Build refreshed hard-negative rows from actual BM25+SAE misses.
- Train `bm25sae-pplx16384k96-stagec-v1`.
- Re-run full-corpus quality and physical metrics.

### 5. Escalate Only From Evidence

- If candidate coverage misses dominate, run `bm25sae-pplx16384k128-stagec-v1`.
- If score-ordering misses dominate, improve Stage-B/Stage-C ranking loss.
- If fanout/noise dominates, strengthen DF/utility budget loss.
- If all variants fail, write a failed-to-beat-dense report with taxonomy and
  stop M130 rather than drifting into unrelated experiments.

## Final Report Requirements

The final M130 report must include:

- exact corpus sizes and held-out split rules;
- all run names and ClearML task links;
- quality matrix for BM25, PPLX dense, BM25+dense, SAE, BM25+SAE;
- physical-cost matrix;
- miss taxonomy for every evaluated mainline checkpoint;
- explanation of whether the final checkpoint is promoted, not promoted, or
  needs another taxonomy-driven Stage-C pass;
- explicit statement that M130 is PPLX-space BM25+SAE SOTA work, not yet a
  text-to-atoms dense-removal product claim.

## Current Next Action

The next action is to implement the `bm25sae` executor around this plan:

1. verify existing full-data artifacts;
2. add or adapt scripts so Stage A trains on full PPLX doc/query embeddings,
   not only `5,620` candidate rows;
3. ensure ClearML logging works without command-line secrets;
4. launch `bm25sae-pplx16384k96-stagea-v2` on `spark`;
5. proceed through Stage B, Stage C, and final report without changing the
   promotion gate.
