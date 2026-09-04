# SAE Text-To-Sparse-Atoms Training Plan

Date: 2026-05-13

## Objective

Train a lightweight encoder that maps text directly into sparse retrieval atoms:

```text
text -> sparse atom ids + atom weights
```

This is the next step after the current SAE-over-dense-embedding path:

```text
text -> Snowflake embedding -> SAE -> sparse atoms
```

The product goal is to remove the query-time dependency on a dense embedding
model while preserving the semantic recall gained by SAE atoms. If successful,
the retrieval stack becomes one sparse-impact engine:

```text
BM25 token atoms
+ learned semantic atoms
  -> one posting namespace
  -> impact-head candidate generation
  -> exact sparse doc-row rerank
```

Snowflake remains useful as a training-time teacher, but it should not be
required in the serving path.

## Why This Matters

The current unified BM25+SAE index is physically sparse, but its atom generation
still depends on dense embeddings. That creates three product costs:

- query-time embedding inference remains required;
- index building requires dense embedding generation before SAE encoding;
- the system cannot claim a fully sparse-native retrieval pipeline.

A direct text-to-atoms encoder changes the deployment shape:

```text
query text
  -> compact sparse encoder
  -> atom postings lookup
  -> candidate rerank
```

This keeps the final search engine close to BM25 operationally while adding a
semantic atom channel.

## Non-Goals

- Do not build a vector ANN fallback into this plan.
- Do not put model training inside PostgreSQL.
- Do not freeze the PostgreSQL API before the encoder proves quality.
- Do not claim dense-model removal until the student encoder matches the
  Snowflake+SAE teacher on held-out datasets.
- Do not train only on one benchmark dataset and present it as generalization.

## Training Data Requirements

### 1. Raw Text Corpus

The student encoder needs the same raw text that will be available at indexing
and query time.

Document text sources:

- BEIR corpora for reproducible public quality matrices;
- arxiv title, abstract, and optionally full-text chunks;
- PubMed title, abstract, MeSH, journal, and optionally chunks;
- policy documents and policy chunks;
- internal RAG or memory corpora once stable snapshots exist.

Query text sources:

- BEIR benchmark queries;
- application query logs when available and safe to use;
- synthetic/pseudo queries generated from documents;
- task-specific RAG questions with qrels or proxy labels.

The first product-quality matrix should keep training and evaluation corpora
separate enough to test generalization. If a dataset is used for training, its
result must be reported separately from held-out datasets.

### 2. Teacher Atom Labels

The first teacher is the current best SAE-over-dense path:

```text
text
  -> Snowflake/snowflake-arctic-embed-m-v2.0 embedding
  -> trained SAE
  -> top-k sparse atoms
```

For each document and query, store:

```text
id
text hash
teacher model id
sae model id
active atom ids
active atom weights
active atom budget
embedding normalization
generation timestamp
```

Teacher labels are needed for distillation. They also make runs reproducible
after the dense teacher is removed from the serving path.

### 3. Retrieval Supervision

Distillation alone teaches the student to mimic individual atom vectors. It
does not guarantee good retrieval.

Retrieval supervision should include:

- BEIR qrels;
- internal qrels where available;
- dense-teacher top-k lists;
- BM25 hard negatives;
- atom-index hard negatives;
- cross-dataset held-out query sets.

The dense-teacher top-k list is especially useful because it transfers semantic
neighborhood structure rather than only active atom labels.

### 4. Index-Aware Statistics

The encoder must learn not only accurate atoms, but useful atoms for an inverted
index.

Required statistics:

```text
atom document frequency
atom query frequency
posting list length
mean atom impact
atom recall lift
atom MRR/NDCG lift
candidate fanout per query
rerank terms per query
```

These statistics feed budget and fanout losses. Without them, retrieval-aware
training can collapse into broad, high-DF atoms that improve recall but destroy
index selectivity.

## Model Shape

Use an asymmetric sparse encoder:

```text
document text -> doc sparse atoms
query text    -> query sparse atoms
```

The query and document encoders may share a backbone in early experiments, but
they should expose separate projection heads and budgets. Queries are shorter
and need a more selective activation policy.

Initial model target:

```text
small transformer or embedding backbone
+ sparse atom projection head
+ top-k or differentiable budget gate
```

The output contract should match the unified evidence-atom engine:

```text
int32 atom_ids[]
float32 atom_weights[]
uint16 active_count
float32 norm
```

The first version should optimize for retrievability and deterministic output,
not explainability.

## Loss Design

### Stage 1: Teacher Atom Distillation

Goal:

```text
student(text) approximates Snowflake+SAE atoms
```

Losses:

- support loss: predict active atom ids;
- value loss: match active atom weights;
- rank loss: preserve top atom order;
- budget loss: enforce fixed active atom count;
- norm loss: keep comparable vector scale.

This stage should be run on large unlabeled text because teacher labels are
cheap once generated offline.

### Stage 2: Retrieval-Aware Fine-Tuning

Goal:

```text
query atoms retrieve positive documents before hard negatives
```

Losses:

- qrel contrastive loss;
- dense-teacher listwise distillation;
- BM25 hard-negative margin loss;
- in-batch negative loss;
- teacher top-k distribution KL loss.

This stage should use query/document pairs rather than isolated text rows.

### Stage 3: Index-Aware Regularization

Goal:

```text
keep recall while reducing posting fanout and rerank cost
```

Losses:

- atom DF penalty;
- query fanout penalty;
- candidate-budget penalty;
- load-balance penalty over useful atoms;
- rare-use pruning penalty for dead atoms.

The objective is not minimum sparsity. The objective is useful sparse impact:
high recall per posting touched.

### Stage 4: Unified Atom Calibration

Goal:

```text
BM25 token atoms and learned semantic atoms share one score surface
```

Calibration tasks:

- normalize semantic atom weights into BM25-compatible impact ranges;
- prevent learned atoms from drowning exact lexical matches;
- preserve first-page precision;
- keep final scoring source-blind if possible;
- record whether token and semantic atoms need separate score priors.

If source-blind final scoring regresses MRR, use source-aware calibration only
as an experimental fallback. The preferred physical direction remains one atom
namespace.

## Evaluation Matrix

Every serious run should compare:

```text
pure BM25
BM25 + Snowflake-SAE atoms
BM25 + text-student atoms
BM25 + text-student atoms with index-aware loss
```

Metrics:

```text
Recall@20
Recall@100
MRR@20
NDCG@10
MAP@100
candidate_docs
candidate_postings
rerank_doc_terms
mean latency
p95 latency
payload bytes per document
query encoder latency
```

The minimum success bar is:

```text
BM25 + text-student atoms
  >= pure BM25 by a clear margin
  and within 0.01-0.02 Recall@100 of BM25 + Snowflake-SAE atoms
```

The stronger product bar is:

```text
BM25 + text-student atoms
  matches or beats BM25 + Snowflake-SAE atoms on held-out datasets
  with lower serving complexity
```

## Data Split Policy

Use at least three split levels:

### In-Dataset Holdout

Train and test on the same dataset family, but hold out queries.

Purpose:

```text
detect overfitting and verify basic retrieval learning
```

### Cross-Dataset Holdout

Train on several datasets and test on unseen datasets.

Purpose:

```text
measure whether atoms generalize beyond one benchmark distribution
```

### Product Holdout

Evaluate on arxiv/pubmed/policy or internal RAG sets that were not used for
training.

Purpose:

```text
decide whether the encoder is useful for the actual product workload
```

Do not merge these numbers into one headline. A model can pass in-dataset and
fail cross-dataset.

## Artifact Layout

Suggested local artifact structure:

```text
research/text_atoms/
  corpora/
    {dataset}/documents.jsonl
    {dataset}/queries.jsonl
    {dataset}/qrels.jsonl
  teachers/
    {teacher_id}/{dataset}/doc_atoms.jsonl
    {teacher_id}/{dataset}/query_atoms.jsonl
    {teacher_id}/{dataset}/dense_topk.jsonl
  runs/
    {run_id}/config.json
    {run_id}/model.pt
    {run_id}/training_metrics.json
    {run_id}/atom_stats.json
    {run_id}/retrieval_matrix.json
    {run_id}/index_cost_matrix.json
```

The model artifact must record:

```text
tokenizer
backbone checkpoint
teacher id
SAE id
atom namespace version
doc/query active budgets
normalization
training corpus hashes
```

## Milestones

## Execution Status: 2026-05-14

Current execution reports:

```text
sae-text-to-sparse-atoms-plan-execution-report.md
sae-full-beir-shared-teacher-training-report.md
sae-full15-gap-adaptive-gate-report.md
sae-full15-candidate-budget-training-report.md
sae-full15-candidate-budget-physical-cost-report.md
sae-full15-bm25-sae-candidate-union-report.md
sae-full15-native-bm25-sae-union-readiness-report.md
sae-full15-unified-native-payload-simulator-report.md
sae-full15-unified-payload-c-readonly-report.md
sae-unified-payload-pg-readonly-report.md
sae-milestone22-sae-splade-concept-roadmap.md
```

Status by milestone:

| Milestone | Status | Current Evidence |
| --- | --- | --- |
| T1: Teacher Dataset Builder | Passed for full 15-BEIR shared teacher | `shared_sae_8192_64`, 59,059 training records, 10k unlabeled commons rows |
| T2: Distillation Baseline | Passed as stronger baseline | best student Recall@100 `0.7955` vs BM25 `0.7838` |
| T3: Retrieval-Aware Student | Current frontier | dense-pair run Recall@100 `0.8240`, MRR@20 `0.8231` |
| T4: Index-Aware Student | Useful but not frontier | coverage run Recall@100 `0.8223`, MRR@20 `0.8218` |
| T5: Cross-Dataset Generalization | Reframed as 15-BEIR qrels-backed matrix | all 15 datasets now share one atom id space |
| T6: Product Workload Gate | Not passed | arxiv/pubmed unlabeled helps teacher formation, but direct student distillation trails pure BEIR training |
| T7: Adaptive Student Weight Gate | Diagnostic only | per-query oracle is strong, but LODO threshold/learned gates do not beat fixed `student_w0p5` enough |
| T8: Candidate-Budget Training | New frontier | budget16 loss `0.04` improves all four main metrics over dense-pair baseline |
| T9: Physical Candidate Cost | Promising SAE-only baseline | `d16_p128` preserves near-exact SAE quality with about 969 candidates/query |
| T10: BM25+SAE Candidate Union | New physical frontier | `d8_p128_bm25200` matches exact BM25+SAE with about 739 candidates/query |
| T11: Native/PG Union Readiness | Passed for SAE-side native counters | `d8_p128` SAE side is about 616 candidates and 1.13 ms/query in C, PG smoke about 1.01 ms/query |
| T12: Unified Native Payload Simulator | New engineering frontier | `d8_p128_bm25200_bt64` keeps about 735 candidates/query and cuts BM25 postings to about 678/query |
| T13: Unified Read-Only C Payload | Passed C/Python parity smoke | source-tagged BM25+SAE payload reaches 1.0 exact parity over 15 capped datasets |
| T14: Unified PostgreSQL Payload | Passed first SQL smoke | `UBMXM001` direct bytea and by-id read-only functions pass parity, filter, TID, and diagnostics smoke |
| T15: SAE-SPLADE Concept Encoder | New planning milestone | arXiv:2604.21511 motivates TopK concept-vocabulary training with QD-FLOPs-style sparse-cost gates |

Decision:

```text
Promote the full15 shared-teacher path as the new text-to-atoms research
baseline.

Do not productize the text-only student as a dense-teacher replacement yet.
The student now consistently beats BM25, but still trails the Snowflake-derived
SAE teacher by about 0.0185 Recall@100 and 0.0227 MRR@20 on the 15-BEIR
qrels-backed matrix.

The current best balanced run is:

results/sae/text-atoms/full15-shared-dense-budget16-w0p04-token-char/
```

Full15 mean references:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| `bm25_teacher_sae` | 0.8425 | 0.8458 | 0.7507 | 0.7249 |
| `bm25_student_atoms` best balanced | 0.8324 | 0.8291 | 0.7225 | 0.6913 |

Commons unlabeled follow-up:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `full15-shared-dense-teacher-commons2k-distill-token-char` | 0.8223 | 0.8182 | 0.7065 | 0.6795 |
| `full15-shared-dense-teacher-commons-distill-token-char` | 0.8196 | 0.8142 | 0.7033 | 0.6733 |

Direct unweighted arxiv/pubmed doc-side distillation does not beat the pure
BEIR dense-pair student. Keep unlabeled commons text in the shared teacher path,
but require pseudo-query, dense-neighborhood, or low-weight curriculum work
before using it as a default student-training input.

Adaptive gate follow-up:

| Gate | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Best fixed weight | 0.8240 | 0.8227 | 0.7111 | 0.6827 |
| Best threshold diagnostic | 0.8261 | 0.8237 | 0.7111 | 0.6812 |
| Best learned centroid/kNN by metric | 0.8222 | 0.8238 | 0.7071 | 0.6758 |

Per-query oracle can almost close the Recall@100/NDCG@10 teacher gap, but the
available runtime-safe query features do not generalize well enough in
leave-one-dataset-out evaluation. Treat adaptive weighting as a diagnostic, not
the next main product route.

Candidate-budget training follow-up:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Previous dense-pair frontier | 0.8240 | 0.8231 | 0.7111 | 0.6827 |
| Candidate budget top-8 | 0.8239 | 0.8195 | 0.7110 | 0.6819 |
| Candidate budget top-16, loss `0.01` | 0.8280 | 0.8264 | 0.7154 | 0.6837 |
| Candidate budget top-16, loss `0.04` | 0.8324 | 0.8291 | 0.7225 | 0.6913 |
| Candidate budget top-16, loss `0.08` | 0.8327 | 0.8255 | 0.7223 | 0.6891 |
| Candidate budget top-32 | 0.8277 | 0.8247 | 0.7162 | 0.6833 |

Candidate-budget top-16 with loss `0.04` is the new best balanced training
frontier. Loss `0.08` gives a tiny Recall@100 gain but loses MRR@20 and MAP@100,
so it is the first observed balance tradeoff. This supports the current product
direction: optimize the query atom representation under the same kind of atom
budget the unified sparse index will use at query-time.

Physical candidate follow-up:

| Config | Recall@100 | MRR@20 | Candidates | Postings | Exact@100 Overlap |
| --- | ---: | ---: | ---: | ---: | ---: |
| Full exact SAE student | 0.7278 | 0.4870 | all docs | all postings | 1.0000 |
| Candidate `d16_p128` | 0.7250 | 0.4864 | 968.7 | 1443.7 | 0.9534 |
| Candidate `d32_p128` | 0.7279 | 0.4904 | 1326.6 | 2263.6 | 0.9745 |

This evaluator measures the SAE candidate path alone, not the final BM25+SAE
unified sparse ranking. The result is still important: the trained query atoms
can preserve near-exact SAE quality while opening a bounded candidate set.
The next engineering question is whether BM25 token postings fill the residual
misses without expanding candidate cost too much.

BM25+SAE candidate union follow-up:

| Config | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidates | SAE Postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Exact BM25+SAE | 0.8324 | 0.8291 | 0.7225 | 0.6913 | all docs | all postings |
| `d8_p128_bm25200` | 0.8325 | 0.8291 | 0.7223 | 0.6913 | 739.1 | 781.7 |
| `d16_p128_bm25100` | 0.8321 | 0.8291 | 0.7223 | 0.6910 | 1011.8 | 1443.7 |
| `d8_p64_bm25200` | 0.8275 | 0.8291 | 0.7225 | 0.6909 | 531.7 | 454.4 |

This is the first result that makes the unified physical engine direction more
concrete. BM25 token candidates fill SAE-only misses, so the SAE side can use a
smaller atom/posting budget without losing the exact BM25+SAE quality surface.
The next implementation target should port this union path to the native
payload simulator and read-only PostgreSQL generation.

Native/PG readiness follow-up:

| Config | R@100 | MAP@100 | Union Candidates | Native SAE Candidates | Native SAE Postings | C ms/query |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `d8_p128_bm25200` | 0.8325 | 0.6913 | 739.1 | 615.9 | 782.5 | 1.126 |
| `d16_p128_bm25100` | 0.8321 | 0.6910 | 1011.8 | 971.4 | 1449.5 | 1.346 |
| `d8_p64_bm25200` | 0.8275 | 0.6909 | 531.7 | 387.3 | 454.9 | 0.994 |

Unified native payload simulator follow-up:

| Config | R@100 | MRR@20 | Candidates | SAE Postings | BM25 Postings |
| --- | ---: | ---: | ---: | ---: | ---: |
| `d8_p128_bm25200_bt0` | 0.8325 | 0.8291 | 739.1 | 781.7 | 10185.0 |
| `d8_p128_bm25200_bt128` | 0.8317 | 0.8291 | 738.6 | 781.7 | 1158.8 |
| `d8_p128_bm25200_bt64` | 0.8314 | 0.8298 | 735.5 | 781.7 | 677.5 |
| `d16_p128_bm25200_bt128` | 0.8327 | 0.8291 | 1061.8 | 1443.7 | 1158.8 |

`bt0` keeps the old quality baseline but still assumes full BM25 posting
scans. `bt64` is the current balanced native simulator target: it reduces BM25
posting reads by about 15x with about 0.001 Recall@100 loss versus
`d8_p128_bm25200_bt0`. The next priority should be C/PG engineering
integration for a unified read-only payload, not another training run.

Unified read-only C payload follow-up:

| Variant | Exact Match Rate | Candidates | SAE Postings | BM25 Postings |
| --- | ---: | ---: | ---: | ---: |
| all BM25 query terms | 1.0000 | 740.9 | 787.4 | 679.6 |
| top-16 BM25 query terms | 1.0000 | 738.5 | 787.4 | 381.4 |

The C payload now stores BM25 token dimensions and SAE atom dimensions in one
source-tagged binary structure and reconstructs the same candidate union and
source-aware rerank as Python. `bm25_active_terms=16` fixes the long-query
posting spike observed on `arguana`, reducing its BM25 postings from about
4,392/query to about 226/query in the capped smoke without changing Recall@100.
The first PostgreSQL read-only bytea payload integration is implemented.

Unified PostgreSQL read-only payload follow-up:

| Surface | Status | Coverage |
| --- | --- | --- |
| `ii42_unified_payload_query` | implemented | direct `bytea` query parity |
| `ii42_unified_payload_query_by_id` | implemented | resident-table by-id parity |
| `query_filter` | implemented | single-query smoke |
| `doc_tids` | implemented | non-null TID mapping smoke |
| UBMX parsed cache | deferred | larger-payload decode benchmark first |

This closes the first SQL-visible integration checkpoint for the unified
payload. The next engineering target is a larger BEIR payload benchmark that
separates direct decode cost, by-id fetch/decode cost, traversal cost, and
memory footprint.

The older read-only PostgreSQL smoke for `d8_p128` over 20 queries per dataset
reported about `1.01 ms/query`, `602` candidates/query, and `765` decoded
postings/query. That result validated the SAE-side resident-generation
mechanics. The new `UBMXM001` path is the more relevant integration target
because it owns both BM25 token candidate heads and SAE atom candidate heads in
one payload and exposes that unified path to SQL.

SAE-SPLADE paper planning follow-up:

```text
sae-milestone22-sae-splade-concept-roadmap.md
```

The main planning change is that the next model-side experiment should not be
another fusion-weight sweep. `From Tokens to Concepts: Leveraging SAE for
SPLADE` shows a stronger route: train SAE latents as the sparse output
vocabulary of a learned sparse retriever. For this project, the important
translation is:

```text
token-level encoder hidden states
-> TopK SAE concept vocabulary
-> SPLADE-style max aggregation
-> retrieval fine-tuning with candidate-budget / QD-FLOPs-style cost
-> UBMX runtime query atoms
```

New training priorities:

- add QD-FLOPs-style and physical posting-read metrics to the matrix;
- build a DistilBERT paper-aligned control before trying larger encoders;
- compare concept atoms against BM25, Snowflake-SAE teacher atoms, and current
  text-student atoms;
- test larger latent vocabularies only with fanout and posting-read gates;
- add synonymy/polysemy and multilingual atom-overlap diagnostics before using
  policy data as a quality benchmark.

### T1: Teacher Dataset Builder

Build a reproducible pipeline that exports:

```text
raw text
teacher doc atoms
teacher query atoms
dense-teacher top-k
qrels
hard negatives
atom DF statistics
```

Exit condition:

- five existing sampled BEIR datasets can be regenerated end to end;
- artifacts are deterministic by content hash;
- the teacher path reproduces current Snowflake+SAE matrix numbers.

### T2: Distillation Baseline

Train a first text-to-atoms student using only teacher atom labels.

Exit condition:

- student support overlap with teacher is measured;
- BM25 + student atoms beats pure BM25 on the sampled matrix;
- failure cases identify whether loss is from support mismatch or weight
  mismatch.

### T3: Retrieval-Aware Student

Add qrels, dense-teacher lists, and hard negatives.

Current implementation includes pairwise dense-teacher positives through:

```text
--dense-teacher-pairs-per-query
--dense-teacher-positive-k
--dense-teacher-negative-k
```

This improves first-page ranking in-domain, but it does not solve cross-domain
coverage by itself. The next dense-teacher step should be listwise
distribution matching over teacher top-k lists.

First listwise implementation status:

```text
opt-in listwise dense-teacher loss implemented
in-domain frontier not improved
best listwise run Recall@100 0.7527, MRR@20 0.6408
```

This is a useful negative result. The next training target should move from
static listwise distribution matching to candidate-coverage training: measure
whether the query atom set can open the dense-teacher positives inside a fixed
candidate budget, then optimize that coverage directly.

Five-step follow-up status:

```text
sae-text-to-sparse-atoms-five-step-training-report.md
```

The first coverage objective is implemented and evaluated. It is better than
static listwise distribution matching, but it does not beat the existing
retrieval-aware/dense-pair frontier. The next candidate-coverage iteration
should train against the physical candidate generator:

```text
query atom scores -> selected atom budget -> opened document set -> coverage
```

The small randomly initialized transformer encoder underperformed BM25, so the
next stronger-encoder attempt should use a pretrained retrieval backbone or
SPLADE-style encoder rather than a from-scratch transformer.

Exit condition:

- student improves over distillation-only retrieval quality;
- held-out query MRR does not collapse;
- candidate fanout remains bounded.

### T4: Index-Aware Student

Add fanout and candidate-budget losses.

Exit condition:

- Recall@100 remains within the target band;
- candidate docs and rerank terms approach the current EATMH002 cost profile;
- high-DF atom collapse is reduced.

### T5: Cross-Dataset Generalization

Train on a multi-dataset mix and test on held-out datasets.

Current result:

```text
fixed high student weight: failed
conservative low/gated student weight: narrowly above BM25
teacher-replacement bar: not passed
```

Exit condition:

- model does not only memorize benchmark style;
- degradation versus Snowflake+SAE teacher is quantified;
- a default training corpus recipe is chosen.

### T6: Product Workload Gate

Evaluate on arxiv/pubmed/policy or another product snapshot with defensible
qrels or proxy qrels.

Exit condition:

- product workload quality is good enough to justify engineering integration;
- if labels are unavailable, only efficiency is reported, not recall quality.

## Compute Plan

Phase 1 can use local Mac GPU/MPS for sampled datasets and small backbones.

Full-corpus work should use a staged offline pipeline:

```text
1. generate teacher atoms in shards
2. train student on shuffled shards
3. encode validation corpora
4. build unified sparse payload
5. run recall/cost matrix
```

Large BEIR corpora and internal corpora should not require all dense embeddings
or all teacher labels in memory at once.

## Key Risks

### Teacher Ceiling

The student may only imitate Snowflake+SAE and never exceed it. That is still
useful if serving becomes much cheaper, but it is not a quality breakthrough.

Mitigation:

- use retrieval-aware fine-tuning after distillation;
- use qrels and hard negatives;
- use dense-teacher lists instead of only atom-label mimicry.

### Query Generalization

Short queries may not contain enough information to reproduce dense-teacher
semantic atoms.

Mitigation:

- use asymmetric query/document encoders;
- train on real and synthetic queries;
- use listwise retrieval loss.

### High-Fanout Atom Collapse

The model may learn broad semantic atoms that retrieve too many documents.

Mitigation:

- include atom DF and candidate-budget losses;
- report fanout as a first-class metric;
- reject runs that improve recall only by opening most documents.

### Score Calibration

Learned atoms can dominate BM25 token evidence or under-contribute.

Mitigation:

- calibrate atom weights against the unified evidence-atom scorer;
- compare source-blind and source-aware calibration;
- keep MRR/NDCG as first-page precision gates.

### Product Drift

The model may pass BEIR and fail arxiv/pubmed/policy.

Mitigation:

- build product holdouts before API integration;
- keep corpus snapshots and hashes;
- rerun the matrix when the corpus distribution changes.

## Productization Decision Gate

Start PostgreSQL/API product integration only when the following are true:

```text
1. BM25 + text-student atoms beats pure BM25 on the benchmark matrix.
2. It is close to BM25 + Snowflake-SAE teacher on held-out datasets.
3. Query encoder latency is low enough for serving.
4. Candidate fanout is bounded and compatible with the unified sparse engine.
5. Product workload validation does not contradict benchmark validation.
```

If these fail, keep the current Snowflake+SAE teacher path as the research
baseline and continue model-side work before changing the database API.

## M24 Execution Update: Larger Teacher Distillation

The M24 run tested whether the stronger 12288/16384 SAE teachers could produce
a better direct text-to-atoms student:

```text
sae-milestone24-16384-distillation-report.md
scripts/research_sae_m24_teacher_distillation.py
```

Runs:

| Run | Recall@100 | NDCG@10 | MAP@100 | Active8 Candidates | SAE Posts | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `m24-16384-budget16-w0p04-token-lse` | 0.7829 | 0.6656 | 0.6216 | 393.3 | 235.3 | cost-only |
| `m24-12288-budget16-w0p04-token-lse` | 0.7828 | 0.6638 | 0.6203 | 416.6 | 261.6 | cost-only |
| `m24-16384-budget16-w0p04-token-lse-h384` | 0.7844 | 0.6644 | 0.6219 | 393.9 | 235.6 | cost-only |

Current baseline:

```text
full15-shared-dense-budget16-w0p04-token-char
Recall@100 = 0.8324
NDCG@10    = 0.7225
MAP@100    = 0.6913
```

Decision:

```text
do not promote larger-teacher text-student distillation
freeze 12288/16384 loss tuning
move to M27 exploration closure before engineering
```

The failure mode is clear: larger teachers improve the teacher-side BM25+SAE
signal, but direct text-student distillation collapses to very sparse,
low-cost, low-quality rows. The next training attempt should require a changed
architecture or stronger supervision, not another weight sweep over the same
objective.

## M27 Planning Update

M27 supersedes direct PostgreSQL/API product integration as the next phase.
The product target remains direct text-to-atoms serving, but the current
student has not closed the Snowflake-SAE teacher gap enough to remove
query-time dense embedding dependency.

The M27 closure plan is:

```text
sae-m27-exploration-closure-plan.md
```

This training plan should now be read under the M27 gate:

```text
text-to-atoms final push
SoftSAE learned selector
M22 concept-vocabulary control
EATMH/M21 as evaluation harness only
```

PostgreSQL/API integration can resume only after M27 records a final exit
decision: text-to-atoms promoted, teacher-only harness promoted, or
dense-removal not ready.

## M27 Execution Update

The M27 closure run is complete:

```text
scripts/research_sae_m27_exploration_closure.py
sae-m27-exploration-closure-report.md
```

The direct text-to-atoms line did not pass the product-research gate. The
runner scanned `290` full15 student rows, and the best row remains:

```text
full15-shared-dense-budget16-w0p04-token-char
source = bm25_student_atoms
Recall@100 = 0.8324
MRR@20     = 0.8291
NDCG@10    = 0.7225
MAP@100    = 0.6913
```

Against `teacher_16384`, the remaining ranking gap is still too large for a
dense-removal claim. SoftSAE learned selection and concept-vocabulary controls
also failed their M27 gates, so the current training line is closed as:

```text
dense-removal not ready
```

Future training should require a genuinely new encoder architecture,
supervision source, or concept vocabulary. Do not continue with another
weight-only or budget-only sweep over the current student family.
