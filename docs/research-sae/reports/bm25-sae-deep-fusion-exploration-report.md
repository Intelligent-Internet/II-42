# BM25 + SAE Deep Fusion Exploration Report

Date: 2026-05-12

Current status has been consolidated in:

```text
docs/research-sae/reports/milestones/sae-current-status-and-engineering-readiness-report.md
```

Use that report as the current readiness snapshot. This file remains the
historical deep-fusion exploration record.

Milestone base:

```text
sae-milestone-1
```

## Purpose

This report starts the post-milestone-1 exploration. The previous line proved a
useful but still source-aware shape:

```text
BM25 source score
+ SAE source score
  -> fixed-saturation fusion
```

That is already much better than late fusion, but it still treats lexical
matching and SAE semantic matching as two different score families.

The new question is more aggressive:

```text
Can BM25 tokens and SAE latent activations become the same kind of retrieval
evidence, stored and traversed by one physical inverted index, without a
source-level fusion step?
```

This is not guaranteed to be interpretable, and it may fail. The value of this
checkpoint is to define concrete experiments before touching the native index
hot path.

## Core Abstraction: Evidence Atoms

The most useful abstraction is an `evidence atom`.

An atom is any sparse feature that can support retrieval:

```text
token:cuda
token:graph
token:optimization
latent:4310
latent:917
mixed:cuda@4310
mixed:graph-neural-network@917
```

The index does not need to know whether an atom came from a token, an SAE
latent, or a learned mixed feature. At query time, scoring can collapse to:

```text
score(doc) = sum(query_impact(atom) * doc_impact(atom))
```

This is the real deep-fusion target:

```text
text matching
+ abstract semantic matching
  -> same atom namespace
  -> same postings format
  -> same traversal and pruning algorithm
```

Source metadata should still exist for debugging, calibration, and rebuilds,
but the runtime scorer should be able to run without separate BM25 and SAE
accumulators.

## Difference From The Current Unified Sparse Index

The current unified sparse index keeps one physical postings space, but the
score is still source-aware:

```text
bm25_raw(doc) = sum(token impacts)
sae_raw(doc) = sum(latent impacts)
score(doc) = f(bm25_raw, sae_raw)
```

Deep fusion asks whether we can instead do:

```text
score(doc) = sum(all atom impacts)
```

This matters because:

- one accumulator is simpler for WAND/MaxScore;
- block and posting upper bounds become atom-local instead of source-local;
- token and latent features can compete directly;
- candidate generation no longer depends on a source weight policy;
- future sources can be added as atoms rather than separate retrievers.

The risk is calibration. Token atoms and SAE atoms have different statistical
distributions. If their impacts are not calibrated into the same confidence
space, broad SAE atoms can behave like stopwords or lexical atoms can dominate
semantic recall.

## Route 1: Single Atom Namespace

### Hypothesis

BM25 tokens and SAE latent dimensions can be mapped into one atom namespace
with calibrated impacts. The runtime query path can be fully source-blind.

### Build Shape

Offline indexing:

```text
document text
  -> BM25 token atoms
  -> dense embedding
  -> SAE latent atoms
  -> calibrated atom impacts
  -> one postings table
```

Query:

```text
query text
  -> BM25 token atoms
  -> dense embedding
  -> SAE latent atoms
  -> calibrated query impacts
  -> one sparse query vector
```

### Impact Calibration

A first source-blind impact contract can normalize every atom family into the
same range before indexing:

```text
doc_impact(atom, doc) =
    atom_idf(atom)
  * local_strength(atom, doc)
  * reliability(atom)

query_impact(atom, query) =
    query_strength(atom, query)
```

Possible local strengths:

```text
token local strength = BM25 tf saturation without source-level final weight
SAE local strength = normalized latent activation
```

Possible reliability terms:

```text
rare useful atoms          -> higher reliability
high-DF broad atoms        -> lower reliability
atoms with qrel lift       -> higher reliability
atoms that add noise       -> lower reliability
```

The important change is that `reliability(atom)` is atom-local, not
source-local. That lets a highly useful SAE latent beat a weak token and lets a
precise token beat a broad latent.

### Why It Could Work

- It turns SAE latents into semantic tokens.
- It keeps the physical engine simple.
- It allows exact MaxScore/WAND with one upper-bound model.
- It can reuse the existing sparse-impact payload direction.

### Why It Could Fail

- BM25 impacts and SAE activations may not share a stable scale.
- SAE high-DF latents may create huge fanout even after IDF.
- Source-blind scoring may lose useful source-level debugging and policy
  control.
- If all useful SAE dimensions are broad, the atom namespace still opens too
  many documents.

### First Experiment

Add an offline scorer:

```text
single_atom_score =
    sum(token_atom_query_weight * token_atom_doc_impact)
  + sum(sae_atom_query_weight * sae_atom_doc_impact)
```

but with one shared calibration table:

```text
atom_id, df, idf, impact_tau, reliability
```

Compare against milestone-1 fixed-saturation BM25+SAE:

- Recall@20/100
- MRR@20
- posting fanout
- opened doc fraction
- source contribution distribution

Success condition:

```text
quality within 1-2 points of fixed-saturation BM25+SAE
and materially better bound/opened-doc behavior
```

## Route 2: Token-Latent Co-Activation Graph

### Hypothesis

Tokens and SAE latents should not merely coexist. They should translate into
each other through a corpus-derived co-activation graph.

Example:

```text
token:cuda               -> latent:gpu_acceleration
token:graph-neural-net   -> latent:gnn_training
latent:4310              -> token:cuda, token:gpu, token:kernel
```

This route is still index-light: it does not require training a new encoder.
It learns relations between existing token atoms and SAE atoms.

### Build Shape

Offline:

```text
for each document:
  collect lexical atoms
  collect SAE latent atoms
  update token-latent co-activation statistics

derive edges:
  token -> latent
  latent -> token
  token+latent -> mixed atom
```

At query time:

```text
initial query atoms
  -> graph expansion
  -> expanded query atom vector
  -> same postings traversal
```

At index time, two variants are possible:

1. Query-only expansion:

```text
index stores token atoms + SAE atoms
query expands token atoms into latent atoms and latent atoms into token atoms
```

2. Mixed atom materialization:

```text
index also stores mixed atoms such as token@latent
query can hit a mixed atom when both sides are active
```

### Edge Weighting

Good edge weights should not be raw co-occurrence counts. Raw counts will favor
broad, noisy atoms.

Better candidates:

```text
PMI(token, latent)
normalized PMI
chi-square / lift
qrel-aware lift when qrels exist
asymmetric P(latent | token) and P(token | latent)
```

The asymmetry matters:

```text
token -> latent
```

helps lexical queries gain semantic recall.

```text
latent -> token
```

helps semantic queries regain precise lexical anchors.

### Why It Could Work

- It uses corpus structure instead of only embedding geometry.
- It may reduce SAE broad-latent fanout by anchoring latents to lexical
  contexts.
- It can create more selective mixed atoms.
- It is easier to debug than a fully learned mixed encoder.

### Why It Could Fail

- Co-activation may mostly recover obvious lexical correlations.
- Mixed atoms can explode the dictionary and postings size.
- Query expansion can add noise and hurt MRR.
- It may overfit a corpus slice and generalize poorly.

### First Experiment

Build a token-latent graph over the current five-dataset artifacts:

```text
token min_df
latent min_df
edge min_count
edge score = normalized PMI or lift
top_edges_per_atom = 8..32
```

Evaluate:

```text
BM25
BM25+SAE fixed saturation
single atom namespace
single atom + query graph expansion
single atom + materialized mixed atoms
```

Success condition:

```text
same or better Recall@100 than fixed-saturation BM25+SAE
with lower SAE fanout or lower opened-doc fraction
```

The main signal to watch is not only quality. It is whether graph expansion
replaces broad SAE postings with more selective mixed evidence.

## Route 3: Mixed Sparse Encoder

### Hypothesis

The cleanest long-term design may be a trained encoder that outputs retrieval
atoms directly. The atoms are not necessarily tokens or SAE dimensions. They
are learned retrieval features whose training objective includes lexical and
semantic evidence.

This is the most disruptive route:

```text
text
  -> dense embedding
  -> lexical sketch
  -> mixed sparse encoder
  -> retrieval atoms
```

The atom dictionary may contain:

- copied lexical atoms for exact matching;
- SAE-like semantic atoms;
- mixed atoms that are not directly interpretable;
- field-sensitive atoms if we add field context later.

### Training Inputs

The encoder can consume:

```text
dense embedding
BM25 token sketch
token IDF sketch
optional field flags
```

The output is a sparse vector:

```text
top-k atom ids + weights
```

### Training Objective

A useful objective should include both retrieval quality and fanout control:

```text
loss =
    dense-teacher listwise loss
  + qrel supervised loss when available
  + lexical anchor preservation loss
  + candidate-budget penalty
  + atom DF / entropy regularization
```

Lexical anchor preservation matters. If the mixed encoder ignores exact tokens,
it becomes a sparse dense-vector clone and loses one of the main reasons BM25 is
useful.

Candidate-budget penalty matters because milestone 1 showed the main systems
problem: broad SAE latents open too many documents.

### Why It Could Work

- It can learn atoms that are more selective than raw SAE latents.
- It can preserve exact lexical anchors and semantic abstraction in one vector.
- It can make source-blind scoring natural.
- It offers the best chance to remove dense vector search from the online
  retrieval path.

### Why It Could Fail

- It needs training data and careful validation.
- It may overfit benchmark qrels.
- It can become hard to explain.
- It may require a larger model or online encoder than a simple SAE adapter.
- If the learned atoms are unstable, every model update is a full reindex.

### First Experiment

Do not start with a new large model. Start with a small adapter over existing
artifacts:

```text
input = concat(
    Snowflake dense embedding,
    compact lexical sketch,
    existing SAE top-k vector
)

output = top-k mixed atom vector
```

Teacher:

```text
dense top-k list
BM25+SAE fixed-saturation top-k list
available qrels
```

Metrics:

- Recall@20/100
- MRR@20
- atom fanout
- opened-doc fraction
- number of atoms per query/document
- overlap with lexical-only and SAE-only hits

Success condition:

```text
matches BM25+SAE quality
and reduces opened-doc fraction substantially
```

This route is the most likely to create a true next-generation index, but it
should come after Route 1 and Route 2 provide the baseline atom vocabulary and
fanout diagnostics.

## Comparison

| Route | Integration cost | Model risk | Explainability | Chance of better pruning | Best use |
| --- | ---: | ---: | ---: | ---: | --- |
| Single atom namespace | Low | Low | Medium | Medium | Fastest proof of source-blind scoring |
| Token-latent graph | Medium | Low-Medium | Medium-High | Medium-High | Test whether corpus structure can make SAE selective |
| Mixed sparse encoder | High | High | Low-Medium | High | Best long-term chance to replace dense retrieval |

## Recommended Order

### Step 1: Source-Blind Atom Scorer

Implement an offline source-blind atom scorer over current artifacts.

Question:

```text
Can atom-local calibration match fixed-saturation BM25+SAE quality?
```

If no, deep fusion is probably not ready. Keep source-aware fixed saturation.

### Step 2: Token-Latent Graph

Add graph expansion and optional mixed atom materialization.

Question:

```text
Can co-activation reduce broad SAE fanout while preserving semantic recall?
```

If yes, this may become the first SQL/native deep-fusion path because it does
not require training a new model.

### Step 3: Mixed Sparse Encoder

Train a small retrieval-aware mixed atom encoder.

Question:

```text
Can learned atoms beat raw SAE latents on the quality/cost frontier?
```

If yes, the native index should target generic evidence atoms, not BM25+SAE as
special source families.

## Milestone 2.5 Consolidated Update

The consolidated follow-up is saved in:

```text
sae-milestone25-consolidated-results-report.md
scripts/research_sae_milestone25_consolidated.py
results/sae/milestone25/consolidated/summary.md
```

The result narrows the route choice:

```text
head16_route1:
  Recall@100       0.7964
  MRR@20           0.6790
  postings touched 1381.7
  candidate docs   852.9

route1 full:
  Recall@100       0.7951
  MRR@20           0.6790
  postings touched 17655.8
  candidate docs   1980.9
```

The larger retrieval-aware selector training pass did not become the new
default. It helped `scifact`, but failed to generalize across `scidocs`,
`nfcorpus`, `arguana`, and `fiqa`. A safer selector that preserves original
high-impact SAE atoms reduced the loss, but still did not beat `head16_route1`.

This changes the recommended order:

1. Keep the source-blind atom scorer.
2. Promote impact-head or impact-ordered candidate generation to the first
   native execution target.
3. Treat learned selector weights as a research branch, not as the next SQL
   integration target.
4. If training continues, train new retrieval atoms or mixed atoms with an
   explicit candidate-budget objective instead of only reweighting existing
   SAE atoms.

## Milestone 3 Fusion/Training Update

The next pass is saved in:

```text
sae-milestone3-fusion-training-report.md
scripts/research_sae_milestone3_fusion_training.py
results/sae/milestone3/fusion-training/summary.md
```

It tested a native-like payload proxy and a pseudo-query-trained mixed
token-latent atom line.

The technical path converged further:

```text
base_head16:
  Recall@100       0.7964
  MRR@20           0.6790
  postings touched 1381.7
  candidate docs   852.9
```

This remains the first native target: source-blind evidence atoms, impact-head
candidate collection, and exact sparse rerank over the candidate pool.

The mixed atom training result is narrower:

```text
mixed_candidate_base_score_head8:
  Recall@100       0.7875
  MRR@20           0.6791

mixed_expand_candidate_base_score_head8:
  Recall@100       0.7879
  MRR@20           0.6790

mixed_score_head16:
  Recall@100       0.7945
  MRR@20           0.5657
```

Mixed atoms can be candidate-only boosters, especially for low-cost head8
profiles, but they are not safe as direct final-score atoms yet. Their scale is
not calibrated against base token and SAE atoms, and parent suppression does
not fix the MRR loss.

Updated implementation order:

1. Port the base evidence-atom impact-head payload first.
2. Keep mixed atoms out of the final scorer until calibration is solved.
3. If mixed atoms are used, use them only to widen candidates before base
   rerank.
4. Move future training toward retrieval-atom generation with explicit
   candidate coverage, fanout, and calibration losses.

## Milestone 4 Evidence Payload Update

The base evidence-atom payload is now implemented as an experimental binary
format:

```text
EATMH001
```

Artifacts:

```text
sae-milestone4-evidence-payload-report.md
scripts/research_sae_milestone4_evidence_payload.py
scripts/test_research_sae_evidence_payload.py
results/sae/milestone4/evidence-payload/summary.md
```

The payload stores one shared atom dictionary for BM25 token atoms and SAE
latent atoms, plus:

```text
impact-head lists:
  atom_id -> [(doc_ord, impact)]

doc-row sparse vectors:
  doc_ord -> [(atom_id, impact)]
```

The query path is:

```text
query atoms
  -> atom ids
  -> impact-head candidate union
  -> exact candidate rerank through doc-row sparse vectors
```

Five-dataset result:

```text
payload_head16:
  Recall@100       0.7964
  MRR@20           0.6790
  touched postings 1381.7
  candidate docs   852.9
  rerank doc terms 146385.1
```

The important implementation result is parity:

```text
EATMH001 binary payload doc-order parity with the in-memory reference = 100%
```

This confirms that the deep-fusion abstraction is no longer only a Python
scoring experiment. It now has a concrete read-only payload contract.

The remaining systems bottleneck is exact rerank cost. Since documents now
contain lexical token atoms as well as SAE atoms, doc-row exact rerank scans
many more terms than the earlier SAE-only v4 payload. The next implementation
step should therefore be:

1. PostgreSQL read-only resident function;
2. reusable query-local scan state or merge fallback;
3. doc-row compression;
4. only after that, mutable maintenance design.

## Milestone 5 Evidence C Reader Update

The standalone C reader is now implemented and measured:

```text
tests/research_sae_evidence_payload_reader.c
scripts/research_sae_milestone5_evidence_c_reader.py
sae-milestone5-evidence-c-reader-report.md
results/sae/milestone5/evidence-c-reader/summary.md
```

It reads the `EATMH001` payload plus an `EATMQ001` query/expected-answer file
and verifies exact doc-order parity against the Python payload.

Five-dataset result:

| Run | Exact ratio | Mean ms | Touched postings | Candidate docs |
| --- | ---: | ---: | ---: | ---: |
| `head8_scan` | 1.0000 | 0.1431 | 705.4 | 516.8 |
| `head16_scan` | 1.0000 | 0.2300 | 1381.7 | 852.9 |
| `head32_scan` | 1.0000 | 0.3485 | 2652.3 | 1268.1 |

`head16_scan` is roughly 33x faster than the Python payload reader on the
current five-dataset slice while preserving the same Recall@100 and MRR@20.

Three exact rerank strategies were tested:

```text
scan  = fastest current C path;
merge = scale-up fallback without an atom_count-sized query array;
seek  = slower and not the default direction.
```

This shifts the next bottleneck. The C traversal itself is not the immediate
blocker anymore. PostgreSQL residency, query-local memory reuse, and doc-row
compression are now the primary implementation tasks.

## Milestone 6 PostgreSQL Evidence-Atom Update

The `EATMH001` evidence-atom payload is now exposed through PostgreSQL:

```text
ii42_evidence_atom_query
ii42_evidence_atom_query_by_id
scripts/test_research_sae_evidence_atom_pg_generation.py
sae-milestone6-pg-evidence-atom-report.md
```

The SQL surface accepts atom ordinals and weights, runs impact-head candidate
generation plus exact doc-row scan rerank inside the extension, and returns
ranked `doc_ord`, optional `ctid`, score, and diagnostics.

Verification status:

```text
direct bytea query: passed
generation-table by-id query: passed
TID visibility continuation: passed
existing SBMXM resident smoke: still passed
```

The key limitation is now explicit: the by-id function fetches and decodes the
bytea generation on each call. That is correct for the read-only SQL contract,
but it is not the final performance model. The next implementation target is a
parsed payload cache keyed by generation table relation and generation id, with
invalidation on upsert/delete.

## Milestone 7 PostgreSQL Generation Cache Update

The per-query decode bottleneck has been removed for by-id generation reads:

```text
sae-milestone7-pg-generation-cache-report.md
scripts/research_sae_milestone7_pg_cache_benchmark.py
ii42_sae_generation_cache_clear
ii42_sae_generation_cache_state
```

The cache is backend-local and keyed by:

```text
generation table OID
generation_id
payload kind
row xmin revision
```

It supports both existing `SBMXM001` SAE generations and the new `EATMH001`
evidence-atom generations. Same-backend upsert/delete invalidates immediately;
cross-backend changes are detected by comparing the cached revision with the
current row `xmin`.

`scifact` `head16` benchmark over 50 queries:

| Path | Mean ms |
| --- | ---: |
| `direct_bytea` | 2.8116 |
| `by_id_cold_clear_each` | 1.4916 |
| `by_id_cached` | 0.5180 |

All paths kept exact doc-order parity. The next bottleneck is now the
query-local working set (`candidate_seen`, `candidate_docs`, `scores`,
`scored_seen`, and `query_weights`), which is still allocated and zeroed per
SQL call.

## Milestone 8 PostgreSQL Workspace And Exploration Update

The next bottleneck has been pushed forward, but the result is not a simple
replacement of the old path:

```text
sae-milestone8-pg-workspace-and-exploration-report.md
scripts/research_sae_milestone8_pg_exploration.py
```

The evidence-atom query path now uses an adaptive strategy:

```text
small payload:
  original palloc/zeroing path

larger payload:
  backend-local workspace
  epoch-marked candidate de-duplication
  touched-list query weight reset
  scored-doc ranking without scanning all doc ordinals
```

The adaptive split matters because the current five-dataset research payloads
are only about 2k documents. On that shape, full zeroing is cheap and mark
branches can dominate. Synthetic 10k-document expansion forces the workspace
path and shows the intended scaling behavior:

| Dataset | Docs | Cached ms | Candidates | Rerank doc terms | Direct->cached |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` x1 | 2,000 | 0.5184 | 788.9000 | 146,487.0667 | 5.5546x |
| `scifact` x5 | 10,000 | 0.7341 | 1,057.2333 | 195,137.1333 | 11.6811x |
| `fiqa` x1 | 2,000 | 0.5077 | 755.3000 | 113,341.6000 | 4.5819x |
| `fiqa` x5 | 10,000 | 0.6557 | 1,018.5000 | 152,530.9667 | 9.7723x |

All rows kept exact parity. This makes the next systems frontier more precise:

```text
candidate_docs
rerank_doc_terms
```

A first head-size sweep confirms that impact-head selection is the cleanest
immediate selectivity lever:

| SciFact head size | Cached ms | Candidates | Rerank doc terms |
| ---: | ---: | ---: | ---: |
| 8 | 0.4712 | 463.9333 | 86,069.2333 |
| 16 | 0.5265 | 788.9000 | 146,487.0667 |
| 32 | 0.6469 | 1,208.0667 | 224,995.4667 |

The deep-fusion question is therefore still open and still worth exploring
before productization. The SQL surface should stay experimental until we know
whether atom reliability, impact-head selection, mixed candidate-only atoms, or
doc-row compression can improve the recall/cost frontier further.

## M9-M13 Consolidated Exploration Update

The M9-M13 batch pushed the exploration beyond a single PostgreSQL latency
checkpoint:

```text
sae-m9-m13-exploration-report.md
scripts/research_sae_m9_m13_exploration.py
results/sae/m9-m13/exploration/summary.md
```

The quality/cost matrix confirms that the current best evidence-atom baseline
is still:

```text
h16_qb0_weight
Recall@100 = 0.7964
MRR@20     = 0.6790
candidate_docs = 852.8940
```

The strict selectivity frontier did not find a cheaper configuration within
0.005 Recall@100 of that row. `h8_qb0_weight` is still strategically
interesting because it cuts candidate and rerank work heavily, but the recall
drop is too large for the first exact default.

The strongest new systems signal is compact doc-row rerank:

```text
h16_qball_doc128
Recall@100 = 0.7937
MRR@20     = 0.6840
rerank_doc_terms = 106,998.4100
```

This keeps quality close while reducing rerank terms by about 27% versus the
full doc-row path. It suggests the next payload/layout experiment should be a
compact top-128 doc vector, not another round of query-budget heuristics.

The M13 gate remains closed for productization because no canonical
arxiv/pubmed/commons real-workload qrels were found. Continue the deep-fusion
research path before freezing SQL/API or mutable index design.

The route is now narrowed in:

```text
sae-converged-research-and-engineering-plan.md
```

The next work is no longer broad exploration across every idea. It is a
two-track convergence:

```text
research:
  M14 selectivity frontier 2
  learned/teacher-derived atom reliability
  token-latent and mixed atoms as candidate-only expansion

engineering:
  M15 compact doc-row rerank
  top-128 doc vector payload candidate
  C/PostgreSQL parity and repeated benchmark stability
```

Real workload qrels/proxy-qrels are the third gate for quality claims. Without
M16, the SQL model must remain read-only and experimental, but engineering
latency/fanout validation can still proceed on real corpora.

## M14-M17 Exploration Update

The M14-M17 pass is saved in:

```text
sae-m14-m17-exploration-report.md
scripts/research_sae_m14_m17_exploration.py
results/sae/m14-m17/exploration/summary.md
```

The result changes the next implementation priority:

```text
M14 selectivity frontier 2: failed
M15 compact doc-row rerank: passed
M16 real workload readiness: failed
M17 SQL model gate: blocked
```

The best selectivity row is still the original `h16_base_full`. The current
qrel/dense reliability selectors, graph expansion, and mixed candidate-only
expansion do not beat that frontier.

The systems breakthrough is instead `doc128`:

```text
h16_doc128
Recall@100 = 0.7937
MRR@20     = 0.6840
rerank_doc_terms = 106,998.4100
```

This passes the compact-rerank gate and becomes the next engineering target.
The next physical payload should therefore test:

```text
EATMH002 = compact top-128 doc-row evidence payload
```

Product-quality claims remain blocked until a real workload qrel/proxy-qrel
matrix exists. M20 can still run real arxiv/pubmed/commons efficiency tests to
measure latency, fanout, payload size, memory, cache behavior, and result-shape
sanity, but those tests must not be reported as Recall/MRR quality evidence
without labels.

## Native Index Implication

The native storage should not bake in `bm25` and `sae` as permanent scoring
families if this line works. It should store:

```text
atom_id
doc_ord / tid
impact
block or impact-head metadata
optional source/debug tag
```

The query API can accept:

```text
atom_ids[]
atom_weights[]
k
diagnostic_flags
```

BM25, SAE, SPLADE, BGE-M3 sparse, graph-expanded atoms, and mixed encoder atoms
then become producers of the same physical payload.

This is more general than `BM25+SAE`. The extension becomes a PostgreSQL-native
evidence-atom retrieval engine.

## Open Questions

1. Can atom-local calibration remove the need for source-level saturation?
2. Which atom reliability metric best predicts retrieval value?
3. Do mixed token-latent atoms reduce fanout or only increase storage?
4. Can graph expansion improve recall without hurting first-page ranking?
5. Can a mixed sparse encoder preserve exact lexical anchors?
6. How much online query encoding cost is acceptable for AI RAG and memory
   retrieval?
7. Can atom-level upper bounds be made tight enough for exact traversal?

## Decision Gate

Continue toward deep fusion only if at least one route shows:

```text
BM25+SAE-level quality
+ lower opened-doc fraction
+ source-blind or source-light scoring
```

If none of the three routes improves the systems frontier, return to:

```text
sae-milestone-1
```

and continue the fixed-saturation BM25+SAE native path.

## Milestone 2 First Results

The first three-route run is recorded in:

```text
scripts/research_sae_milestone2_deep_fusion.py
sae-milestone2-deep-fusion-results-report.md
results/sae/milestone2/deep-fusion/summary.md
```

Mean five-dataset result:

| Route | Recall@100 | MRR@20 | Cost signal |
| --- | ---: | ---: | --- |
| milestone-1 fixed saturation | 0.7934 | 0.6742 | baseline fanout |
| Route 1 scaled single atom | 0.7951 | 0.6790 | same fanout |
| Route 2 graph expansion | 0.7992 | 0.6090 | much higher fanout |
| Route 2 mixed atoms | 0.7954 | 0.6681 | same fanout |
| Route 3 learned budget 64 | 0.7927 | 0.6550 | slightly lower fanout |

Current decision:

- Continue Route 1 as the primary milestone-2 line.
- Keep only the selective mixed-atom part of Route 2.
- Stop the current global-reliability form of Route 3; replace it with a
  query-conditioned mixed sparse encoder if this route is reopened.

## Milestone 2 Follow-Up Results

The five follow-up directions are recorded in:

```text
scripts/research_sae_milestone2_followup_experiments.py
sae-milestone2-followup-experiments-report.md
results/sae/milestone2/followup-experiments/summary.md
```

The strongest result is no longer a new fusion formula. It is impact-head
candidate generation over Route 1 source-blind atoms:

| Run | Recall@100 | MRR@20 | Postings touched | Candidate docs |
| --- | ---: | ---: | ---: | ---: |
| Route 1 scaled atom | 0.7951 | 0.6790 | 17655.8 | 1980.9 |
| impact head 16 | 0.7964 | 0.6790 | 1381.7 | 852.9 |
| impact head 8 | 0.7856 | 0.6791 | 705.4 | 516.8 |

Current decision:

- Route 1 source-blind atoms remain the primary scoring abstraction.
- Impact-head candidate generation is the primary systems path.
- Atom-local calibration should next be applied to impact-head selection, not
  only final scoring.
- Retrieval-aware SAE atom training is still interesting, but the first global
  atom-reweighting version is not robust enough; the next model must be
  query-conditioned.
