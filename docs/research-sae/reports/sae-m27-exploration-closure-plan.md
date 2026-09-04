# SAE M27 Exploration Closure Plan

Date: 2026-05-17

## Summary

M27 pauses product engineering and turns the next phase into an exploration
closure campaign. The goal is to finish the remaining SAE, BM25+SAE, SoftSAE,
concept-vocabulary, and text-to-atoms routes before starting real SQL/API or
mutable-index productization.

The target product shape remains:

```text
text -> sparse atoms -> unified evidence-atom index
```

The serving path must eventually avoid query-time dense embedding dependency.
The current teacher path is strong, but the direct text-to-atoms student has
not yet closed the quality gap enough to claim dense removal.

Until M27 closes:

```text
do not freeze SQL/API
do not design mutable maintenance
do not claim dense retrieval can be removed
use EATMH/M21 only as the evaluation harness
```

## Current Gap

The closest product-relevant comparison is:

| Path | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `teacher_16384` | 0.8455 | 0.8462 | 0.7530 | 0.7273 |
| `baseline_budget16 text-student` | 0.8324 | 0.8291 | 0.7225 | 0.6913 |
| Gap | -0.0131 | -0.0171 | -0.0305 | -0.0360 |

This means text-to-atoms is not far from the teacher on Recall@100, but it is
still behind on first-page ranking and score quality. M27 must focus on
MRR/NDCG/MAP, not only recall.

## Closure Matrix

| Track | Status | Evidence | M27 decision |
| --- | --- | --- | --- |
| Text-to-atoms student | `open-final-push` | Recall close to teacher; NDCG/MAP still behind | Main blocker; run one final ranking-focused push |
| SoftSAE / adaptive budget | `open-final-push` | M26a cost-positive but not quality-promotable | Build learned selector; park if it cannot preserve quality |
| M22 concept vocabulary / SAE-SPLADE | `open-final-push` | Theory aligns with final text-to-atoms goal; current SPLADE baseline not enough | Run paper-aligned concept control; park if it does not beat current student |
| Larger teacher distillation | `closed` | M24 12288/16384 students were cost-only and worse on ranking | Do not retune the same loss family |
| Unified physical engine | `promoted-harness` | M21 full15 Python/C/PostgreSQL parity passed | Keep as read-only evaluation harness, not product approval |
| EATMH002/doc128 | `promoted-harness` | Full15 parity and M20 pilot show lower rerank cost | Use as physical-cost guardrail |
| Real workload quality | `blocked` | No canonical arxiv/pubmed/policy qrels or proxy-qrels | No product Recall/MRR/NDCG/MAP claims |
| Real workload efficiency | `closed-pilot` | 5k-doc arxiv/pubmed/policy pilot passed | Scale only after M27 model direction is chosen |
| Naive SPLADE replacement | `closed` | Did not beat current SAE route in existing exploration | Reopen only through concept-vocabulary control |
| Raw larger latent/loss sweeps | `closed` | Larger-teacher direct students were cost-only | Reopen only with a new encoder architecture |
| Naive query budgets / DF stoplists / active128 | `closed` | Negative prior results or poor cost-quality tradeoff | Do not rerun |

## Text-To-Atoms Final Push

This is the primary M27 workstream. The baseline is fixed:

```text
teacher: teacher_16384 / bm25_sae
student: baseline_budget16 text-student / bm25_student_atoms
```

Do not change the promotion target by moving to easier metrics. The final
student must be judged against the same full15 quality matrix and physical
cost matrix.

Required attempts:

- ranking-aware calibration focused on MRR/NDCG/MAP;
- candidate-budget loss refinement using the same physical budget exposed by
  the evidence-atom engine;
- hard-negative refresh with qrels positives, BM25 hard negatives, dense
  near-misses, and SAE near-misses;
- query/document asymmetric active budgets;
- stronger pretrained text encoder or SPLADE-style encoder control;
- LODO or dataset-family holdout to reject in-domain-only tuning.

Do not rerun:

- raw 12288/16384 larger-teacher loss sweeps;
- naive listwise loss that already failed to improve the frontier;
- `token_max`;
- naive SAE DF stoplists;
- active128 as a standalone fix;
- simple global fusion-weight sweeps.

Promotion gates:

| Gate | Requirement |
| --- | --- |
| Strong pass | Recall gap <= 0.005, MRR gap <= 0.010, NDCG/MAP gap <= 0.015 |
| Product-research pass | Recall gap <= 0.010, MRR gap <= 0.015, NDCG/MAP gap <= 0.025 |
| Cost gate | Candidate docs, SAE postings, and payload MB must not exceed the current EATMH002 profile by a material amount |
| Robustness gate | full15 has no dataset collapse and LODO does not require in-domain tuning |

If the product-research pass is reached, M27 can reopen read-only engineering.
If it is not reached, record:

```text
dense-removal not ready
```

## SoftSAE Closure

M26a showed that adaptive budgets can cut cost but currently lose too much
quality. M27 therefore runs only the learned-selector step before considering
any expensive adaptive-k SAE training.

Selector policy:

- choose among fixed low/medium/high profiles, especially `fixed_q64_d96_h12`
  and `fixed_q96_d128_h16`;
- do not change scoring or invent a new retrieval objective in this step;
- use only runtime-safe signals: query length, BM25 concentration, atom
  entropy, top atom mass, predicted fanout, and coverage slope.

Pass gates:

| Gate | Requirement |
| --- | --- |
| Quality | close to `fixed_q96_d128_h16` without clear Recall/MRR/NDCG/MAP regression |
| Cost | at least 20% lower postings or rerank terms versus fixed high |
| Generalization | LODO selector does not collapse |
| Decision | if these fail, park SoftSAE and do not run adaptive-k SAE training |

Only if the learned selector passes should M27 consider retrieval-aware
adaptive-k SAE training.

## Concept Vocabulary Closure

M22 remains open because the concept-vocabulary direction matches the final
serving goal more closely than late fusion:

```text
text encoder -> SAE concept vocabulary -> sparse atoms
```

The closure experiment must be paper-aligned, not a generic SPLADE baseline.
It must compare:

```text
BM25
Snowflake-SAE teacher
current text-student atoms
concept-vocabulary student
```

The matrix must include:

```text
Recall@20
Recall@100
MRR@20
NDCG@10
MAP@100
QD-FLOPs-style cost
posting reads
candidate docs
rerank terms
payload MB
```

Park this route if it cannot exceed the current text-student on NDCG/MAP or
cannot materially improve the cost frontier.

## Evaluation And Acceptance

All M27 workstreams must use the same acceptance framework:

- full15 quality matrix: Recall@20, Recall@100, MRR@20, NDCG@10, MAP@100;
- full15 physical matrix: candidate docs, SAE postings, BM25 postings, rerank
  terms, payload MB;
- native/runtime checks: C reader parity, M21 PostgreSQL by-id parity, and
  EATMH002/doc128 no-regression;
- generalization checks: LODO or dataset-family holdout;
- real corpus checks: arxiv/pubmed/policy efficiency only unless qrels or
  defensible proxy-qrels exist;
- closure report: every track must end as `promoted`, `closed`, `parked`, or
  `blocked`, with evidence.

## Final M27 Exit Decision

M27 ends in exactly one of three states:

| State | Meaning | Next phase |
| --- | --- | --- |
| `text-to-atoms promoted` | Product-research pass or stronger achieved | Resume read-only engineering with no query-time dense dependency claim staged for validation |
| `teacher-only harness promoted` | Student fails but teacher path remains useful | Continue research prototype; no dense-removal product claim |
| `dense-removal not ready` | Student and concept routes fail closure | Stop product engineering for this line and archive the closure report |

The default expectation is not automatic productization. The default is to
finish the model evidence first, then decide.

## Execution Update: 2026-05-17

The M27 closure harness is now implemented and executed:

```text
scripts/research_sae_m27_exploration_closure.py
sae-m27-exploration-closure-report.md
results/sae/m27/exploration-closure/m27_exploration_closure.json
/Volumes/Betty/Tmp/ii42_sae_reports/m27-exploration-closure
```

The runner scans existing full15 text-student matrices, runs the SoftSAE
learned-selector closure using the same `evaluate_run` metric surface as the
previous full15 harnesses, and folds the M22 concept-control evidence into one
closure decision.

Final M27 result:

```text
dense-removal not ready
```

Evidence:

| Track | Result | Evidence |
| --- | --- | --- |
| Text-to-atoms | `open-final-push-failed` | best full15 student remains `baseline_budget16`; gate still fails against `teacher_16384` |
| SoftSAE selector | `parked` | learned rule `coverage_slope>=7.25` preserves quality but drops postings by only `3.27%`, below the `20%` gate |
| Concept vocabulary | `parked` | existing SPLADE/concept control does not beat current text-student ranking quality |

This closes M27 as a product-model negative result. The read-only physical
engine remains useful as a research harness, but the project should not claim
query-time dense dependency removal from the current direct text-to-atoms
student.
