# SAE Converged Research And Engineering Plan

Date: 2026-05-13
Last updated: 2026-05-17

Current canonical status report:

```text
docs/research-sae/reports/milestones/sae-current-status-and-engineering-readiness-report.md
sae-milestone26-softsae-adaptive-sparsity-report.md
sae-m21-runtime-evidence-atom-mainline-report.md
sae-m20-real-corpus-pilot-report.md
sae-m27-exploration-closure-plan.md
```

That report consolidates the prior phase documents into the current readiness
decision. This file remains the active milestone plan; the status report is the
best entry point for the latest direction, gates, metrics, and remaining risk.

## Current Position

The physical engine exploration has converged enough to provide a strong
read-only evaluation harness, but the model-side product target is not closed
yet. The next phase is therefore M27 exploration closure, not SQL/API
productization.

The winning abstraction remains:

```text
BM25 token atoms
+ SAE latent atoms
  -> one evidence-atom namespace
  -> impact-head candidate generation
  -> exact doc-row rerank
```

The current full15 runtime-query quality baseline is:

```text
EATMH001/head16
Recall@100 = 0.8365
MRR@20     = 0.8416
NDCG@10    = 0.7506
MAP@100    = 0.7148
candidates = 743.5798
rerank terms = 109,857.3877
```

The current read-only engineering candidate is:

```text
EATMH002/head16_doc128
Recall@100 = 0.8354
MRR@20     = 0.8414
NDCG@10    = 0.7492
MAP@100    = 0.7136
PG mean ms = 0.7460
rerank terms = 84,820.7950
```

The older five-dataset `h16_qb0_weight` and `h16_qball_doc128` rows remain
historical checkpoints, but M21 now gives the stronger full15 parity evidence.
`EATMH002/head16_doc128` is the current physical-cost guardrail and read-only
evaluation harness. It is not a productization approval until M27 closes the
text-to-atoms model gap.

## Decisions

### Keep

- Keep source-blind evidence atoms as the core physical abstraction.
- Keep `h16_qb0_weight` as the quality guardrail.
- Keep `h8_qb0_weight` as the low-cost reference, not as the default.
- Keep mixed atoms candidate-only until calibration proves they are safe for
  final scoring.
- Keep the PostgreSQL generation path read-only for now.

### Promote As Harness

- Promote top-128 compact doc-row rerank to the evaluation harness.
- Promote trace metrics to first-class gates:

```text
candidate_docs
candidate_postings
rerank_doc_terms
PG cached by-id latency
```

### Stop For Now

- Stop tuning simple global query budgets as the main selectivity path.
- Stop using DF/IDF-cost query selectors as the primary improvement route.
- Stop treating `head32` as a likely default; it is too expensive for the
  quality gain observed so far.
- Stop stable product API or mutable index design until real workload quality
  validation exists. Read-only engineering prototypes can continue behind an
  experimental contract.
- Stop treating M21 runtime SQL as the next productization step. It remains
  available only as the M27 physical-cost and parity harness.

### M27 Closure Gate

- Finish the text-to-atoms final push before claiming query-time dense
  embedding removal.
- Finish SoftSAE learned-selector closure before investing in adaptive-k SAE
  training.
- Finish M22 concept-vocabulary closure before parking or promoting that model
  family.
- Do not resume product engineering unless M27 reaches the product-research
  pass gate or explicitly records a weaker teacher-only harness decision.

### Reopen Only With New Evidence

- Reopen SPLADE/BGE-M3 sparse integration only if a stronger checkpoint or real
  workload evidence beats the current SAE route.
- Reopen mixed atoms as final-score atoms only if a calibration experiment
  preserves MRR and first-page quality.
- Reopen dense-vector fallback only if real workload recall shows evidence
  atoms cannot cover semantic recall.

## M14: Selectivity Frontier 2

Goal:

```text
Preserve h16_qb0 quality
while moving cost toward h8_qb0.
```

Target:

```text
Recall@100 >= 0.7930
MRR@20     >= 0.6750
candidate_docs <= 650
rerank_doc_terms <= 110,000
```

Work:

- Estimate atom reliability from qrel lift and dense-teacher lift.
- Use token-latent graph expansion only for candidate widening.
- Use mixed atoms only as candidate boosters.
- Compare every run against:

```text
h16_qb0_weight
h8_qb0_weight
h16_qb64_weight
h16_qball_doc128
```

Exit condition:

- If a cheaper row stays within the target quality band, promote it to the
  next candidate-generation design.
- If no row improves the frontier, keep `h16_qb0_weight` and move engineering
  effort to compact rerank.

## M15: Compact Doc-Row Rerank

Goal:

```text
Make doc128 a concrete payload and PostgreSQL read-only path.
```

Work:

- Add a compact doc-vector payload variant for top-128 doc atoms.
- Keep full doc rows available as the exact reference.
- Add C reader parity checks:

```text
full doc-row rerank
doc128 compact rerank
```

- Add PostgreSQL by-id generation functions for the compact payload.
- Repeat the five-dataset matrix to confirm the MRR/NDCG gain is stable.

Exit condition:

- `doc128` keeps Recall@100 within 0.005 of full doc rows.
- `doc128` does not regress MRR@20 across repeated runs.
- `doc128` reduces rerank terms by at least 20%.
- C and PostgreSQL paths keep deterministic parity with the Python reference.

## M16: Real Workload Quality Readiness

Goal:

```text
Make arxiv/pubmed/commons quality validation first-class when labels exist.
```

Required artifacts:

- arxiv query set plus qrels or defensible proxy qrels;
- pubmed query set plus qrels or citation/MeSH/proxy qrels;
- commons policy query set only if its workflow is representative enough;
- fixed corpus snapshot and reproducible generation scripts.

Metrics:

```text
Recall@20
Recall@100
MRR@20
NDCG@10
MAP@100
candidate_docs
rerank_doc_terms
PG cached by-id latency
```

Exit condition:

- Real workload matrix confirms the same direction as the five-dataset matrix.
- If real workload contradicts the benchmark matrix, real workload wins.

M16 is a quality-readiness gate, not an efficiency gate. If no qrels or
defensible proxy qrels exist, do not report Recall/MRR/NDCG/MAP for the real
workload. The engineering path can still measure operational cost on real
corpora, but those measurements must not be presented as quality evidence.

## M17: SQL Model Draft

Only start M17 after M14-M16 gates pass. After the M14-M17 pass, this milestone
is superseded by the narrower M18-M21 route below: read-only SQL engineering can
advance with efficiency evidence, while product-quality claims remain blocked
until M16 quality readiness is solved.

Work:

- Draft the read-only SQL model around evidence-atom generations.
- Define payload versioning:

```text
EATMH001 full doc-row reference
EATMH002 compact doc-row candidate, if M15 passes
```

- Define diagnostics required by upper layers:

```text
candidate_docs
rerank_doc_terms
selected_atoms
payload_kind
payload_revision
cache_hit state
```

- Keep mutable maintenance out of scope until read-only semantics and metrics
  are stable.

Exit condition:

- SQL draft has one stable read-only shape and does not encode temporary
  research-only knobs as public API.

## Productization Gate

Do not productize quality claims or a stable public API until all are true:

- `h16` or a better selectivity frontier remains above Recall@100 0.7930.
- Compact rerank has stable repeated-run quality.
- Real workload qrels/proxy-qrels exist and confirm the route.
- PostgreSQL cached by-id latency remains within the target envelope.
- Diagnostics are sufficient for API-level monitoring.

A read-only experimental SQL model is allowed earlier if the engineering gates
pass. It must be documented as an experimental path and must not claim real
workload quality without qrels/proxy-qrels.

Until then, the correct state is:

```text
research-grade read-only generation path
+ converged evidence-atom abstraction
+ no frozen product API
+ no mutable native index
```

## M14-M17 Result Update

The full M14-M17 exploration is saved in:

```text
sae-m14-m17-exploration-report.md
scripts/research_sae_m14_m17_exploration.py
results/sae/m14-m17/exploration/summary.md
```

Result:

```text
M14 selectivity frontier 2: failed
M15 compact doc-row rerank: passed
M16 real workload readiness: failed
M17 SQL model gate: blocked
```

### M14 Decision

The current qrel/dense reliability selectors and graph/mixed candidate-only
expansions did not produce a better selectivity frontier:

```text
best quality/cost row remains h16_base_full
```

`h8_graph_candidate` nearly preserved quality but was too expensive.
`h8_mixed_candidate` was safe but did not materially improve over `h8_base`.
Reliability query budgets lost too much quality.

Conclusion:

```text
Do not keep M14 as the engineering blocker.
Keep selectivity frontier research as a side track.
```

### M15 Decision

`doc128` passed:

```text
Recall@100 = 0.7937
MRR@20     = 0.6840
rerank terms = 106,998.4100
rerank reduction = 26.9%
```

Conclusion:

```text
Promote compact top-128 doc rows to the primary engineering path.
```

### Updated Next Milestones

| Milestone | Purpose | Gate |
| --- | --- | --- |
| M18 EATMH002 payload | Implemented compact top-128 doc-row payload candidate | Closed, passed |
| M19 C/PG compact parity | Implemented C reader and PostgreSQL by-id smoke path | Closed for read-only parity |
| M20 efficiency runner | Implemented efficiency-only runner | Closed on benchmark artifacts |
| M20 real workload efficiency matrix | 5k-doc arxiv/pubmed/policy pilot completed with 256-dim SAE | scale only after M27 model closure |
| M20b real workload quality matrix | Future qrel/proxy-qrel construction for arxiv/pubmed/commons | canonical_quality_ready = true |
| M21 SQL model draft | Runtime-query evidence-atom SQL and full15 parity harness are implemented | evaluation harness only |
| M27 exploration closure | Text-to-atoms, SoftSAE, and concept-vocabulary final closure | product model closed or dense-removal not ready |

M20 can validate runtime feasibility, but it cannot validate retrieval quality
without a control set. Productization-quality claims remain blocked until M20b
or equivalent real-workload qrels/proxy-qrels exist.

## M18-M20 Engineering Result Update

The M18-M20 implementation pass is saved in:

```text
sae-m18-m20-engineering-report.md
scripts/research_sae_m20_efficiency_matrix.py
results/sae/m18/eatmh002/summary.md
results/sae/m19/eatmh002-c/summary.md
results/sae/m20/efficiency-matrix/summary.md
```

Prior result:

```text
M18 EATMH002 payload: implemented
M19 C reader: implemented
M19 PostgreSQL by-id smoke: implemented
M20 efficiency-only runner: implemented
```

The current compact payload keeps impact-head candidate generation on full
rows and compacts only doc-row rerank vectors:

| Run | Recall@100 | MRR@20 | Candidates | Rerank terms |
| --- | ---: | ---: | ---: | ---: |
| `payload_head16` | 0.7964 | 0.6790 | 852.8940 | 146,385.0740 |
| `payload_head16_doc128` | 0.7937 | 0.6840 | 852.8940 | 106,998.4100 |

The standalone C scan path preserves expected-order parity:

| Run | Exact ratio | Mean ms |
| --- | ---: | ---: |
| `head16_scan` | 1.0000 | 0.2350 |
| `head16_doc128_scan` | 1.0000 | 0.1726 |

The PostgreSQL smoke also passed for both `EATMH001` and `EATMH002` through
direct bytea and resident by-id generation queries.

Updated milestone state:

| Milestone | Status | Next action |
| --- | --- | --- |
| M18 | Closed, passed | Keep `EATMH002/doc128` as compact candidate |
| M19 | Closed for read-only smoke | Keep C/PG parity tests as regression gates |
| M20 runner | Closed, implemented | Use it for real arxiv/pubmed/commons artifacts |
| M20 real corpus run | Pilot passed | Scale beyond 5k docs after M27 chooses the model lineage |
| M20b quality | Open | Build qrels/proxy-qrels before quality claims |
| M21 SQL model | Harness passed | `EATMH001/EATMH002` runtime query by-id SQL is the evaluation harness; `UBMXM001` is a source-aware embedded-query control |
| M22 SAE-SPLADE concept vocabulary | Open final push | Use arXiv:2604.21511 to close concept-encoder and sparse-cost planning |
| M27 exploration closure | New canonical next phase | Close text-to-atoms, SoftSAE selector, and concept-vocabulary before product engineering |

## M21-M22 Planning Update

The first unified BM25+SAE PostgreSQL payload checkpoint is saved in:

```text
sae-unified-payload-pg-readonly-report.md
scripts/test_research_sae_unified_payload_pg.py
```

Result:

```text
EATMH001/EATMH002 runtime query by-id SQL: implemented
UBMXM001 direct/resident embedded-query control: implemented
embedded-query parity smoke: passed
query_filter smoke: passed
doc_tids smoke: passed
full15 runtime-query parity harness: passed
parsed UBMX cache: deferred; not the mainline blocker
```

This means M21 is no longer blocked at the basic SQL visibility layer. The
source-blind evidence-atom family already has the product-shaped runtime query
contract:

```text
ii42_evidence_atom_query_by_id(
    generation_table,
    generation_id,
    query_atoms[],
    query_weights[],
    k,
    doc_tids
)
```

The full15 Python/C/PostgreSQL parity and diagnostics harness around that
contract has now passed. `UBMXM001` remains useful as a source-aware control
surface, but it should not define the mainline API.

Full15 M21 result:

| Payload | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | PG strict parity | PG mean ms | Rerank terms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head16` | 0.8365 | 0.8416 | 0.7506 | 0.7148 | 1.0000 | 0.8112 | 109,857.3877 |
| `head16_doc128` | 0.8354 | 0.8414 | 0.7492 | 0.7136 | 1.0000 | 0.7460 | 84,820.7950 |

The next model-side milestone is saved in:

```text
sae-milestone22-sae-splade-concept-roadmap.md
```

`arXiv:2604.21511` changes the planning emphasis. It validates a direction very
close to our target: replacing a token vocabulary with SAE-learned concept
latents inside a learned sparse retrieval model. The most relevant lessons are:

- concept latents can be treated as a sparse output vocabulary, not only as a
  dense-embedding compression artifact;
- SAE pretraining and IR fine-tuning should stay separated;
- TopK activation budgets are stronger cost controls than late clipping alone;
- QD-FLOPs-style sparse interaction cost should become a first-class benchmark
  metric;
- multilingual and synonymy/polysemy diagnostics are useful for policy/RAG
  workloads even before labeled qrels exist.

Updated immediate order after the M27 planning decision:

```text
1. run M27 text-to-atoms final push against the teacher_16384 gap
2. run M26b learned budget selector as a closure side track
3. run M22 concept-vocabulary paper-aligned control
4. keep EATMH002/head16_doc128 as the physical-cost guardrail
5. keep real arxiv/pubmed/policy work efficiency-only until qrels exist
6. resume read-only engineering only if M27 reaches the product-research pass
```

Do not start mutable access-method integration or product API freeze until M27
has an explicit final decision.

## M24 Training Closure Update

The larger-teacher text-student checkpoint is saved in:

```text
sae-milestone24-16384-distillation-report.md
scripts/research_sae_m24_teacher_distillation.py
```

Result:

```text
shared_sae_16384_64 student: cost-only, not promoted
shared_sae_12288_64 student: cost-only, not promoted
shared_sae_16384_64 h384 retry: cost-only, not promoted
```

M24 closes the high-value larger-teacher distillation check. The 12288/16384
teachers remain strong representation signals, but the direct text-to-atoms
student did not preserve their ranking quality. The best M24 rows reduce
candidate/posting cost, but they fall below the current `budget16` text-student
baseline by about five Recall@100 points and about seven MAP@100 points.

Updated decision:

```text
freeze 12288/16384 text-student loss tuning
do not claim dense-removal readiness
return to M27 exploration closure before engineering
```

The larger-teacher result should not be retried through the same loss family.
Further model-side work is allowed only through the M27 closure tracks: a new
encoder architecture, stronger supervision source, SoftSAE learned selector, or
paper-aligned concept-vocabulary control.

## M26 SoftSAE Adaptive Sparsity Update

The SoftSAE review is saved in:

```text
sae-milestone26-softsae-adaptive-sparsity-report.md
```

`arXiv:2605.06610` does not change the immediate engineering route, but it
does identify a useful model-side side track. The paper's key transferable idea
is input-dependent sparse capacity: each sample predicts its own active
feature budget instead of using one global TopK. This maps directly to our
current fixed deployment knobs:

```text
query active budget
doc80/doc88/doc96/doc128 export clipping
impact-head width
rerank doc-row width
```

Updated decision:

```text
keep M21 runtime-query evidence-atom SQL as the evaluation harness
add M26 adaptive sparse capacity as an M27 side track
start M26 with post-hoc dynamic budget simulation and learned selector,
not expensive adaptive-k training
```

The first M26 experiment should simulate per-query and per-document budgets
over existing full15 artifacts. Candidate signals include query length, BM25
score concentration, SAE atom entropy, top atom mass, teacher coverage slope,
and predicted posting fanout. Only if that simulation forms a clear quality/cost
frontier should we train a retrieval-aware budget predictor or adaptive-k SAE.

Do not put Soft Top-K into PostgreSQL query execution. If adopted, it is a
training-time mechanism only; inference must still export hard sparse atoms.

## M21 Runtime Evidence-Atom Mainline Update

The M21 mainline update is saved in:

```text
sae-m21-runtime-evidence-atom-mainline-report.md
scripts/research_sae_m21_runtime_evidence_atom_parity.py
scripts/research_sae_m26_dynamic_budget_sim.py
```

Result:

```text
M21 runtime evidence-atom parity harness: implemented
M26 dynamic budget simulation runner: implemented
scifact temp-Postgres M21 smoke: passed
EATMH001/EATMH002 PostgreSQL smoke: passed
M26 dynamic budget smoke: passed
full15 M21 Python/C/PostgreSQL parity: passed
full15 M26 dynamic budget simulation: completed, not promoted
```

The M21 full15 run makes EATMH002/head16_doc128 the current physical-cost
guardrail. The M26 full15 run did not promote adaptive budgets yet:
`adaptive_fanout` cuts cost but loses too much ranking quality, while
`fixed_q64_d96_h12` is the better medium-cost profile. The next bottleneck is
therefore M27 model closure, with M26 learned budget selection kept as a side
track.

## M20 Real-Corpus Pilot Update

The first M20 real-corpus pilot is saved in:

```text
sae-m20-real-corpus-pilot-report.md
```

Result:

```text
arxiv/pubmed/policy_ca_chunks 5k-doc efficiency pilot: passed
quality metrics: intentionally not reported
```

Important finding:

```text
commons stored vectors are 256-dimensional
full15 shared_sae_8192_64 is 768-dimensional
```

The pilot therefore trained a separate 256-dimensional `shared_sae_4096_64`
only to validate the physical path. On the 5k-doc real-corpus pilot,
`EATMH002/head16_doc128` reduced mean rerank terms from `210,656.7400` to
`136,740.0833` and C reader latency from `0.3577 ms` to `0.2503 ms`.

The next model decision is not simply 256-dimensional versus 768-dimensional
storage. M27 must first decide whether the product route can use direct
text-to-atoms at all. Only after that should commons choose an embedding or
teacher lineage for larger real-corpus scale-up.

## M27 Exploration Closure Update

The new canonical closure plan is saved in:

```text
sae-m27-exploration-closure-plan.md
```

M27 supersedes the previous "start read-only engineering next" framing. The
engineering path is feasible, but the product model is not closed because the
direct text-to-atoms student still trails the Snowflake-derived teacher:

```text
Recall@100 gap = -0.0131
MRR@20 gap     = -0.0171
NDCG@10 gap    = -0.0305
MAP@100 gap    = -0.0360
```

M27 has three open final-push tracks:

```text
text-to-atoms ranking-quality closure
SoftSAE learned-selector closure
M22 concept-vocabulary closure
```

The exit decision must be explicit: promote text-to-atoms, keep only a teacher
harness, or record dense-removal as not ready.

## M27 Execution Result

M27 is now executed through:

```text
scripts/research_sae_m27_exploration_closure.py
sae-m27-exploration-closure-report.md
```

Result:

```text
dense-removal not ready
```

Closure details:

| Track | Decision | Evidence |
| --- | --- | --- |
| Text-to-atoms | `open-final-push-failed` | `290` full15 student rows scanned; best row is still `baseline_budget16`, below product-research pass |
| SoftSAE selector | `parked` | learned selector preserves fixed-high quality but cuts postings by only `3.27%`, below the `20%` gate |
| M22 concept vocabulary | `parked` | existing SPLADE/control evidence is below current text-student ranking quality |

This changes the immediate engineering order:

```text
1. do not freeze direct text-to-atoms SQL/API
2. do not claim dense retrieval removal
3. keep M21/EATMH as a read-only evaluation harness and teacher-path prototype
4. resume model research only if a new architecture or supervision source is introduced
5. use real-corpus work for efficiency only unless qrels/proxy-qrels are added
```
