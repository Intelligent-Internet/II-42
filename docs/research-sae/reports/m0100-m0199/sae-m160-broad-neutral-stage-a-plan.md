# M160A Broad Neutral Stage-A Dense Distillation Plan

Date: 2026-06-01

## Summary

M160A is a corrective Stage-A run. It addresses the execution gap discovered in
M150: the M150 plan called for broad BEIR plus high-volume neutral
Wikipedia/arXiv/PubMed data, but the current M150 `stagea-v1` checkpoint was
trained from a BEIR15-only manifest.

M160A must not drift into another narrow BEIR-only training run. Its purpose is
to train a sparse atom encoder that is as close as possible to the PPLX dense
teacher before any Stage-B or Stage-C BM25-aware ranking calibration.

Primary goal:

```text
text -> PPLX embedding -> sparse atoms
```

should preserve dense neighborhoods on broad neutral and retrieval-like text.
M160A does not try to prove final BM25+SAE ranking by itself.

## Current Live Training Status

The previous Spark Stage-B canary was stopped because M160A starts a new
Stage-A line and should not wait for more Stage-B tuning.

```text
tmux: bm25sae_m152_b1_streaming_large_canary_v1
docker: bm25sae_m152_b1_streaming_large_canary_v1
run: bm25sae-m152-b1-streaming-stageb-large-canary-v1
```

Final observed snapshot before stopping:

```text
step: 3500 / 3500
loss: 21.438074
scale_sae: 1.042746
scale_bm25: 0.361288
all.dense.hit@20: 0.806078
all.dense.mrr@20: 0.552248
all.sae.hit@20: 0.780029
all.sae.mrr@20: 0.565495
all.bm25_sae.hit@20: 0.781476
all.bm25_sae.mrr@20: 0.568178
```

The stopped M152 snapshot is useful evidence, but it is not the M160A base
checkpoint.

## Why M160A Exists

M130 all-data Stage A used an external neutral mix, but the corpus was small:

```text
documents: 993,336
queries:   205,059
```

Its verified neutral sources included about:

```text
wikipedia: 200,000 docs
arxiv:     199,178 docs
pubmed:    198,576 docs
```

M150 improved BEIR exposure by moving to official BEIR full corpus training, but
the executed `stagea-v1` manifest did not include the planned
Wikipedia/arXiv/PubMed neutral supplement. M160A combines the intended strengths
of both:

- M130's neutral generalization mix;
- M150's full BEIR corpus exposure;
- stricter source-balanced sampling;
- stronger dense-neighborhood preservation gates.

## Non-Goals

- Do not train final BM25+SAE fusion in Stage A.
- Do not optimize only candidate-surface metrics.
- Do not use official held-out test qrel labels.
- Do not claim dense removal from Stage-A-only metrics.
- Do not inherit the M150 checkpoint for the primary run. M160A primary starts
  from random initialization. A warm-start ablation can run later.

## Data Contract

M160A has to materialize a new explicit manifest. The manifest must contain
source labels and must be auditable before training starts.

### BEIR15 Data

Use all available BEIR corpus documents under the canonical PPLX policy, while
isolating test relevance labels.

Allowed:

- official BEIR corpus documents;
- train/dev query text where available;
- document-derived synthetic query-style rows;
- non-test qrels only for auxiliary diagnostics if needed.

Disallowed:

- official test qrel labels;
- held-out test query labels in training;
- selecting checkpoints by held-out test query performance.

If a BEIR dataset only exposes test queries, treat its corpus documents as
unsupervised representation data and keep its test queries/qrels evaluation
only.

### Neutral Corpora

M160A must include neutral data, not only BEIR15.

Required source families:

```text
wikipedia
arxiv
pubmed
```

Preferred optional source families:

```text
commons_arxiv_proxy
commons_pubmed_proxy
policy_or_longform_proxy
web_or_qa_neutral
```

Optional proxy sources are for representation/fanout distribution only. They do
not become quality claims unless qrels or proxy-qrels exist.

### Initial Exposure Target

The sampler should use source weights rather than requiring equal raw row
counts.

Initial per-epoch exposure:

| Source group | Target exposure |
| --- | ---: |
| BEIR corpus docs | 35% |
| Wikipedia docs | 20% |
| arXiv docs | 15% |
| PubMed docs | 15% |
| Other neutral/proxy docs | 5% |
| Query-style rows | 10% additive/interleaved |

The immediate M160A run uses already materialized 1024-dimensional PPLX rows
from `/home/huoju/leask/runs/m130-pplx-all-data-corpus`. That corpus contains
about 200k unique rows each for Wikipedia, arXiv, and PubMed, plus query-style
rows. M160A therefore enforces source exposure through an explicit manifest and
per-source repeats. A later M160 promotion run should expand unique neutral rows
above this if the Stage-A evidence is strong.

Minimum acceptable M160A unique surface:

| Source group | Minimum rows |
| --- | ---: |
| BEIR corpus docs | full available, excluding test-query labels |
| Wikipedia docs | >= 190k |
| arXiv docs | >= 190k |
| PubMed docs | >= 190k |
| Query-style rows | >= 45k per neutral source |

The promotion run should scale beyond these minimums.

## Embedding Policy

Use the same PPLX policy for all sources:

```text
model: perplexity-ai/pplx-embed-v1-0.6B
embedding_dim: 1024
normalize: false
similarity: cosine
max_seq_length: 512
max_text_chars: 6000
dtype/materialization: consistent per manifest generation
```

Do not mix fp32/sdpa and bf16/flash artifacts in one manifest unless the
manifest records the difference and the evaluator explicitly accepts it.

## Model Configuration

Primary run:

```text
run_name: bm25sae-m160a-pplx16384k96-stagea-v1
input_dims: 1024
n_features: 16384
feature_k: 96
checkpoint source: none
device: spark cuda
clearml_project: ii42_sae/M160
```

Optional capacity ablation after the primary run:

```text
n_features: 32768
feature_k: 96 or 128
```

Do not start the capacity ablation until the `16384/k96` source-balanced run is
complete and evaluated.

## Stage-A Objective

M160A Stage A should train representation, not final ranking.

Required losses:

1. Dense reconstruction / cosine preservation.
2. Dense-neighborhood KL or MSE with in-batch and sampled top-k neighbors.
3. Query/document neighborhood preservation for query-style rows.
4. Atom fanout / DF regularization to prevent high-DF semantic spam.
5. Optional semantic coverage auxiliary only when labels are not held-out test
   labels and when the auxiliary does not dominate dense preservation.

Selection metric must include:

```text
dense_neighborhood_overlap
cosine_reconstruction
heldout_source_family_overlap
fanout_cost
SAE-only Recall@100 on non-test dev/canary
BM25+SAE Recall@100 on non-test dev/canary
```

The best checkpoint cannot be selected by BEIR held-out test labels.

## Evaluation Gates

### Representation Gate

M160A Stage A must beat M150 Stage A on dense-shape preservation without
increasing fanout materially.

Compare against:

```text
/home/huoju/leask/runs/bm25sae-m150-pplx16384k96-stagea-v1
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagea-v2
```

Required metrics:

- same-surface neighbor overlap;
- cross-source neighbor overlap;
- cosine reconstruction;
- source-family breakdown;
- atom DF / fanout distribution.

### Retrieval-Readiness Gate

Stage A is not promoted by final ranking, but it must not damage retrieval
readiness.

Report:

- SAE-only Recall@100;
- BM25+SAE Recall@100;
- relevant-hit totals;
- qrel-count buckets;
- per-dataset deltas for `trec-covid`, `fiqa`, `scidocs`, `msmarco`, and one
  large QA dataset such as `nq` or `hotpotqa`.

### Holdout Gate

Official BEIR test queries/qrels are only for final reporting, not model
selection. Any model-selection gate must use non-test, dev, or synthetic
diagnostic splits.

## Execution Steps

1. Stop the current M152 run and preserve its final status.
2. Build `m160a_source_inventory.json` with row counts and source labels.
3. Materialize or reuse PPLX embeddings for every accepted source under one
   explicit policy.
4. Write source manifest files:

```text
/home/huoju/leask/runs/m160a-broad-neutral-manifest/documents.files
/home/huoju/leask/runs/m160a-broad-neutral-manifest/queries.files
/home/huoju/leask/runs/m160a-broad-neutral-manifest/manifest_summary.json
```

5. Run a source-balanced manifest validation before training:

- no missing files;
- all rows have embeddings;
- source counts match inventory;
- no held-out test qrel labels in training rows.

6. Run Stage A from scratch:

```text
bm25sae-m160a-pplx16384k96-stagea-v1
```

7. Evaluate against M130 Stage A v2 and M150 Stage A v1.
8. Only after Stage A passes, open a new Stage B plan. Do not reuse M152
   conclusions blindly; M160A will have a different representation shape.

## Deep Training Policy

Do not interrupt the primary M160A Stage-A run early. The current run should
finish its planned 2 epochs unless the process crashes or the loss clearly
diverges.

Checkpoint selection:

- keep both `bm25sae_stagea_best.pt` and `bm25sae_stagea_latest.pt`;
- prefer the best checkpoint selected by Stage-A objective for evaluation;
- also inspect latest if the last epoch is still improving and the best metric
  appears overly tied to a candidate-eval spike.

Continuation rule:

- if epoch 2 still shows improving loss, neighbor overlap, and source-balanced
  fanout, start a new explicit continuation run instead of overwriting the
  primary run;
- continuation must use `--resume-checkpoint` and a new run name such as
  `bm25sae-m160a-pplx16384k96-stagea-cont-e3-v1`;
- do not continue only because candidate eval fluctuates; Stage A is selected
  primarily by dense-neighborhood preservation and fanout safety.

## First Implementation Tasks

Create these tools before launching M160A training:

1. `research_sae_m160a_build_stage_a_manifest.py`
   - builds source-weighted manifest files;
   - refuses to produce a manifest if Wikipedia/arXiv/PubMed are missing.
2. `run_m160a_broad_neutral_stage_a_spark.sh`
   - launches from scratch under ClearML `ii42_sae/M160`;
   - writes output to
     `/home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-v1`.
3. `research_sae_m160_stage_a_gate.py`
   - compares M130, M150, and M160A Stage A on representation and retrieval
     readiness metrics.

## Acceptance Decision

M160A can replace M150 as the base checkpoint only if it satisfies all of:

- better or equal dense-neighborhood fidelity than M150;
- better cross-source generalization than M150;
- no material fanout regression;
- SAE-only and BM25+SAE readiness metrics do not regress on representative
  non-test gates;
- all source and leakage checks are reproducible from the manifest.

If M160A improves dense fidelity but hurts BM25+SAE readiness, the next step is
not Stage C. It is a Stage-A objective correction that preserves BM25-complement
semantic coverage without turning Stage A into final ranking training.

## Primary Run Completion

The primary M160A run completed on 2026-06-01:

```text
run_name: bm25sae-m160a-pplx16384k96-stagea-v1
output: /home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-v1
steps: 269040
epochs: 2
examples: 103311067
latest checkpoint: bm25sae_stagea_latest.pt
objective-selected checkpoint: bm25sae_stagea_best.pt
```

Latest candidate-eval result:

| Row | Hit@20 | MRR@10 | MRR@20 |
| --- | ---: | ---: | ---: |
| BM25 | `0.6061` | `0.3903` | `0.3951` |
| Dense | `0.6817` | `0.4639` | `0.4677` |
| M160A latest | `0.6648` | `0.4354` | `0.4400` |

Best observed candidate-ranking point:

```text
step: 257250
hit@20: 0.6749
mrr@10: 0.4437
mrr@20: 0.4483
```

Decision: primary M160A is a saved milestone, but Stage A is not closed. The
late straight-through phase still improved ranking, so the next step is a
bounded continuation from `latest.pt`, not Stage B.

Continuation run:

```text
run_name: bm25sae-m160a-pplx16384k96-stagea-cont-e3-v1
resume: /home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-v1/bm25sae_stagea_latest.pt
max_steps: 50000
lr: 5e-5
mode: straight_through from step 1
tau: 0.35 -> 0.25
```

Implementation note: the first continuation attempt was stopped after `750`
steps because the default trainer schedule restarted from `soft/tau ~= 1.0`.
That was not a true late-stage continuation. The launcher now exposes schedule
parameters and the continuation defaults to low-learning-rate straight-through
training.
