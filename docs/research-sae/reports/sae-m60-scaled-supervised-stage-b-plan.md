# SAE M60 Scaled Supervised Stage-B Plan

Date: 2026-05-21

## Summary

M60 scales Stage-B in both directions:

```text
more supervised query training
+ more held-out supervised evaluation
+ explicit source/family splits
```

The goal is not to add more BEIR qrels to the same loop. M57-M59 already show
that naive full15 qrel expansion on the current small encoder/objective does
not close the teacher gap. M60 instead builds a larger supervised retrieval
surface from BEIR and non-BEIR labeled sources, with train/test boundaries
defined per source before training starts.

The model target remains:

```text
text -> sparse atoms -> unified BM25 + SAE sparse evidence engine
```

M60 is still model research. It does not freeze SQL/API behavior and does not
claim dense-removal readiness.

## Core Principle

Training and evaluation must scale together.

If a new labeled query collection is used for supervision, it must also
contribute one of:

- an official held-out split;
- a source-native dev/test split;
- a query-level cross-validation fold;
- a family holdout group.

No source may be used only as training data without a corresponding leakage
check and held-out measurement. Conversely, no new evaluation source should be
added without deciding whether its train portion is allowed to teach the model.

## Source Registry

M60 starts by normalizing every source into a registry row:

| Field | Meaning |
| --- | --- |
| `source_id` | Stable source name, for example `msmarco_passage` |
| `family` | Retrieval family: web, QA, biomedical, entity, long-tail, scientific |
| `label_type` | `human_qrel`, `teacher_pseudo`, `proxy_qrel`, or `unlabeled` |
| `quality_claim_allowed` | Whether metrics from this source can support quality claims |
| `split_policy` | Official split, query-level folds, or train-only diagnostic |
| `train_query_ids` | Explicit train query set |
| `eval_query_ids` | Explicit eval query set |
| `leakage_keys` | Query IDs, text hashes, corpus IDs used for leakage checks |
| `weight_cap` | Maximum share of Stage-B update steps |

Every downstream artifact must carry this metadata.

## Candidate Supervised Sources

| Source | Training Use | Evaluation Use | Notes |
| --- | --- | --- | --- |
| BEIR15 official train/dev/test | Broad regression backbone | Full15 and official-test holdout | Keep current full15 regression for continuity |
| MS MARCO passage ranking | Web-query supervision and hard negatives | MS MARCO dev and TREC DL bridge | Do not let volume dominate batches |
| TREC Deep Learning | High-quality judged web ranking | Held-out TREC DL years/folds | Prefer as high-quality eval/control |
| Natural Questions retrieval | Wikipedia QA supervision | Official dev/test or query folds | Useful for semantic QA-style retrieval |
| LoTTE | Long-tail domain robustness | Domain holdout | Good for topic/domain generalization |
| TREC-COVID / CORD-19 | Biomedical broad-query supervision | Query-level folds only if no clean train/test | Must avoid using official held-out labels as train |
| arXiv/PubMed/commons | Distribution/fanout/proxy diagnostics | Efficiency and proxy-only quality | Quality claim requires qrels or defensible proxy-qrels |

M60 should prioritize labeled human qrels first. Teacher-generated labels are
allowed only as auxiliary supervision and must not be reported as human-quality
evidence.

## Split Rules

1. Official train/dev/test splits are preserved when available.
2. Sources without official splits use deterministic query-level folds.
3. Query text hashes are checked across train/eval to catch duplicate leakage.
4. Corpus documents may overlap when the benchmark design expects it, but qrel
   labels for held-out queries must not be used for training.
5. Proxy labels are never mixed into human-qrel evaluation metrics.

For small sources such as TREC-COVID, M60 should prefer cross-validation
reporting:

```text
fold_0 train queries -> fold_0 eval queries
fold_1 train queries -> fold_1 eval queries
...
aggregate folds as cross-validation, not product holdout
```

## Artifact Layout

Each normalized source emits a BEIR-style artifact:

```text
documents.jsonl
queries.jsonl
qrels.jsonl
quality_qrels.json
manifest.json
source_registry.json
split_manifest.json
```

Then materialization produces:

```text
shared_sae_8192_64/document_latents.jsonl
shared_sae_8192_64/query_latents.jsonl
bm25_candidates.jsonl
teacher_candidates.jsonl
student_miss_candidates.jsonl
candidate_coverage.json
```

The artifact must record:

- qrel-positive coverage;
- teacher top-20/top-100 coverage;
- BM25 hard-negative coverage;
- SAE near-miss coverage;
- candidate docs and posting fanout estimates.

## Training Design

M60 should keep the fixed-doc query-side harness first. Document atoms remain
teacher document atoms until the query-side gate improves.

### Stage B0: Query Teacher-Shape Warmup

Train query text to teacher query atoms on all supervised-source queries before
ranking fine-tuning.

Primary losses:

- support top-k recall;
- active-atom value calibration;
- teacher-neighborhood KL;
- fanout-aware active atom budget.

Purpose: preserve teacher semantic shape before qrels reshape ranking.

### Stage B1: Supervised Ranking Residual

Use human qrels as residual constraints over the teacher/BM25 candidate pool.

Candidate union must include:

- qrel positives;
- BM25 hard negatives;
- teacher top-k and near-misses;
- SAE near-misses;
- current student misses.

Primary losses:

| Loss | Role |
| --- | --- |
| teacher listwise KL | Preserve semantic teacher calibration |
| qrel NDCG/pairwise residual | Improve human relevance ranking |
| BM25 preservation | Prevent lexical positives from being suppressed |
| source/family DRO | Penalize worst-family collapse |
| fanout/posting penalty | Keep query-time cost bounded |
| support/value auxiliary | Keep atoms interpretable and teacher-shaped |

Qrels must not globally rewrite the teacher target. M29 showed that strong
qrel target injection can improve aggregate metrics while causing dataset
collapse.

## Sampling And Scale

Use source-balanced sampling, not raw qrel-pair count.

Initial update-step caps:

| Group | Max Step Share |
| --- | ---: |
| Any single dataset | 15% |
| Biomedical sources combined | 25% |
| MS MARCO + TREC DL web ranking | 25% |
| BEIR mixed sources | 30% |
| QA sources such as NQ | 20% |
| Long-tail/domain sources such as LoTTE | 20% |
| Teacher-imitation replay during Stage B | 20% |

Training scale should grow in tiers:

| Tier | Supervised Queries | Purpose |
| --- | ---: | --- |
| M60A source smoke | 5k-10k | Validate adapters, splits, leakage checks, candidate coverage |
| M60B medium | 50k-100k | First meaningful multi-source Stage-B run |
| M60C large | 100k-250k+ | Only if M60B improves held-out robustness |

The goal is not to maximize query count immediately. The first gate is whether
the added source diversity improves held-out hard-family behavior.

## Evaluation Design

M60 evaluation must include:

1. Current full15 regression surface for continuity.
2. Official BEIR train/dev/test holdouts where available.
3. MS MARCO/TREC DL heldout.
4. NQ heldout or deterministic query folds.
5. LoTTE domain holdout.
6. TREC-COVID/CORD-19 query-fold validation if no clean train/test split exists.
7. Real arXiv/PubMed/commons efficiency matrix, with quality metrics only when
   qrels or defensible proxy-qrels exist.

Report metrics:

- Recall@20/100;
- MRR@20;
- NDCG@10;
- MAP@100;
- candidate docs;
- BM25 postings;
- SAE postings;
- rerank terms;
- payload MB;
- source/family collapse table.

The primary score is not a single global mean. M60 must report:

- macro average by source family;
- full15 continuity mean;
- hard-family minimum delta versus teacher;
- source holdout deltas;
- physical cost frontier.

## Promotion Gate

M60 can advance only if it improves robustness, not just aggregate quality.

Minimum gate:

- full15 query-side fixed-doc teacher gap improves versus M58/M59;
- no major regression versus BM25 on hard datasets;
- hard-family deltas improve on `trec-covid`, `msmarco`, and `dbpedia-entity`;
- at least one external heldout source improves without source-specific tuning;
- SAE postings and candidate docs stay within the current teacher-path cost
  envelope or form a clear quality/cost Pareto frontier.

Strong gate:

- full15 fixed-doc teacher gap approaches product-research pass;
- source-family holdout shows no collapse;
- hard biomedical/web/entity sources improve together;
- query-side model is good enough to justify doc-side text-to-atoms training.

## Immediate Implementation Steps

1. Add a source registry builder that scans local/Hugging Face/cache locations
   for BEIR, MS MARCO, TREC DL, NQ, LoTTE, and TREC-COVID/CORD-19.
2. Add adapters that emit the normalized BEIR-style artifact layout.
3. Add deterministic query-level split/fold generation and leakage reports.
4. Materialize teacher atoms and candidates for M60A source smoke.
5. Run fixed-doc query-side Stage-B0/B1 on M60A.
6. Promote to M60B only if M60A improves at least one held-out hard-family
   signal without increasing collapse elsewhere.

## Decision

M60 reopens Stage-B data scaling, but with stricter structure:

```text
scale supervised queries
+ scale held-out evaluation sources
+ preserve train/eval boundaries
+ keep teacher-shape first
+ use qrels as residual relevance constraints
```

This is the correct next direction before product engineering resumes.
