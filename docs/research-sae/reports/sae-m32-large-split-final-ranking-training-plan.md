# SAE M32 Large-Split Final-Ranking Training Plan

Status: planned next training phase.

M29/M30/M31 closed several small-artifact branches:

- query-prefix mismatch is not the blocker;
- scalar calibration is not enough;
- final `BM25 + SAE` ranking supervision has signal;
- unfreezing the query encoder under BM25-preservation reduces
  `dbpedia-entity` and `msmarco` collapse;
- `trec-covid` remains the hard failure;
- the current 1,342-query prepared artifact is too small and too
  test-like to close the product gate.

M32 therefore changes the training question from "which small loss/weight
works on the current full15 artifact?" to "can final-ranking training
generalize when trained on official BEIR train/dev queries and evaluated on a
held-out regression set?"

## Current Data Boundary

The current shared full15 artifact is not full BEIR:

| Scope | Documents | Queries | Qrel pairs |
| --- | ---: | ---: | ---: |
| Current prepared full15 artifact | 49,059 | 1,342 | 39,742 |
| Local official BEIR15 files | 33,860,495 | 778,054 | not fully counted |
| Current prepared share | 0.14% docs | 0.17% queries | sampled/evaluation subset |

This artifact is still useful as a fixed regression/evaluation surface, but it
should no longer be the main training data for model-gate decisions.

## Official Train/Dev Availability

The local official BEIR cache has train/dev qrels for only part of the 15
datasets:

| Dataset | Train qrel queries | Dev qrel queries | Test qrel queries | M32 role |
| --- | ---: | ---: | ---: | --- |
| `arguana` | 0 | 0 | 1406 | holdout/regression only |
| `climate-fever` | 0 | 0 | 1535 | holdout/regression only |
| `cqadupstack` | 0 | 0 | 13145 | holdout/regression only |
| `dbpedia-entity` | 0 | 67 | 400 | small dev train + test holdout |
| `fever` | 109810 | 6666 | 6666 | primary train/dev |
| `fiqa` | 5500 | 500 | 648 | primary train/dev |
| `hotpotqa` | 85000 | 5447 | 7405 | primary train/dev |
| `msmarco` | 502939 | 6980 | 43 | primary train/dev, tiny test caution |
| `nfcorpus` | 2590 | 324 | 323 | primary train/dev |
| `nq` | 0 | 0 | 3452 | holdout/regression only |
| `quora` | 0 | 5000 | 10000 | dev train + test holdout |
| `scidocs` | 0 | 0 | 1000 | holdout/regression only |
| `scifact` | 809 | 0 | 300 | small train + test holdout |
| `trec-covid` | 0 | 0 | 50 | hard holdout only |
| `webis-touche2020` | 0 | 0 | 49 | hard holdout only |

M32 must not use official test queries for training. Datasets without
train/dev remain holdout-only unless a separate pseudo-query protocol is
designed and marked as non-qrel evidence.

## Core Hypothesis

The next promising direction is a joint objective:

```text
query text
  -> query SAE atoms
  -> runtime-safe BM25/SAE/residual controls
  -> final BM25+SAE sparse ranking
  -> teacher anchor + qrel ranking + BM25-preservation + fanout cost
```

The encoder should not merely imitate teacher atoms, and it should not let
qrels rewrite the teacher distribution globally. Teacher and qrels have
different roles:

- Teacher anchors the semantic distribution and prevents qrel overfit.
- Qrels optimize final ranking where they are consistent with robust evidence.
- BM25-preservation prevents lexical positives from being suppressed.
- Fanout/posting cost keeps the unified sparse engine physically viable.

## Workstream M32.0: Build Clean Train/Eval Artifacts

Create a new artifact builder instead of reusing the current 1,342-query
evaluation artifact as training data.

Requirements:

- Inputs: official BEIR train/dev/test files under the local cache.
- Training queries: train/dev qrels where available.
- Regression queries: current prepared full15 query set plus official test
  queries, with explicit query-id exclusion from training.
- Corpus context: build at least a `20k/q300`-style corpus/candidate artifact
  per dataset where feasible.
- Positive preservation: always include qrel-positive documents for selected
  training/eval queries.
- Hard negatives: include BM25 top-k, teacher top-k, current student misses,
  and SAE near-misses.
- Output metadata: record source split, sampled docs, qrel count, query-id
  exclusions, corpus sampling seed, and quality-claim eligibility.

Initial target sizes:

| Artifact | Purpose | Target |
| --- | --- | --- |
| `m32-train-20k-q300` | main training | up to 20k docs and 300 train/dev queries per dataset |
| `m32-regression-current` | apples-to-apples continuity | current 49,059-doc / 1,342-query artifact |
| `m32-test-official` | final holdout where feasible | official test queries, no training contamination |

If a dataset has too many train queries, sample stratified by query length,
BM25 concentration, qrel count, and teacher/BM25 disagreement. If a dataset has
too few train/dev queries, do not oversell it as training evidence.

## Workstream M32.1: Teacher And Candidate Preparation

For every M32 artifact:

- build or reuse Snowflake prefixed query teacher latents;
- build teacher document latents for the selected corpus;
- build BM25 postings and SAE postings;
- compute teacher rankings, BM25 rankings, and current text-student rankings;
- compute candidate union with qrels positives, BM25 hard negatives, teacher
  near-misses, SAE near-misses, and current-student misses;
- record candidate coverage for qrel positives and teacher top-k.

Acceptance before training:

- qrel positives are present in the candidate set at a known coverage rate;
- candidate fanout/posting statistics are stable across datasets;
- no official test query ID appears in the training artifact.

## Workstream M32.2: Final-Ranking Model Training

Start from M31, but train on the M32 train artifact.

Primary model:

- initialize query encoder from M29 Arm A;
- unfreeze query encoder with low learning rate;
- train the joint BM25/SAE/residual head;
- keep document atoms fixed to teacher doc atoms for this phase;
- no doc-side training until query-side gate passes.

Primary loss:

| Component | Purpose |
| --- | --- |
| teacher listwise KL | keep semantic teacher calibration |
| qrel NDCG/pairwise residual | improve final ranking without target rewriting |
| BM25-preservation | stop suppressing BM25-supported positives |
| collapse/DRO penalty | penalize worst dataset-family degradation |
| fanout/posting penalty | keep query-time cost below teacher path |
| atom support/value auxiliary | prevent the encoder from drifting into uninterpretable atoms |

M32 should not do a broad weight sweep. It should test a small set of
well-motivated variants:

| Variant | Change | Reason |
| --- | --- | --- |
| `m32-primary` | M31 final-ranking objective on M32 train split | main hypothesis |
| `m32-bm25-preserve-strong` | stronger lexical-positive preservation | directly targets `trec-covid` |
| `m32-dataset-dro` | worst-family/dropout-style objective | prevents full15 mean from hiding collapse |
| `m32-cost-balanced` | stronger fanout/posting penalty | checks quality/cost Pareto |

## Workstream M32.3: Evaluation Gates

Evaluate each model on separate surfaces:

| Surface | Meaning |
| --- | --- |
| current prepared full15 artifact | continuity with M27-M31 |
| official test splits | cleaner held-out BEIR evidence |
| dataset-family holdout | robustness against in-domain overfit |
| hard datasets | focused `trec-covid`, `msmarco`, `dbpedia-entity` analysis |
| physical harness | candidate docs, BM25 postings, SAE postings, payload size, latency where available |

M32 pass gate:

| Gate | Requirement |
| --- | --- |
| Aggregate quality | no worse than M29 strong aggregate by more than small tolerance |
| Teacher gap | Recall@100, MRR@20, NDCG@10, MAP@100 stay within M27 product-research bounds |
| Collapse | no dataset NDCG/MAP delta below `-0.025` versus teacher on regression or official holdout |
| `trec-covid` | must materially improve MAP/NDCG without hiding the issue through aggregate averages |
| Cost | candidate docs and SAE postings must not materially exceed current EATMH/M29 profile unless quality gains create a clear Pareto frontier |
| Leakage | no official test query in training data |

If aggregate quality improves but collapse remains, M32 fails. If collapse
improves only by becoming BM25-like and losing semantic recall, M32 also fails.

## Workstream M32.4: Decision Rules

Promote to doc-side training only if query-side fixed-doc passes the gate.

If M32 query-side passes:

- train document-side text-to-atoms using the same artifact and loss family;
- rerun full text-to-atoms evaluation;
- then reopen read-only SQL/runtime engineering around the promoted model.

If M32 improves `msmarco/dbpedia` but not `trec-covid`:

- keep final-ranking objective;
- add a targeted biomedical/claim-heavy training or proxy-data track;
- do not proceed to doc-side training.

If M32 does not improve over M31:

- stop small Arm-A lineage tuning;
- consider M33 stronger query representation, but only under the same clean
  split and final-ranking objective.

## Implementation Order

1. Add `research_sae_m32_build_split_artifacts.py`.
2. Build `m32-train-20k-q300` and `m32-regression-current` metadata.
3. Add leakage checks and candidate coverage reports.
4. Extend M31 trainer to accept M32 train/eval roots separately.
5. Run `m32-primary` on the train artifact and evaluate on current regression.
6. Run official-test evaluation where qrels are available.
7. Run one BM25-preservation-strong variant if `trec-covid` remains the only
   major collapse.
8. Produce `sae-m32-large-split-final-ranking-results-report.md`.

## Expected Outcome

M32 is not guaranteed to pass. Its purpose is to answer the most important open
question cleanly:

```text
Does final-ranking text-to-atoms training fail because the objective is wrong,
or because the previous 1,342-query artifact was too small and too
test-contaminated to learn robust query-family behavior?
```

Only after M32 should the project decide whether to keep pushing this Arm-A
lineage, move to a stronger query encoder, or stop dense-removal product work.
