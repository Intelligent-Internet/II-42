# SAE Phase 5 Gap Exploration Plan

Date: 2026-05-12

Current status has been consolidated in:

```text
docs/research-sae/reports/milestones/sae-current-status-and-engineering-readiness-report.md
```

Use that report as the current readiness snapshot. This file remains useful for
the historical Phase 5 exploration backlog and supporting evidence.

## Purpose

Phase 4 proved that SAE-derived sparse impacts can be stored and queried from a
PostgreSQL-resident read-only payload. It also showed that the current
BM25+SAE signal is competitive with BM25+dense on the sampled BEIR-style
matrix, but the Python scorer is still more expensive and the native payload is
not yet a production index family.

Phase 5 therefore has a narrower purpose: identify the remaining gaps before
we invest in a mutable native index. The target is still a single generic
sparse-impact engine:

```text
BM25 lexical impacts
+ SAE latent semantic impacts
+ future sparse semantic sources
  -> one impact namespace
  -> one boundable scorer
  -> one block/max or WAND-like traversal
```

## Current Baseline

The current branch has two relevant baselines.

| Path | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean query ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.6025 | 0.7035 | 0.5904 | 0.5031 | 0.4134 | 1.9422 |
| BM25+dense | 0.6947 | 0.7817 | 0.6860 | 0.5984 | 0.5010 | 2.4125 |
| BM25+SAE | 0.7045 | 0.7947 | 0.6835 | 0.6036 | 0.5052 | 5.4446 |

Interpretation:

- BM25+SAE is a real quality signal, not only an experimental curiosity.
- BM25+dense remains the cheaper current path.
- The next step must focus on native sparse efficiency and boundable scoring,
  not more late-fusion weight tuning.

## Nine Phase 5 Exploration Targets

### 1. Strong Learned Sparse Baselines

Question: are we beating or at least matching practical learned sparse
retrieval directions?

Scope:

- Check local availability for BGE-M3 sparse and SPLADE-style baselines.
- Record whether existing BGE-M3 artifacts are already present.
- Do not block Phase 5 on installing large dependencies.

Exit condition:

- A concrete feasibility status is recorded.
- If the dependency is missing, the report must say so and keep the comparison
  as a follow-up rather than silently treating SAE as the only learned sparse
  option.

### 2. Boundable Score Contract

Question: can BM25+SAE use a native exact scorer without per-query max
normalization?

Scope:

- Compare current per-query normalization against raw weighted sum.
- Compare fixed source-level saturation:

```text
saturated(score, tau) = score / (score + tau)
```

Rationale:

- Per-query max normalization is useful offline, but it is not the first exact
  native contract because block upper bounds would depend on candidate-window
  state.
- Fixed saturation remains upper-boundable if the index can upper-bound the raw
  source score.

Exit condition:

- The report identifies whether fixed saturation is close enough to current
  normalized quality to become the Phase 5 native scorer candidate.

### 3. BM25 + Dense + SAE Tri-Hybrid

Question: does SAE add value even when dense is present, or is it only
replacing dense?

Scope:

- Add a three-source full-scan research scorer.
- Compare quality to BM25+dense and BM25+SAE.

Exit condition:

- If tri-hybrid materially beats both two-source paths, SAE should be treated
  as a third signal.
- If not, Phase 5 should continue optimizing SAE as a dense-replacement
  candidate.

### 4. Query-Dimension Budget

Question: how much query-side SAE fanout can be cut without losing too much
quality?

Scope:

- Evaluate query top-4, top-8, top-12, top-16, top-32, and top-64 latent dims.
- Record touched posting counts as the native traversal cost proxy.

Exit condition:

- Identify a practical query active-dimension budget for the first native
  scorer.

### 5. Document-Vector Pruning

Question: can the resident doc-vector/rerank payload be smaller without
collapsing recall?

Scope:

- Evaluate document latent pruning at top-16, top-32, top-48, and top-64.
- Record postings and quality deltas.

Exit condition:

- Decide whether doc-vector compression is a Phase 5 first-class task or a
  later memory optimization.

### 6. Corpus-Specific Sparse Vocabulary

Question: does a simple corpus-specific lexical expansion close some of the
semantic gap without learned dense/SAE features?

Scope:

- Add a unigram+bigram BM25 research baseline.
- Compare BM25 and BM25+SAE with and without bigram lexical dimensions.

Exit condition:

- If bigrams are cheap and useful, keep the native payload namespace generic
  enough for lexical n-gram dimensions.
- If not, keep focus on learned semantic sparse dimensions.

### 7. Seismic-Inspired Physical Layout Probe

Question: which posting and block summaries matter most for native pruning?

Scope:

- Measure dimension fanout, high-DF dimensions, impact-head coverage, and
  candidate concentration.
- Map findings to block summaries, source-aware upper bounds, and learned
  document ordering.

Exit condition:

- Phase 5 payload design must specify which per-block metadata is mandatory
  before mutable index work begins.

### 8. SINDI-Inspired Sequential Access And Rerank Probe

Question: can a candidate-generation plus compact doc-vector rerank path be
fast enough without opening most documents?

Scope:

- Use posting fanout and doc-vector pair counts as a memory/decode proxy.
- Keep random lookup pressure visible in the report.

Exit condition:

- Identify whether Phase 5 should optimize resident decode memory first or
  improve candidate selectivity first.

### 9. Real Workload Readiness

Question: do we have enough arxiv/pubmed/commons artifacts to validate against
our real RAG workload?

Scope:

- Search for available local query/qrel artifacts.
- Do not depend on the production database for this planning step.

Exit condition:

- If real workload qrels are missing, the next data task is explicit:
  construct arxiv/pubmed/commons query sets before claiming production
  quality.

## Phase 5 Decision Gate

Move from exploration to implementation only if:

- BM25+SAE still improves BM25 on real-qrels style matrices.
- The selected scorer is compatible with exact native upper bounds.
- Candidate fanout has a credible native cost path.
- Real workload validation is either available or clearly scheduled as the
  next data milestone.

## Archived And Active Tracks

The first unattended Phase 5 pass is archived in:

```text
sae-phase5-archived-explorations.md
```

The model-side SPLADE gap has now been tested for `naver/splade_v2_distil`.
This was intentionally run before any SQL sidecar or native payload integration
because it answers a model/signal question:

```text
Can a learned sparse text encoder beat or complement SAE-over-dense enough to
justify adding it as another source in the generic sparse-impact engine?
```

SPLADE remains outside PostgreSQL. The database-facing shape is still generic
weighted sparse impacts:

```text
source_id = splade
source_dim_id = token_id
weight = learned sparse lexical/expansion impact
```

The SPLADE baseline compared:

```text
BM25
BM25+SAE
SPLADE
BM25+SPLADE
BM25+SAE+SPLADE
```

The result is archived in:

```text
sae-splade-baseline-report.md
results/sae/phase5/splade-baseline/summary_normalized.md
results/sae/phase5/splade-baseline/summary_fixed_saturation.md
```

Decision: `naver/splade_v2_distil` is useful as a learned-sparse reference, but
it does not beat the current BM25+SAE path on the five-dataset mean. Do not
move this checkpoint into SQL/native integration now. Reopen SPLADE only for
stronger checkpoints, larger active budgets, or real workload evidence.

The detailed reason this result is not a general rejection of SPLADE is saved
in `sae-splade-baseline-report.md`.

## Milestone 8 Exploration-First Update

The next stage should not be treated as productization. The current read-only
PostgreSQL path is a laboratory for deciding whether the evidence-atom route is
strong enough to become the next-generation SQL/index model.

The latest implementation and benchmark are saved in:

```text
sae-milestone8-pg-workspace-and-exploration-report.md
scripts/research_sae_milestone8_pg_exploration.py
results/sae/milestone8/pg-exploration/summary.md
```

What changed:

- The EATMH001 query path now has an adaptive backend-local workspace for
  larger payloads.
- Small 2k-document research payloads keep the original palloc/zeroing path
  because the workspace mark branches are not free.
- Synthetic 10k-document expansion keeps cached by-id latency below 1 ms on
  the sampled `scifact` and `fiqa` runs while preserving exact parity.
- The benchmark now records query trace means such as `candidate_docs`,
  `candidate_postings`, and `rerank_doc_terms`.
- A SciFact head-size sweep confirms that impact-head size directly controls
  the candidate/rerank cost frontier and should be optimized together with
  recall, not as a fixed constant.

Current medium-term exploration matrix:

| Track | Goal | Current status | Next evidence needed |
| --- | --- | --- | --- |
| Workspace and memory lifecycle | Remove per-query full-size allocation/zeroing without hurting small payloads | Adaptive path implemented | Larger non-synthetic payloads and threshold sweep |
| Cross-dataset PostgreSQL benchmark | Separate native SQL path cost from Python scorer cost | M8 runner covers multi-dataset synthetic expansion | All five datasets plus real commons/arxiv/pubmed qrels |
| Candidate selectivity | Keep impact-head candidate count low while preserving recall | Trace counters are now collected | Sweep head size, atom reliability, and candidate-only mixed atoms |
| Exact rerank cost | Reduce doc-row term scans after candidate generation | Rerank doc terms are now visible | Doc-vector compression and selective rerank layout experiments |
| SQL model gate | Avoid freezing an API before the index direction is settled | Current functions remain experimental | Gate on quality, latency, memory, and exact parity |

This keeps the stage intentionally broad. The priority is to exhaust the
deep-fusion design space before committing to a product API or mutable index
maintenance model.

## M9-M13 Consolidated Exploration Update

The broader unattended exploration is saved in:

```text
sae-m9-m13-exploration-report.md
scripts/research_sae_m9_m13_exploration.py
results/sae/m9-m13/exploration/summary.md
```

The most important finding is that the current best quality/cost point is still
the simple evidence-atom baseline:

```text
h16_qb0_weight
Recall@100 = 0.7964
MRR@20     = 0.6790
candidates = 852.8940
```

The strict M10 selectivity frontier did not improve on that row. This is a
useful negative result: simple query budgets and DF/IDF-cost query selectors do
not yet preserve enough quality to replace the unlimited query path.

M11 found a better near-term systems direction:

```text
h16_qball_doc128
Recall@100 = 0.7937
MRR@20     = 0.6840
rerank terms = 106,998.4100
```

This reduces rerank terms by about 27% versus the full doc-row path while
keeping Recall@100 close and improving MRR/NDCG in the current matrix. Treat
`doc128` as the next compact-rerank candidate, but verify with repeated runs
before encoding it as a payload contract.

M12 still blocks productization:

```text
canonical_ready = false
```

No canonical arxiv/pubmed/commons query plus qrel pair was found in the scan.
The next stage should therefore be:

1. M14 selectivity frontier 2: learned or teacher-derived atom reliability,
   token-latent candidate expansion, mixed atoms candidate-only.
2. M15 compact doc-row rerank: make top-128 doc rows a real C/PG payload
   variant and verify quality stability.
3. M16 real workload quality readiness: build arxiv/pubmed/commons qrels or
   proxy-qrels before making quality claims.
4. M17 SQL model draft only after the above gates pass.

The detailed converged plan is now maintained in:

```text
sae-converged-research-and-engineering-plan.md
```

That plan supersedes the broad Phase 5 exploration list as the active roadmap.
The older list remains useful as historical context, but the next work should
be judged against the narrower gates below.

## Converged Active Roadmap

### Active Baselines

Quality guardrail:

```text
h16_qb0_weight
Recall@100 = 0.7964
MRR@20     = 0.6790
candidates = 852.8940
```

Engineering candidate:

```text
h16_qball_doc128
Recall@100 = 0.7937
MRR@20     = 0.6840
rerank terms = 106,998.4100
```

Low-cost reference:

```text
h8_qb0_weight
Recall@100 = 0.7856
MRR@20     = 0.6791
candidates = 516.8160
```

### Stop Conditions

Do not spend the next phase on:

- simple global query-budget tuning as the primary selectivity path;
- DF/IDF-cost query selectors without a new signal;
- head32 as a default candidate path;
- product SQL/API stabilization;
- mutable native index maintenance.

### Next Milestones

| Milestone | Purpose | Gate |
| --- | --- | --- |
| M14 selectivity frontier 2 | Keep h16 quality while moving cost toward h8 | Recall@100 >= 0.7930, MRR@20 >= 0.6750, candidates <= 650 |
| M15 compact doc-row rerank | Turn doc128 into a C/PG payload candidate | Recall drop <= 0.005, rerank terms down >= 20%, parity preserved |
| M16 real workload quality readiness | Validate arxiv/pubmed/commons style RAG retrieval quality | canonical query/qrel or proxy-qrel sets exist |
| M17 SQL model draft | Draft read-only API only after M14-M16 | no research-only knobs frozen into public API |

## M14-M17 Result Update

The M14-M17 exploration is saved in:

```text
sae-m14-m17-exploration-report.md
scripts/research_sae_m14_m17_exploration.py
results/sae/m14-m17/exploration/summary.md
```

Outcome:

- M14 selectivity frontier 2 did not pass. The baseline `h16_base_full`
  remains the selected quality/cost row.
- M15 compact doc-row rerank passed. `h16_doc128` reduced rerank terms by
  about 26.9%, kept Recall@100 within 0.005, and improved MRR@20.
- M16 still did not find canonical arxiv/pubmed/commons query/qrel artifacts.
- M17 product SQL/API remains blocked, but the read-only engineering path can
  continue with efficiency-only validation.

The active route is therefore updated:

1. M18: implement `EATMH002` compact top-128 doc-row payload candidate.
2. M19: add C and PostgreSQL read-only parity/benchmark for `EATMH002`.
3. M20: run a real-workload efficiency matrix for latency, fanout, payload,
   memory, cache behavior, and result-shape sanity. Do not report Recall/MRR
   without labels.
4. M20b: construct real workload qrels or proxy-qrels for future quality
   validation.
5. M21: draft the read-only SQL model after M19 and M20 efficiency pass.

M14-style selectivity work can continue in parallel, but it is not the next
engineering blocker.

## Milestone 5 Native Reader Status

The evidence-atom payload now has a standalone C reader checkpoint:

```text
sae-milestone5-evidence-c-reader-report.md
tests/research_sae_evidence_payload_reader.c
scripts/research_sae_milestone5_evidence_c_reader.py
results/sae/milestone5/evidence-c-reader/summary.md
```

Current five-dataset guardrail:

| Path | Recall@100 | MRR@20 | Mean query ms |
| --- | ---: | ---: | ---: |
| Python `head16` payload | 0.7964 | 0.6790 | 7.6516 |
| C `head16_scan` payload | 0.7964 | 0.6790 | 0.2300 |

The C reader keeps exact doc-order parity for every query. `scan` is the
fastest current native strategy; `merge` is slower on the research slices but
remains the large-dictionary fallback because it avoids an atom-count-sized
query array.

Updated active implementation order:

1. Add reusable/touched query-local workspace for candidate and score arrays.
2. Keep scan semantics first, but stop allocating and zeroing doc/atom-sized
   arrays on every SQL call.
3. Keep merge fallback available for large atom dictionaries.
4. Measure resident memory and SQL-call overhead before changing the payload
   scorer.
5. Defer mutable deltas and maintenance until read-only residency is stable.

The first item from the previous plan is complete:

```text
sae-milestone6-pg-evidence-atom-report.md
ii42_evidence_atom_query
ii42_evidence_atom_query_by_id
scripts/test_research_sae_evidence_atom_pg_generation.py
```

The PostgreSQL path is now correct and SQL-visible. It is not yet the final
performance model because the by-id function still decodes the bytea payload
per call.

The decoded-payload bottleneck is now also complete:

```text
sae-milestone7-pg-generation-cache-report.md
ii42_sae_generation_cache_clear
ii42_sae_generation_cache_state
scripts/research_sae_milestone7_pg_cache_benchmark.py
```

Benchmark on a 4.38 MB `scifact` `head16` evidence payload:

| Path | Mean ms |
| --- | ---: |
| `direct_bytea` | 2.8116 |
| `by_id_cold_clear_each` | 1.4916 |
| `by_id_cached` | 0.5180 |

The cache keeps exact parity and invalidates on upsert/delete. The remaining
Phase 5 systems bottleneck is now query-local workspace allocation and reset,
not payload decode.

## Pre-SQL Unified Exploration Checkpoint

The follow-up pass has now pushed the remaining pre-SQL questions far enough
to decide the next implementation direction:

```text
scripts/research_sae_pre_sql_unified_exploration.py
sae-pre-sql-unified-exploration-report.md
results/sae/phase5/pre-sql-unified-exploration/summary.md
```

This pass covered the eight open questions that matter before SQL/native
integration:

| Question | Current result |
| --- | --- |
| Mixed-source bound tightness | Exact, but still opens about `98%` of documents with simple layouts. |
| Impact calibration | Fixed saturation remains the first native scorer contract; `sae_weight=2.0..2.5` is the current range. |
| BM25+SAE block layout | Natural, SAE-first, BM25-first, mixed, and random layouts do not solve selectivity by themselves. |
| Budgeted exactness | `q32_d64` is a useful systems profile, but it is not exact; `q16_d64` is too lossy as a default. |
| Real workload qrels | No canonical arxiv/pubmed/commons qrels are wired into the runner yet; real-workload M20 can only validate efficiency until this changes. |
| BGE-M3 sparse baseline | Local BGE-M3 artifacts are dense-vector profiles; sparse weights still need an export pass. |
| Stronger SPLADE / larger active dims | Current SPLADE v2-distil does not justify SQL integration; active-128 is a quality profile, not the first systems profile. |
| Real-scale payload size | Raw payload estimate is about `1.74 GB / 1M docs` before production metadata and alignment overhead. |

Decision: do not spend the next phase on late-fusion tuning. The next useful
systems work is a tighter source-aware/impact-ordered native traversal and a
better physical representation. The next useful real-workload work is split:
first measure efficiency on arxiv/pubmed/commons corpora, then add qrels or
proxy-qrels before making quality claims.

## Milestone 1 And Deep-Fusion Branch Point

The current stable checkpoint is tagged as:

```text
sae-milestone-1
```

Use it as the rollback point if the post-milestone deep-fusion line fails.

The new exploratory question is whether BM25 tokens and SAE latent activations
can be collapsed into the same evidence-atom abstraction, instead of keeping a
source-level BM25 score and SAE score.

The first report for that line is:

```text
bm25-sae-deep-fusion-exploration-report.md
```

It studies three routes:

1. a source-blind single atom namespace;
2. a token-latent co-activation graph;
3. a retrieval-aware mixed sparse encoder.

The first milestone-2 matrix is now saved in:

```text
sae-milestone2-deep-fusion-results-report.md
```

Result: Route 1 source-calibrated single atoms are the strongest immediate
direction. Route 2 graph expansion is noisy, Route 2 mixed atoms are worth
narrowing, and the current Route 3 global reliability proxy is not sufficient.

The second milestone-2 pass is saved in:

```text
sae-milestone2-followup-experiments-report.md
```

Result: impact-head candidate generation over Route 1 source-blind atoms is
the strongest systems route. `impact_head_16` slightly improves mean
Recall@100 while reducing touched postings from about `17656` to `1382` and
candidate docs from about `1981` to `853`. Retrieval-aware SAE atom training
remains promising, but the current global atom-reweighting proxy should be
replaced by a query-conditioned selector.

The consolidated milestone-2.5 pass is saved in:

```text
sae-milestone25-consolidated-results-report.md
```

Result: larger query-conditioned selector training over the current pseudo
query artifacts does not generalize enough to replace Route 1. The safe
selector variant, which preserves baseline high-impact SAE atoms before using
trained weights, reduces the regression but still trails `head16_route1` on
the five-dataset mean. `head16_route1` remains the best quality/cost point:
mean Recall@100 `0.7964`, MRR@20 `0.6790`, touched postings about `1382`, and
candidate docs about `853`.

Decision: pause further selector-weight tuning over fixed SAE atoms. The next
learned direction should be retrieval-aware atom training or mixed atom
generation with an explicit candidate-budget/fanout term. The next native
systems direction should treat impact-head or impact-ordered traversal as the
first execution path for source-blind evidence atoms.

The milestone-3 fusion/training pass is saved in:

```text
sae-milestone3-fusion-training-report.md
```

Result: the technical integration route is now clearer. The first native path
should be the base source-blind evidence-atom impact-head payload followed by
exact sparse rerank. Pseudo-query-trained mixed token-latent atoms are not
calibrated enough for final scoring: direct mixed scoring drops mean MRR@20
from `0.6790` to `0.5657`, and token-expanded mixed scoring is worse. Mixed
atoms are only safe as candidate-only expansion followed by base rerank. That
candidate-only route gives a small head8 Recall@100 lift (`0.7856` ->
`0.7879`), but it does not displace `base_head16`.

Decision: do not block SQL/native integration on mixed atoms. Use mixed atoms
as an optional candidate-booster research line. The next training line should
learn retrieval atoms with separate candidate coverage, fanout, and calibration
objectives instead of manually reweighting current mixed atoms.

The milestone-4 evidence payload pass is saved in:

```text
sae-milestone4-evidence-payload-report.md
```

Result: the base source-blind evidence atom route now has a concrete binary
payload prototype, `EATMH001`. It serializes one atom dictionary for BM25 token
atoms and SAE latent atoms, plus impact-head candidate lists and doc-row sparse
vectors for exact candidate rerank. The payload reader matches the in-memory
reference with `100%` exact doc-order parity across the five datasets and
head8/head16/head32. Mean head16 quality remains Recall@100 `0.7964`, MRR@20
`0.6790`, touched postings about `1382`, and candidate docs about `853`.

Decision: the next mainline engineering step is a standalone C reader for
`EATMH001`, followed by a PostgreSQL read-only resident function. The next
optimization concern is exact rerank cost: head16 rerank scans about `146k`
doc-row terms per query in the Python prototype because lexical token atoms
are now part of each document vector.

## First Unattended Exploration Result

Script:

```text
scripts/research_sae_phase5_gap_exploration.py
```

Artifacts:

```text
results/sae/phase5/gap-exploration/phase5_gap_exploration.json
results/sae/phase5/gap-exploration/summary.md
sae-phase5-gap-exploration-report.md
```

The first run used the existing `sae_8192_64` artifacts over:

```text
scifact, scidocs, nfcorpus, arguana, fiqa
```

### Score Contract Result

| Run | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `normalized` | 0.7045 | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `raw` | 0.6431 | 0.7470 | 0.6315 | 0.5457 | 0.4522 |
| `fixed_saturation` | 0.6971 | 0.7934 | 0.6742 | 0.5952 | 0.4987 |

Conclusion: raw weighted score is not calibrated enough. Fixed source-level
saturation is close to per-query normalization and remains compatible with
native exact upper bounds, so it should be the first Phase 5 native scorer
candidate.

### Dense/SAE Interaction Result

| Run | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `bm25_dense` | 0.6947 | 0.7817 | 0.6860 | 0.5984 | 0.5010 |
| `bm25_sae` | 0.7045 | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_dense_sae` | 0.7141 | 0.8019 | 0.6927 | 0.6168 | 0.5147 |

Conclusion: SAE is not just replacing dense. In the current matrix, tri-hybrid
is better than both two-source paths. This means Phase 5 should keep the
single-index sparse-impact direction, but we should not claim dense can be
removed universally until real workload validation is available.

### Budget Result

| Query active dims | Recall@100 | MRR@20 |
| ---: | ---: | ---: |
| 4 | 0.7651 | 0.6116 |
| 8 | 0.7724 | 0.6392 |
| 12 | 0.7801 | 0.6404 |
| 16 | 0.7833 | 0.6606 |
| 32 | 0.7889 | 0.6735 |
| 64 | 0.7947 | 0.6835 |

| Document active dims | Recall@100 | MRR@20 |
| ---: | ---: | ---: |
| 16 | 0.7799 | 0.6552 |
| 32 | 0.7799 | 0.6690 |
| 48 | 0.7904 | 0.6764 |
| 64 | 0.7947 | 0.6835 |

Conclusion: query top-16/top-32 and document top-48 are the first credible
budget points. Query top-4/top-8 are too lossy for a general default.

### Lexical Expansion Result

| Run | Recall@100 | MRR@20 |
| --- | ---: | ---: |
| `bm25_bigram` | 0.6949 | 0.5570 |
| `bm25_bigram_sae` | 0.7943 | 0.6708 |

Conclusion: naive bigram BM25 does not replace learned semantic sparse
features. It may remain useful as a future source type, but it is not the next
mainline optimization.

### Physical Probe Result

Mean counters:

| Counter | Mean |
| --- | ---: |
| `sae_posting_p95` | 194.6000 |
| `query_sae_touch_p95` | 8476.4000 |
| `doc_vector_pairs_mean` | 64.0000 |

Conclusion: the main Phase 5 systems problem is still candidate fanout and
resident doc-vector decode cost. The next implementation work should improve
native sparse candidate selectivity and resident doc-vector compression before
mutable maintenance.

### Follow-Up Priority

1. Implement fixed-saturation scoring in the native simulator and PostgreSQL
   read-only candidate path.
2. Add query top-16/top-32 budget modes to the native candidate path and
   measure exact overlap against full BM25+SAE.
3. Add document top-48 or compressed doc-vector payload generation.
4. Run real arxiv/pubmed/commons efficiency tests first. Build qrels or
   proxy-qrels separately before using that workload to answer quality questions
   such as whether dense fallback can be removed.

## M18-M20 Implementation Update

The active implementation result is now:

```text
sae-m18-m20-engineering-report.md
```

Key updates:

- `EATMH002` compact doc-row payload is implemented.
- Standalone C and PostgreSQL read-only by-id paths accept both `EATMH001` and
  `EATMH002`.
- `doc128` keeps the full impact-head candidate pool and reduces rerank doc
  terms from `146,385.0740` to `106,998.4100` on the five-dataset mean.
- The M20 efficiency-only runner is implemented, but the real
  arxiv/pubmed/commons corpus run still requires a prepared local artifact.
- Quality claims for real workloads remain blocked until qrels or defensible
  proxy-qrels exist.

The Phase 5 backlog should now treat `doc128` compact rerank as the current
engineering candidate and focus any further exploration on real workload
artifact preparation, candidate selectivity, or atom training improvements.

## M21-M22 Integration And SAE-SPLADE Planning Update

The unified BM25+SAE read-only SQL surface is now documented in:

```text
sae-unified-payload-pg-readonly-report.md
```

This closes the first PostgreSQL-visible `UBMXM001` checkpoint:

```text
direct bytea query: passed smoke
resident-table by-id query: passed smoke
query_filter: passed smoke
doc_tids: passed smoke
runtime query atoms: still open
parsed UBMX cache: deferred
```

The next planning milestone is:

```text
sae-milestone22-sae-splade-concept-roadmap.md
```

The new paper input, `From Tokens to Concepts: Leveraging SAE for SPLADE`,
supports a stronger version of the current direction: instead of treating SAE
only as a post-hoc dense teacher compression, train concept latents as the
output vocabulary of a learned sparse retriever.

Phase 5 should therefore split into two coordinated lines:

1. SQL/runtime integration:

```text
larger UBMX SQL benchmark
runtime query atom function
optional parsed UBMX cache only if decode cost dominates
```

2. Model/index co-design:

```text
QD-FLOPs-style sparse-cost metrics
TopK concept-vocabulary control model
larger latent vocabulary tests under posting-read gates
synonymy/polysemy and multilingual atom diagnostics
```

This supersedes further global fusion-weight tuning as the mainline work.
