# SAE Milestone 26 SoftSAE Adaptive Sparsity Review

Date: 2026-05-17
Branch checkpoint: `sae` at `5b2c94b`

## Scope

This milestone records the post-M24 project state and reviews
`arXiv:2605.06610`, `SoftSAE: Dynamic Top-K Selection for Adaptive Sparse
Autoencoders`, against the current BM25+SAE unified sparse retrieval direction.

The goal is not to restart broad training immediately. The goal is to decide
whether the paper changes our next step, and to preserve the current milestone
state before moving forward.

## Current Project State

M24 closed the high-value larger-teacher text-to-atoms check:

```text
shared_sae_16384_64 student: cost-only, not promoted
shared_sae_12288_64 student: cost-only, not promoted
shared_sae_16384_64 h384 retry: cost-only, not promoted
```

The strongest larger-teacher students reduced candidate and posting cost, but
lost too much ranking quality versus the current text-student baseline:

```text
best M24 Recall@100 delta ~= -0.048
best M24 MAP@100 delta    ~= -0.069
```

The training conclusion remains:

```text
freeze 12288/16384 direct text-student loss tuning
do not claim dense-removal readiness
move to M27 exploration closure before engineering
```

The active physical-engine direction remains:

```text
BM25 token atoms
+ SAE latent atoms
  -> one evidence-atom namespace
  -> impact-head candidate generation
  -> exact doc-row rerank
```

M21 runtime-query evidence-atom SQL has passed as the evaluation harness:
external query atom ids and weights, Python/C/PostgreSQL parity, and an
experimental read-only SQL surface. `EATMH001/EATMH002` is the source-blind
payload family for M27 cost measurement; `UBMXM001` remains a source-aware
embedded-query research/control payload.

## Paper Summary

SoftSAE replaces a fixed TopK SAE budget with an input-dependent active feature
budget. A Dynamic Sparsity MLP predicts `k_hat`, and training uses a
differentiable Soft Top-K operator so gradients can flow through the selection
mechanism. Late training and inference use hard TopK again, so the final
representation is still exactly sparse.

The most relevant mechanism is:

```text
x -> encoder activations z
x -> Dynamic Sparsity MLP -> k_hat
training:  soft_topk(z, k_hat, alpha) * z
inference: hard_topk(z, round(k_hat))
```

The objective combines:

- reconstruction loss;
- a Softplus-style mean sparsity budget penalty;
- an auxiliary dead-feature loss;
- annealing for target sparsity and Soft Top-K sharpness;
- a final hard-TopK phase to close the soft-to-hard mismatch.

The paper evaluates CLIP embeddings and Gemma-2-2B activations. It reports
competitive reconstruction/sparsity tradeoffs and better feature-quality
diagnostics such as absorption, splitting, SCR, and TPP. It is important that
this is not a retrieval paper: it does not optimize Recall/MRR/NDCG/MAP and
does not validate sparse inverted-index behavior.

The paper also documents a relevant cost warning: Soft Top-K increases training
cost and becomes a bottleneck for large dictionaries. That makes it suitable as
a training-time tool, not as a PostgreSQL query-time operator.

Primary references:

- Paper: <https://arxiv.org/abs/2605.06610>
- Official code: <https://github.com/St0pien/SoftSAE>

## Relevance To Our Direction

SoftSAE is relevant because our current system still uses fixed deployment
budgets:

```text
query active budget: fixed
document active budget: doc80/doc88/doc96/doc128 style clipping
impact-head width: fixed per run
rerank doc-row width: fixed per payload profile
```

The paper gives a stronger reason to avoid one global budget. Retrieval inputs
also have different local complexity:

- entity-like short queries often need few atoms and should avoid noisy
  semantic expansion;
- multi-concept RAG questions may need more atoms to avoid semantic coverage
  loss;
- simple documents can be represented compactly;
- dense scientific abstracts or policy documents may need larger doc budgets.

This maps directly to our current failure modes:

- `doc80/doc88/doc96` are cost knobs, but they are global and post-hoc;
- text-student distillation lost candidate coverage because it learned a cheap
  representation, not an adaptive one;
- larger teachers were strong, but fixed student export budgets did not keep
  enough semantic detail where needed;
- impact-head traversal wants a query-dependent fanout control, not a universal
  candidate window.

The key idea to absorb is therefore not SoftSAE as-is. The useful idea is:

```text
learn or estimate per-query and per-document sparse capacity,
then feed that capacity into the unified evidence-atom engine.
```

## What Is Not Suitable For This Project

SoftSAE should not be imported directly into the query path:

- Soft Top-K is a training-time differentiable approximation and is too heavy
  for PostgreSQL query execution.
- The paper optimizes reconstruction and interpretability metrics, not
  retrieval quality.
- Its Dynamic Sparsity MLP consumes dense activations, so it does not by itself
  remove the need for an upstream dense encoder.
- The reported experiments are CLIP/Gemma interpretability workloads, not BEIR
  retrieval or our arxiv/pubmed/commons RAG workloads.

The paper should influence model and budget design, not replace the current
read-only engineering plan.

## Proposed Absorption Plan

### M26a: Post-Hoc Dynamic Budget Simulation

Run this before heavy training.

Implementation entrypoint:

```text
scripts/research_sae_m26_dynamic_budget_sim.py
```

Use existing full15 artifacts and simulate dynamic budgets without changing the
SAE dictionary:

```text
query budget candidates: 32/48/64/80/96/128
doc budget candidates: 64/80/88/96/128/160
head candidates: 8/12/16/24
```

Candidate budget signals:

- query length and token entropy;
- BM25 score concentration;
- SAE atom activation entropy;
- top atom score mass ratio;
- teacher top-k coverage slope;
- predicted candidate fanout from atom DF/posting statistics.

Report the same full15 metrics as before:

```text
Recall@20
Recall@100
MRR@20
NDCG@10
MAP@100
candidate docs
SAE postings
rerank terms
payload MB
```

Promotion gate:

```text
match or beat doc128 quality
reduce average postings or rerank terms
no per-dataset collapse
```

### M26b: Retrieval-Aware Budget Predictor

If M26a shows a Pareto signal, train only the budget predictor first. Do not
train a new dictionary.

Possible targets:

- oracle cheapest budget preserving teacher top-k coverage;
- qrel-positive coverage at top-k;
- dense-teacher neighborhood coverage;
- rerank quality under a candidate/posting penalty.

This can use Snowflake+SAE teacher atoms as fixed features. The predictor may
consume text features, dense embeddings, or precomputed atom statistics during
research. Productization can later decide whether the dense input is acceptable.

### M26c: Retrieval-Aware SoftSAE Teacher

Only run this if M26a/M26b are positive.

Train an adaptive-k SAE over Snowflake activations, but change the objective
from pure reconstruction to retrieval-aware utility:

```text
reconstruction
+ teacher-neighborhood coverage
+ qrel-positive coverage where available
- posting fanout / active atom cost
- low-utility exposure
```

Inference must export hard sparse atoms. The query path must never require
Soft Top-K.

### M26d: Runtime Budget In SQL Diagnostics

Even before training, M21 runtime-query evidence-atom SQL should reserve
diagnostics for adaptive budgets:

```text
query_active_atoms
query_budget
doc_budget
head_budget
candidate_docs
posting_reads
rerank_terms
budget_source
```

This keeps the evaluation harness compatible with adaptive capacity if
M26a/M26b work.

## Updated Direction

The immediate mainline is M27 exploration closure:

```text
text-to-atoms final push
SoftSAE learned selector
M22 concept-vocabulary closure
M21 runtime-query evidence-atom SQL as evaluation harness
M20b qrels/proxy-qrels for quality validation
```

SoftSAE opens a model-side side track:

```text
M26 adaptive sparse capacity
```

This side track started with cheap simulation, not another expensive
end-to-end student training run. The first simulation runner is implemented and
now has full15 results.

## M26a Full15 Result

Command:

```bash
PYTHONPATH=scripts python3 \
  scripts/research_sae_m26_dynamic_budget_sim.py \
  --output-dir /Volumes/Betty/Tmp/ii42_sae_reports/m26-dynamic-budget-full15
```

Result:

```text
dynamic budget simulation completed on all 15 BEIR datasets
```

Mean quality and cost:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Query budget | Doc budget | Head | Postings | Candidates | Rerank terms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `adaptive_complexity` | 0.7987 | 0.8069 | 0.7068 | 0.6704 | 96.0000 | 64.0000 | 16.0000 | 985.1131 | 721.2118 | 46,157.5559 |
| `adaptive_fanout` | 0.8009 | 0.8235 | 0.7244 | 0.6638 | 75.9824 | 83.4721 | 11.9171 | 624.5487 | 483.5170 | 39,447.6629 |
| `fixed_q32_d80_h8` | 0.7520 | 0.8021 | 0.6985 | 0.6253 | 32.0000 | 80.0000 | 8.0000 | 236.4654 | 201.9434 | 16,042.7225 |
| `fixed_q64_d96_h12` | 0.8197 | 0.8289 | 0.7352 | 0.6909 | 64.0000 | 96.0000 | 12.0000 | 667.9452 | 523.8214 | 48,782.0034 |
| `fixed_q96_d128_h16` | 0.8334 | 0.8387 | 0.7467 | 0.7110 | 96.0000 | 128.0000 | 16.0000 | 985.1131 | 721.2118 | 81,994.6103 |

Decision:

```text
do not promote adaptive budget as the mainline yet
```

`adaptive_fanout` is a useful cost signal, but it loses too much ranking
quality versus `fixed_q96_d128_h16`: about -0.0325 Recall@100, -0.0152
MRR@20, -0.0223 NDCG@10, and -0.0472 MAP@100. It does reduce postings by
about 36.6%, candidates by about 33.0%, and rerank terms by about 51.9%, so
the idea remains useful as a learned policy target.

The cleaner near-term evaluation profile is `fixed_q64_d96_h12`: it preserves
more quality than the current adaptive policies while still cutting postings,
candidates, and rerank terms substantially. The next M26 work inside M27 should
therefore not be expensive adaptive-k SAE training. It should first improve the
budget selector by learning when to choose between fixed low/medium/high
profiles.

## Bottom Line

SoftSAE does not overturn the current decision to stop direct larger-teacher
text-student tuning. It does add one useful next research question:

```text
Can adaptive per-query/per-document active atom budgets preserve semantic
coverage while reducing posting fanout and rerank cost?
```

That question is directly aligned with the unified sparse index goal and should
continue as a side track, but the full15 simulation says the first adaptive
heuristics are not yet better than fixed profiles. M27 is therefore the
mainline, while M26 should focus on a learned budget selector before any new
adaptive-k SAE training.
