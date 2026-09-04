# SAE M70 End-To-End Deep Text-To-Atoms Training Plan

Date: 2026-05-21

Status: implementation started. This is the next model-side line after the
M60 scaled supervised stage failed to close the product blocker.

## Summary

M60 showed that adding more labels to the current small query-side family is
not enough. The best M60 frozen control remains useful evidence, but it still
trails the fixed-doc teacher by a large margin:

| Metric | M60 best | Teacher fixed-doc | Gap |
| --- | ---: | ---: | ---: |
| Recall@100 | 0.8076 | 0.8447 | -0.0371 |
| MRR@20 | 0.8010 | 0.8399 | -0.0389 |
| NDCG@10 | 0.6884 | 0.7490 | -0.0606 |
| MAP@100 | 0.6509 | 0.7247 | -0.0738 |

The next line should therefore stop treating the current text-to-atoms model
as a nearly solved calibration problem. M70 resets the model training shape:

```text
text -> deep query/doc atom encoder -> hard sparse atoms
     -> unified BM25 + SAE evidence-atom scoring
     -> supervised final retrieval objective
```

The product target is still query-time dense removal, but M70 does not claim it
up front. Dense teachers are allowed during training and data generation. The
deployed path must be text to sparse atoms plus BM25, with no dense embedding
required at query time.

## Why M70 Exists

The previous line was too small in three ways:

- The data surface was too narrow. Even after M60B, the effective train scope
  was only 3467 queries and 52672 qrel pairs after per-dataset caps.
- The objective was too indirect. Many runs either imitated a teacher shape or
  tuned scalar gates around a fixed atom representation, instead of optimizing
  the final BM25+SAE ranked list.
- The model family was too shallow. The best checkpoints are valuable
  diagnostics, but they are not a convincing final encoder for broad RAG,
  memory retrieval, and AI knowledge search.

M70 should treat model training as the central product problem, not as a small
post-hoc correction over a fixed SAE teacher.

## External Lesson: diffsae

The [`adaminti/diffsae`](https://github.com/adaminti/diffsae) project is useful
because it demonstrates a clean differentiable sparse retrieval path:

- text encoder plus TopK SAE head;
- soft top-k surrogate for gradients and hard top-k for deployed inference;
- straight-through hard/soft alignment;
- multi-positive contrastive training rather than single-positive InfoNCE;
- sparse inverted-index dot product as the real inference target;
- evidence that sparse top-k can approach dense quality when the model is
  trained for the sparse path.

The part we should not copy directly is the task definition. diffsae optimizes
mostly same-document chunk retrieval on one in-domain corpus. M70 must optimize
human-labeled retrieval and the final BM25+SAE ranking surface across broad
corpora. The right lesson is not "imitate dense harder"; it is "make the
deployed sparse retrieval operator differentiable enough to train against."

## Product Target

M70 aims for a compact, explainable sparse retrieval model:

```text
query text
  -> query atoms + query atom weights + runtime-safe gate/budget signals
  -> BM25 postings + SAE atom postings
  -> one ranked candidate list

document text
  -> document atoms + document atom weights
  -> unified sparse payload indexed beside lexical postings
```

BM25 already covers exact lexical evidence. SAE should spend its capacity on
semantic residual evidence: paraphrase, abbreviation, broader concept match,
cross-domain terminology, and cases where literal token matching under-recovers.
The loss must discourage wasting SAE postings on evidence that BM25 already
captures well.

## Data Strategy

M70 uses two data classes and keeps their roles separate.

### A. Large Unlabeled / Weakly Labeled Text For Representation

Use as much high-quality text as practical, excluding held-out evaluation
queries/documents where leakage is possible:

| Source | Role | Initial Scale Target |
| --- | --- | ---: |
| Wikipedia | broad general-language concept coverage | 1M-5M passages |
| arXiv | scientific terminology and long-form abstracts | 500k-2M passages |
| PubMed / PMC | biomedical terminology and clinical language | 500k-2M passages |
| BEIR15 corpora non-heldout docs | benchmark-domain coverage | all allowed docs |
| Commons arXiv/PubMed/policy mirrors | target workload distribution and DF/fanout statistics | sampled, no quality claim |
| Existing M39/M60 broad sources | continuity with prior query distribution work | all useful rows |

These sources are allowed to train representation shape, atom utilization,
fanout priors, and teacher-neighborhood consistency. They are not sufficient
quality proof unless they have qrels or defensible proxy labels.

### B. Supervised Query/Judgment Sources For Retrieval Meaning

Use every defensible supervised retrieval source, with strict split isolation:

| Source Family | Use |
| --- | --- |
| BEIR15 train/dev where available | broad benchmark supervision |
| MS MARCO passage train/dev and TREC DL | large-scale web/passage relevance |
| NQ / TriviaQA style retrieval pairs | natural question semantics |
| LoTTE / domain retrieval sets | robustness outside BEIR |
| SciFact / NFCorpus / TREC-COVID style biomedical judgments | broad, high-DF, many-positive queries |
| FiQA / ArguAna / CQADupStack / Quora style sets | short/ambiguous user-query behavior |
| Dense-teacher near misses and BM25 hard negatives | candidate coverage and complementarity |
| SAE-model near misses from previous milestones | failure replay |

The goal is not to make BEIR15 look good by overfitting its test queries. M70
must create explicit train/dev/test manifests, family holdouts, and leakage
keys. Full15 remains an anchor, but not the only acceptance surface.

## Split And Leakage Rules

- Every supervised dataset gets stable query-id based train/dev/test splits
  unless the dataset already defines them.
- Full15 reported test rows must never appear in supervised training.
- Near-miss mining may use held-out corpus documents, but not held-out qrel
  labels as training labels.
- If a corpus contributes both representation text and evaluation labels,
  the evaluation query texts and qrel positives are excluded from training
  supervision. Document text may be used for unsupervised corpus modeling only
  when the report marks that choice explicitly.
- All generated query/pseudo-query rows must carry provenance, parent document
  id, generation method, and whether quality claims are allowed.

## Model Architecture

M70 should test a deeper but still deployable encoder family.

### Shared Backbone

Use an open pretrained text encoder as initialization, not as a query-time
dense dependency:

- primary candidate: `Snowflake/snowflake-arctic-embed-m-v2.0`, because the
  current teacher and artifacts already use it;
- control candidates: smaller retrieval encoders only if they reduce inference
  cost without hurting ranking;
- the deployed artifact is the trained text-to-atoms model, not the dense
  teacher embedding.

### Asymmetric Heads

Use a shared text backbone with separate query and document heads:

```text
backbone(text)
  -> contextual pooled features
  -> query atom head: logits, weights, budget/gate
  -> document atom head: logits, weights
  -> optional lexical residual head: BM25/SAE mix calibration
```

Query and document atoms should be asymmetric because query language and
document language have different entropy, length, and fanout behavior.

### Sparse Atom Layer

Adopt a diffsae-style differentiable sparse layer:

- soft top-k / sparsemax-like relaxation during training;
- straight-through hard top-k for forward-path realism;
- hard top-k atom export for inference;
- explicit atom fanout and utilization regularization;
- support for larger dictionaries: start with 16384 and 32768, then promote
  only if quality/cost Pareto improves.

The active budget is not fixed globally. The model may predict query/doc
budget within bounded ranges, but only from runtime-safe signals:

```text
query length, token entropy, BM25 concentration, predicted atom fanout,
top atom mass, atom entropy, and lexical/semantic uncertainty
```

No dataset id, qrels, or dense runtime embedding may be used by the deployed
selector.

## Retrieval Operator To Train Against

M70 should optimize the same retrieval surface we want to deploy:

```text
score(q, d) =
    lexical_score_bm25(q, d)
  + dynamic_sae_scale(q) * sparse_atom_dot(q_atoms, d_atoms)
  + optional residual calibration terms that are runtime-safe
```

The key change is that `dynamic_sae_scale(q)` and atom budgets are learned as
part of the model, not selected after the fact by hand.

The training candidate pool must include:

- qrel positives;
- BM25 top-k;
- dense-teacher top-k;
- current SAE top-k;
- model near-misses from previous checkpoints;
- hard negatives from documents that BM25 or SAE over-promote;
- random and low-score negatives for calibration.

## Loss Design

M70 is not teacher imitation. Teacher signals are auxiliary priors.

### 1. Final Ranking Loss

Use listwise and pairwise ranking losses over the final BM25+SAE scores:

- listwise softmax / KL against graded qrel distribution where available;
- LambdaRank / NDCG-weighted pairwise loss for ordered judgments;
- positive-vs-hard-negative margin loss;
- query-level no-collapse penalty for hard families.

### 2. BM25 Complementarity Loss

Reward SAE atoms when they recover relevant documents missed or under-ranked by
BM25. Penalize SAE capacity spent on documents BM25 already ranks confidently
unless the label distribution proves the semantic signal improves ordering.

This should produce a more efficient combined engine: lexical postings solve
lexical evidence; atom postings solve semantic residual evidence.

### 3. Teacher Geometry Distillation

Use dense teacher and existing SAE teacher as regularizers, not as the main
target:

- preserve local dense neighborhoods where labels are absent;
- preserve atom support/value shape enough to keep sparse retrieval stable;
- downweight teacher when qrels and repeated human judgments disagree.

### 4. Candidate Coverage Loss

The model must learn to place at least one relevant positive into the candidate
pool before reranking. This loss should operate before final top-k selection,
with candidate-budget awareness.

### 5. Physical Cost Loss

Hard constraints are part of the training objective:

- SAE postings;
- candidate docs;
- rerank doc terms;
- payload MB;
- atom DF/fanout;
- query latency proxy.

Quality wins that explode postings are not product wins.

### 6. Robustness / Group DRO

Train with group-aware penalties so gains on easy datasets cannot hide collapse
on broad-query or many-positive datasets:

```text
groups = dataset, dataset-family, query length bucket, lexical-heavy,
semantic-heavy, many-positive, high-DF, biomedical, web passage
```

## Training Stages

### M70A: Data And Judgment Manifest Build

Deliverables:

- unified source registry v2;
- train/dev/test split manifest;
- leakage report;
- qrel and proxy-qrel provenance table;
- BM25 / dense / SAE candidate caches for all supervised sources;
- corpus-scale atom DF and fanout statistics.

Acceptance:

- every reported eval source has an explicit leakage rule;
- full15 test labels are isolated;
- source counts are checked before launching any expensive training.

### M70B: Large Representation Pretraining

Goal: make the encoder preserve broad language and semantic structure before
retrieval fine-tuning.

Training signals:

- multi-positive contrastive pairs from same document, adjacent passages,
  citations, title-abstract-body, and teacher neighborhoods;
- dense-teacher neighborhood distillation;
- atom utilization and fanout regularization;
- hard top-k straight-through path enabled early enough to avoid train/deploy
  mismatch.

Scale target:

```text
millions of passages, multiple epochs, Spark/NVIDIA training as primary host
```

This stage is evaluated by teacher-neighborhood recall, atom utilization,
fanout distribution, and retrieval smoke tests, not by final product quality.

### M70C: Supervised End-To-End Retrieval Training

Goal: optimize final BM25+SAE ranking with human relevance labels.

Training signals:

- full candidate pool from BM25, dense teacher, prior SAE, and hard negatives;
- final ranking loss on `BM25 + SAE`;
- dynamic SAE scale/budget learning;
- complementarity loss;
- physical-cost loss;
- group-DRO collapse prevention.

This is the first stage that can produce a product-research candidate.

### M70D: Iterative Hard Negative Refresh

Run the model, mine new misses, then retrain:

```text
train -> full eval -> miss taxonomy -> hard-negative refresh -> train
```

Stop after the Pareto frontier stops improving, not after one arbitrary epoch
count.

### M70E: Export And Native Payload Evaluation

Export hard atoms and run the existing EATMH/M21 physical harness:

- Python/C/PostgreSQL parity;
- candidate docs;
- SAE postings;
- BM25 postings;
- rerank terms;
- payload MB;
- cached by-id latency;
- real corpus efficiency only for arXiv/PubMed/policy unless qrels exist.

## Infrastructure Plan

Use multiple machines by role:

| Machine | Role |
| --- | --- |
| Local Mac | orchestration, manifest build, small correctness runs |
| `xiaoni-mbp.local` | data prep / CPU-heavy preprocessing if reachable |
| Spark `huoju@100.123.2.95` | primary GPU training and large evaluation |

Storage rules:

- local stable temporary data: `/Volumes/Betty/Tmp`;
- Spark workspace: `/home/huoju/leask`;
- no large ephemeral dependency on `/tmp`;
- every expensive run writes manifest, config, git commit, source counts, and
  host metadata before the first epoch.

Monitoring:

- Aim may be used for long Spark runs;
- reports must include loss curves, quality curves, and physical-cost curves;
- early stopping may stop a bad run, but expensive training is allowed when
  validation evidence says the objective is still improving.

## Evaluation Matrix

Quality:

- Recall@20 / Recall@100;
- MRR@20;
- NDCG@10;
- MAP@100;
- per-dataset and per-family deltas;
- no-collapse table for hard families.

Physical:

- candidate docs;
- BM25 postings;
- SAE postings;
- rerank terms;
- payload MB;
- mean and p95 latency;
- atom DF and utilization.

Baselines:

- BM25 only;
- current M60 best;
- teacher fixed-doc;
- dense teacher where available;
- current EATMH002 physical profile.

New non-BEIR heldouts should include at least MS MARCO dev/TREC DL style
judgments, NQ-style natural questions, and one domain holdout such as LoTTE or
biomedical retrieval if data availability allows.

## Promotion Gates

M70 can reopen product engineering only if it passes all gates.

### Quality Gate

At minimum:

- beats BM25 and M60 best on full15 aggregate;
- closes most teacher gap on ranking metrics;
- does not collapse on `trec-covid`, `msmarco`, or other hard families;
- improves or matches on non-BEIR heldout labels.

Target product-research threshold:

| Metric | Requirement vs teacher fixed-doc |
| --- | --- |
| Recall@100 | gap >= -0.010 |
| MRR@20 | gap >= -0.015 |
| NDCG@10 | gap >= -0.020 |
| MAP@100 | gap >= -0.025 |

If the learned final BM25+SAE objective beats teacher on human relevance while
remaining physically efficient, that is stronger than matching teacher shape.

### Physical Gate

- Query-time path is text-to-atoms plus BM25, no dense embedding.
- Candidate docs, SAE postings, and payload MB must stay within a clear
  Pareto frontier against EATMH002/M60 profiles.
- A model that improves quality only by exploding atom fanout is not promoted.

### Robustness Gate

- Full15 aggregate is not enough.
- Must include family holdout or LODO-style validation.
- Must include hard-query taxonomy before/after.
- No single benchmark family may be sacrificed to pass aggregate.

## Decision Rules

- If M70B improves teacher-shape retention but M70C cannot improve human
  ranking, the failure is objective/supervision, not representation.
- If M70C improves human ranking but physical cost explodes, keep it as a
  teacher for compression, not a product model.
- If M70 cannot beat the current M60/M49 line after broad data and true
  final-ranking training, stop the dense-removal product line and keep SAE as a
  teacher-path research prototype.
- Do not ship SQL/API productization from this line until the model gate passes.

## Immediate Implementation Steps

1. Freeze the current M60 commit/tag as the rollback milestone. Done:
   `milestone-m60-scaled-supervised`.
2. Build `m70_source_registry_v2` with corpus, qrel, split, and leakage
   metadata. Initial version implemented in
   `scripts/research_sae_m70_source_registry.py`.
3. Implement the M70 differentiable sparse retrieval trainer:
   deep asymmetric encoder, straight-through top-k atom layer, final
   BM25+SAE ranking loss, complementarity loss, and physical-cost loss. Initial
   local trainer implemented in `scripts/research_sae_m70_e2e_deep_train.py`.
4. Run small correctness and leakage tests locally. Initial local smoke is
   recorded in `sae-m70-end-to-end-deep-training-results-report.md`.
5. Create data-prep jobs for large corpus text and supervised query sources.
6. Build candidate caches for BM25, dense teacher, current SAE teacher, and
   current model misses.
7. Launch scaled M70B representation pretraining on Spark.
8. Launch M70C supervised final-ranking training only after M70B passes the
   teacher-shape and fanout gates.

## Current Recommendation

Training still has value, but not as another M60-style query-cap or loss-weight
sweep. The only training line worth pursuing is this all-in reset: broader
data, stronger encoder, differentiable hard-sparse retrieval, and final
BM25+SAE ranking supervision. If that line fails, it will be a meaningful
negative result; the current small-patch line has already produced enough
evidence that it is unlikely to cross the product gate.
