# psql_bm25s Technical Report

Date: 2026-05-07

## Abstract

`psql_bm25s` is an independent PostgreSQL extension for exact
BM25-family lexical retrieval. The project is inspired by the `bm25s`
technical report and its central observation: much of BM25 retrieval
cost can be moved from query time to index time by eagerly computing
per-term document contributions and storing them in sparse form. This
project keeps that
core ranking contract where it matters most, then extends it into a
database-native setting with native PostgreSQL index storage, SQL
query surfaces, crash recovery, physical replication, and maintenance
mechanisms for mutable workloads. This report describes the algorithmic
foundation drawn from the public BM25S formulation, the
PostgreSQL-specific design choices in `psql_bm25s`, and the main
architectural change relative to a static-corpus model: a more
maintainable storage and maintenance design for frequent `INSERT`,
`UPDATE`, and `DELETE`. It
also describes the mainline fusion surfaces used by production search
systems: multicolumn BM25 indexes, explicit multi-index score fusion,
field-aware query surfaces, and SQL-level composition with external
vector indexes.

## 1. Introduction

Lexical retrieval remains important because it is exact, transparent,
training-free, and operationally predictable. The `bm25s` report shows
that a Python implementation can become dramatically faster if it
reformulates BM25 retrieval around eager sparse scoring rather than
computing token contributions only at query time.

That result is important, but a database extension faces an additional
set of constraints that are mostly outside the scope of the original
report:

- indexes must live inside PostgreSQL storage
- writes must interact with transactions
- restart and crash recovery must preserve index state
- physical replication must reproduce index state correctly
- SQL users need explicit query surfaces and operational controls
- row churn must be handled without collapsing read performance

`psql_bm25s` is designed around those database constraints while
keeping the BM25 scoring model aligned with the `bm25s` family.

## 2. Algorithmic Foundation from BM25S

The `bm25s` report describes two core implementation ideas that are
directly relevant here.

We use the following notation:

- $C$: indexed collection
- $N = |C|$: number of indexed documents
- $D \in C$: a document
- $|D|$: document length
- $L_{\mathrm{avg}}$: average document length
- $Q = (q_1, \dots, q_{|Q|})$: tokenized query
- $\mathrm{df}(t)$: document frequency of token $t$
- $\mathrm{tf}(t, D)$: term frequency of token $t$ in document $D$
- $m_t$: postings length of token $t$, equal to
  $\mathrm{df}(t)$ in the sparse index
- $L = \sum_{D \in C} |D|$: total corpus token count
- $U = \sum_{D \in C} u_D$: total number of distinct term-document
  pairs, where $u_D$ is the number of unique terms in document $D$

### 2.1 Eager sparse scoring

Instead of waiting for a query to arrive and then computing all token
contributions on demand, `bm25s` precomputes the score contribution of
each indexed term for each document at index time. Terms that do not
occur in a document contribute zero for the sparse variants, so the
result can be stored sparsely.

At retrieval time, a query is evaluated by:

- selecting the sparse rows corresponding to query terms
- summing those rows across the query dimension
- applying top-k selection to the resulting document score vector

This changes the hot path from repeated BM25 arithmetic toward sparse
row access plus summation.

### 2.2 Sparse storage and top-k retrieval

The `bm25s` report uses sparse matrix storage and specifically notes
the practicality of CSC-style storage for efficient access. The report
also emphasizes that top-k selection does not need a full sort; a
selection strategy such as partial partitioning is sufficient for the
exact top-k set.

These two ideas are central to the present project:

- sparse storage remains the core representation
- exact top-k remains the default contract
- the retrieval hot path is still designed around sparse access rather
  than SQL/SPI execution

### 2.3 Variant support and non-occurrence adjustments

The `bm25s` report also describes how sparsity extends naturally to
Lucene, Robertson, and ATIRE style variants, and how BM25L and BM25+
can still be supported exactly by shifting scores relative to a
non-occurrence baseline. That observation matters because it means the
project can preserve exact variant semantics without giving up sparse
retrieval.

`psql_bm25s` keeps this alignment for the supported variants:

- `robertson`
- `lucene`
- `atire`
- `bm25l`
- `bm25+`

### 2.4 Scoring equations

Following the `bm25s` report, the total query score is:

$$
B(Q, D) = \sum_{i=1}^{|Q|} S(q_i, D)
$$

with

$$
S(t, D) = \mathrm{IDF}(t, C) \cdot \mathrm{TFC}(t, D)
$$

For Lucene-style BM25, the current implementation uses:

$$
\mathrm{IDF}_{\mathrm{lucene}}(t, C) =
\log\left(1 + \frac{N - \mathrm{df}(t) + 0.5}{\mathrm{df}(t) + 0.5}\right)
$$

$$
\mathrm{TFC}_{\mathrm{lucene}}(t, D) =
\frac{\mathrm{tf}(t, D)}
{\mathrm{tf}(t, D) + k_1
\left(1 - b + b \cdot \frac{|D|}{L_{\mathrm{avg}}}\right)}
$$

This is the normalized Lucene-style term factor used by the current
code path. The ATIRE branch keeps the more familiar explicit
$(k_1 + 1)$ numerator.

For BM25L and BM25+, sparsity is preserved by shifting against a
non-occurrence baseline. Define:

$$
S_{\theta}(t) = S(t, \varnothing)
$$

where $\varnothing$ is an empty document. Then define the
differential score:

$$
S_{\Delta}(t, D) = S(t, D) - S_{\theta}(t)
$$

and rewrite the total score as:

```math
B(Q, D) =
\sum_{i=1}^{|Q|} S_{\Delta}(q_i, D)
+ \sum_{i=1}^{|Q|} S_{\theta}(q_i)
```

The second term depends only on the query, not on the document, so it
does not change the ordering of documents within that query.

**Proposition 1.**
For a fixed query `Q`, replacing the per-term score `S(t, D)` by the
differential score `S_Δ(t, D)` and adding back the query-only constant
sum of non-occurrence baselines preserves the exact ranking order of
all documents.

**Proof.**
Let:

```math
K(Q) = \sum_{i=1}^{|Q|} S_{\theta}(q_i)
```

Then for every document `D`:

```math
B(Q, D) = B_{\Delta}(Q, D) + K(Q)
```

where:

```math
B_{\Delta}(Q, D) = \sum_{i=1}^{|Q|} S_{\Delta}(q_i, D)
```

For any two documents `D_a` and `D_b`:

```math
B(Q, D_a) \ge B(Q, D_b)
\iff
B_{\Delta}(Q, D_a) + K(Q) \ge B_{\Delta}(Q, D_b) + K(Q)
```

Subtracting the same constant `K(Q)` from both sides yields:

```math
B_{\Delta}(Q, D_a) \ge B_{\Delta}(Q, D_b)
```

So the ordering is unchanged. This is why BM25L and BM25+ can remain
exact while still using sparse differential storage.

### 2.5 Complexity from eager sparse scoring

In the current implementation, term counting inside one document sorts
the document token IDs and collapses adjacent equal values. Therefore,
if the corpus contains documents $D_1, \dots, D_N$, the document-
local counting cost is:

$$
\sum_{i=1}^{N} O(|D_i| \log |D_i|)
$$

The passes that compute corpus statistics and write CSC-style sparse
storage are linear in the number of distinct term-document pairs:

$$
O(U)
$$

So the practical index-build complexity is:

$$
O\left(\sum_{i=1}^{N} |D_i| \log |D_i| + U\right)
$$

and the storage complexity is:

$$
O(U + N + |V|)
$$

where $|V|$ is vocabulary size.

For a query $Q$, the sparse accumulation work over postings is:

$$
O\left(\sum_{t \in Q'} m_t\right)
$$

where $Q'$ is the filtered in-vocabulary query.

However, the exact current implementation also allocates and zeroes a
full score buffer of length $N$, so the practical query-time score
construction cost is:

$$
O\left(N + \sum_{t \in Q'} m_t\right)
$$

The current exact top-k routine uses a bounded heap of size $k$, so
selection is:

$$
O(N \log k)
$$

Therefore the practical canonical retrieval complexity is:

$$
O\left(
N + \sum_{t \in Q'} m_t + N \log k
\right)
$$

**Proposition 2.**
The bounded-heap top-k selection used by the current implementation
returns the exact top-$k$ set under the implementation's total order:
descending score, then ascending document ID as the deterministic
tie-break. It does not need to fully sort all $N$ documents.

**Proof sketch.**
Scan the document scores once while maintaining a min-heap of size at
most $k$.

- While the heap contains fewer than $k$ elements, insert the next
  score.
- Once the heap is full, compare each new score $s$ with the heap
  minimum $h_{\min}$. If $s \le h_{\min}$, discard it. If
  $s > h_{\min}$, remove $h_{\min}$ and insert $s$.

After processing any prefix of the score array, the heap contains the
best $k$ items seen so far under that total order. This is immediate by
induction on the number of processed scores: the invariant is trivially
true before
the heap is full, and once full, any discarded score is no larger than
the smallest retained score, so it cannot belong to the top-$k$ of
the processed prefix. When scores tie, the comparator prefers the
smaller document ID, which makes the retained set deterministic. Any
inserted score strictly improves the retained
set by replacing its current minimum. Therefore, after the full scan,
the heap contains exactly the global top-$k$ scores. A final sort
of those $k$ heap elements produces the exact ranked result.

## 3. Project Scope and Design Goals

The goal of `psql_bm25s` is not to reproduce the original Python API.
The goal is to preserve the retrieval contract while embedding it into
PostgreSQL as a first-class index access method.

The main design goals are:

1. exact BM25-family semantics on the canonical retrieval path
2. PostgreSQL-native durability and physical replication
3. SQL-visible indexing and retrieval
4. high read performance as the top priority
5. more maintainable behavior under row churn than a purely static
   offline artifact

The design explicitly accepts that write-side maintenance may be more
expensive if that is required to protect exact reads and stable query
throughput.

## 4. System Architecture

The implementation is organized as four layers.

### 4.1 BM25 core

Files:

- `src/psql_bm25s_core.c`
- `src/psql_bm25s_core.h`

Responsibilities:

- BM25-family scoring logic
- corpus statistics
- sparse postings and score storage
- exact top-k retrieval
- bounded and subset-aware collector paths for ordered scans
- exact-score rescoring support for mutable maintenance

This is the layer that stays closest to the eager sparse scoring
retrieval model.

### 4.2 Stable binary storage

Files:

- `src/psql_bm25s_storage.c`

Responsibilities:

- portable little-endian serialization
- storage for the internal `psql_bm25s_index` SQL type
- storage for payloads inside PostgreSQL index relations

The mainline design extends the earlier single-payload model with the
exact statistics needed for more maintainable mutable workloads:

- per-posting term frequency
- per-document document length
- per-term document frequency

These are not a change in retrieval semantics. They are extra state
needed to keep exact scoring possible while maintenance becomes more
database-friendly.

### 4.3 PostgreSQL bindings

Files:

- `src/psql_bm25s_pg.c`

Responsibilities:

- SQL type I/O
- standalone builder helpers
- standalone top-k helpers
- utility functions around the serialized storage type

The extension currently supports five indexed source-column types:

- `int4[]`
- `text[]`
- `varchar[]`
- `text`
- `varchar`

These fall into two groups:

- pretokenized inputs supplied by the caller:
  - `int4[]`
  - `text[]`
  - `varchar[]`
- scalar text inputs tokenized at the index boundary:
  - `text`
  - `varchar`

This keeps one BM25 storage and scoring core while still allowing
ordinary SQL schemas to start from raw text columns. The pretokenized
paths remain the more explicit and more performance-oriented contract
when the application already owns tokenization or token-ID assignment.

### 4.4 PostgreSQL access method

Files:

- `src/psql_bm25s_am.c`

Responsibilities:

- `CREATE INDEX USING psql_bm25s`
- canonical exact retrieval over stored indexes
- ordered scans through `<=>`
- predicate scans through `@@`
- candidate-set planning for boolean, verified, and phrase queries
- candidate-restricted ranking and deferred ranking for large subsets
- multicolumn fusion and field-aware query helpers
- phrase and grouped-query pruning before heap verification
- mutable-workload maintenance
- maintenance introspection and recommendation

For `int4[]`, `text[]`, and `varchar[]`, the access method consumes the
stored token or token-ID stream directly. For scalar `text` and
`varchar`, it normalizes and tokenizes at the index boundary, then feeds
the same internal token-stream model to the BM25 core. Phrase and
verified text paths on scalar columns re-tokenize the heap value using
the index-bound text options before final verification; array-backed
paths avoid that retokenization step.

The same boundary rule is also used for multicolumn fusion indexes.
Homogeneous `text[]`, `varchar[]`, `text`, and `varchar` index columns
can be fused into one BM25 document in index-column order. For scalar
fusion, each source column is tokenized with the index text options
before the fused token stream is appended to the shared BM25 build path.

## 5. PostgreSQL Adaptation of the Retrieval Model

### 5.1 Native index relation

Unlike an offline matrix stored outside the database, `psql_bm25s`
stores its state inside a PostgreSQL index relation. This makes the
index participate in ordinary PostgreSQL durability mechanisms:

- WAL logging
- crash recovery
- restart persistence
- physical replication

This is the main structural step from a fast retrieval library toward a
database index.

### 5.2 Canonical retrieval APIs

The extension defines two canonical exact retrieval surfaces:

- `psql_bm25s_query_ids(...)`
- `psql_bm25s_query_tokens(...)`

These functions are the clearest definition of the exact BM25 contract
in this project and are the primary performance path. Other SQL
surfaces exist for usability, but the `rowset` functions are the
right reference point for exactness and benchmarking.

Even when an index is built over scalar `text` or `varchar`, the index
still lowers the source column to the same internal token-stream model.
Raw-text search helpers exist for ergonomics, but the ranking core
remains token- and corpus-statistics-driven.

This separation is the design boundary: richer operator surfaces are
useful, but the system still needs one explicit canonical retrieval
contract.

### 5.3 SQL convenience surfaces

On top of the canonical path, the extension adds:

- raw-query retrieval
- grouped boolean parsing
- prefix handling
- bounded phrase verification
- document-match predicates through `@@`
- ordered scan integration through `<=>`
- tokenization, normalization, stemming, and highlighting helpers

These surfaces are designed so that convenience features do not replace
the exact canonical path as the semantic center of the system.

SQL-visible ranked scans and operator ergonomics follow established
PostgreSQL search-interface patterns. In `psql_bm25s`, those surfaces
are attached to the native access-method path instead of a SQL- or
planner-heavy retrieval core.

### 5.4 Fusion and hybrid SQL composition

The mainline extension supports two BM25 fusion patterns.

The first pattern is a multicolumn fusion index. A homogeneous
multicolumn index over `text[]`, `varchar[]`, `text`, or `varchar`
columns can be treated as one BM25 document. The indexed columns are
fused in index-definition order, scalar text columns are tokenized with
the index text options, and the resulting token stream enters the same
BM25 build path as a single-column document. This is a storage and
query-surface convenience; it does not change the BM25 scoring formula.

The second pattern is explicit score fusion across separate indexes.
Functions such as `psql_bm25s_fusion_query(...)` and
`psql_bm25s_fusion_query_fields(...)` let an application retrieve
field-specific candidates from independent BM25 indexes, apply
query-time weights, and return one ranked result set. This keeps
field weighting visible in SQL and preserves the canonical retrieval
contract inside each participating index.

For `field_aware = true` multicolumn indexes, one BM25 payload preserves
field-scoped internal terms and can be queried with query-time field
weights. This is still a PostgreSQL-native BM25 index, not a separate
BM25F engine and not multiple hidden sub-indexes.

These lexical fusion surfaces also make BM25 usable in hybrid
BM25/vector retrieval systems. `psql_bm25s` does not implement dense
vector indexing; that role belongs to pgvector, VectorChord, or another
vector access method. The extension's contribution is that BM25
retrieval returns ordinary SQL-visible candidate rows and scores. A
single SQL statement can therefore combine:

- relational filters
- BM25 candidates from `psql_bm25s`
- semantic candidates from a vector index
- SQL-side normalization, gating, or weighted score fusion
- a final ordered candidate set for application-level or AI reranking

This is especially important for AI and RAG workloads. Vector search
often supplies semantic recall, while BM25 supplies exact lexical and
field-aware evidence. Keeping both signals composable in SQL lets the
database perform candidate construction before later model-based
reranking.

The detailed user-facing behavior is documented in
[Multi-Column Fusion Indexes](multicolumn-fusion-indexes.md),
[Multi-Field Search](multi-field-search.md), and
[Field-Aware Indexes](field-aware-indexes.md).

### 5.5 Canonical query complexity inside PostgreSQL

The PostgreSQL embedding preserves the same native sparse scoring core,
but adds database-specific fixed costs:

- SQL argument unmarshalling
- relation and cache validation
- MVCC visibility checks
- tuple materialization for SQL return values

Those costs are intentionally kept outside the ranking core. The
canonical `rowset` APIs therefore still follow the same
algorithmic structure as the standalone core:

$$
O\left(
N + \sum_{t \in Q'} m_t + N \log k
\right)
$$

plus PostgreSQL-specific constant factors for executor interaction and
result materialization.

### 5.6 Execution specialization without semantic drift

The formulas above define the retrieval contract. The current project
does not change those formulas in order to get speedups. The newer
performance work instead specializes the execution paths that decide
which documents reach ranking and how much ranking work is actually
performed.

The main practical patterns are:

- bounded collectors on ordered and deferred-ranking paths
- candidate-restricted ranking when filters or grouped queries already
  expose a narrow candidate set
- narrower boolean planning for common `MUST`, `MUST OR`, and positive
  AST shapes
- phrase and verified-query paths that prune candidate sets before
  verification instead of ranking broad positive supersets

These changes matter because PostgreSQL convenience surfaces are not all
the same shape. A small phrase query, a filtered ordered scan, and a
full-corpus exact `rowset` lookup all preserve the same BM25-family
semantics, but they should not necessarily pay the same planning or
ranking costs.

So the design rule is:

- keep the canonical BM25 score definition fixed
- narrow candidate sets as early as correctness allows
- use bounded ranking paths when the query shape already imposes a
  natural bound

This is the main execution-design refinement that emerged after the
initial mainline implementation.

The practical execution lesson is that the largest wins came from
bounded collectors, earlier candidate narrowing, and query-shape-aware
execution, not from changing the BM25 formulas themselves.

This distinction explains a recurring benchmark pattern: the extension
remains strong on larger or heavier workloads, while fixed database
overheads become more visible on lighter, very high-QPS workloads.

## 6. Mainline Enhancement: Maintainable Mutable Indexes

The most important architectural enhancement over a static eager sparse
storage model is mutable-workload maintenance.

### 6.1 Problem

The original eager sparse idea is strongest when the corpus is stable.
Inside PostgreSQL, however, indexes must survive frequent:

- `INSERT`
- `UPDATE`
- `DELETE`

If every change forced a full rebuild immediately, operational cost
would be too high. If changes were ignored, the index would stop
behaving like a real database index.

### 6.2 Exact-stats payload

To support more flexible maintenance without changing BM25 semantics,
the payload stores enough exact statistics to support exact rescoring
and exact overlays:

- posting-level term frequency
- document length
- term-level document frequency

This is the key storage extension that turns a static sparse payload
into something that can support exact maintenance decisions.

### 6.3 Persisted maintenance metadata

The index metapage tracks maintenance state such as:

- pending writes
- pending deletes
- rebuild count

This metadata lets the index expose its state through SQL and survive
restart or replication with the rest of the relation.

### 6.4 Threshold-bounded deferred maintenance

The mainline index can run in either the manual consistency policy or
automatic maintenance policies. In automatic policies, maintenance can
be bounded by reloptions such as:

- `auto_rebuild_threshold`
- `auto_rebuild_delta_bytes`
- `auto_rebuild_churn_ratio`

The key idea is to defer some maintenance work when the pending state
is still small enough to preserve exact reads through a bounded overlay.

For stale-tolerant knowledge-base workloads, the same maintenance model can be
configured with an opt-in query-first eventual policy:

- `consistency = 'eventual'`
- `auto_rebuild_threshold`
- `auto_rebuild_delta_bytes`
- `query_overlay_max_records`
- `query_overlay_max_bytes`

This policy keeps foreground queries from triggering unbounded rebuild work.
Queries use record- and byte-bounded overlays when cheap and otherwise return
base-index results until automatic or explicit maintenance converges the index.
The overlay limits are query budgets only; background rebuild pressure is
controlled by the rebuild threshold and optional delta-byte threshold so large
resident indexes do not rebuild just because a small tail exceeded the inline
query budget.

The recommended automatic deployment path loads `psql_bm25s` through
`shared_preload_libraries`. In that mode, a timer-based generic catch-up
supervisor wakes bounded workers, each worker maintains at most one due
index, and advisory locks prevent duplicate work on the same index.
Deployments without shared preload can still use lightweight touch
wakeups or explicit maintenance helpers. PostgreSQL `VACUUM` remains
part of the normal delete-cleanup lifecycle, but corpus-statistics
convergence is owned by the maintenance path rather than by manual
VACUUM as a synchronization mechanism.

The concrete `CREATE INDEX ... WITH (...)` parameters are documented in
[Index Parameters](index-parameters.md).

The broader decision to make mutable behavior policy-bounded and
explicitly database-facing follows from the database constraint that
maintenance strategy and read performance cannot be treated as
independent concerns.

### 6.5 Exact base-plus-delta overlays

The project extends the static payload model with exact bounded
overlays for:

- inserted rows
- updated rows
- delete tombstone state from PostgreSQL heap-cleanup paths

Canonical reads remain exact by combining:

- the persisted base payload
- bounded deferred delta state
- visibility-aware delete handling

This keeps the read path exact without forcing immediate full rebuilds
for every small mutation.

### 6.6 Operational introspection

The extension exposes maintenance state and policy helpers, including:

- `psql_bm25s_index_details(regclass)`
- `psql_bm25s_index_policy_recommend(regclass, profile text)`
- `psql_bm25s_index_refresh(regclass)`
- `psql_bm25s_index_maintain(regclass)`
- `psql_bm25s_index_try_maintain(regclass)`
- `psql_bm25s_index_maintain_due(max_indexes integer DEFAULT 1)`

This is an explicitly database-oriented addition. The intent is to make
index maintenance observable and tunable rather than hidden.

For query-first eventual indexes, the non-blocking maintenance path stages a
replacement payload before taking the final swap lock. The swap is conditional:
if an in-flight writer exists or the lock is busy, the scheduler retries later.
Append-only delta records that arrive while the replacement is being built are
carried forward after the swap. Non-tail-compatible metapage changes still
discard the staged payload. This keeps the long heap scan and BM25 build out of
foreground query and write paths while preserving a valid previous base index.

That operational emphasis follows from a database-facing maintenance
model: operational systems need clear maintenance-state visibility
rather than opaque background behavior.

### 6.7 Exactness of the mutable overlay design

The mutable-maintenance layer can be stated as a simple correctness
proposition.

**Proposition 3.**
If the persisted base payload represents collection
$C_{\mathrm{base}}$, the delta area represents all pending inserted
or updated documents, and the tombstone/replacement bookkeeping removes
all deleted or superseded documents, then the overlay index built from
those components is equivalent to rebuilding from scratch over the
current visible collection $C_{\mathrm{current}}$.

**Proof sketch.**
The overlay construction reconstructs the base corpus, appends every
pending delta document, and excludes any tuple marked deleted or
replaced by a newer visible version. The resulting document multiset is
exactly the visible corpus. The overlay index is then rebuilt using the
same deterministic BM25-family build routine and the same parameter
tuple. Therefore the resulting corpus statistics, sparse payload, and
query scores are exactly those of a fresh rebuild over
$C_{\mathrm{current}}$.

This is why the maintenance design is described as an enhancement in
maintainability rather than a change in scoring semantics.

### 6.8 Complexity of the mutable enhancements

The mutable-maintenance additions introduce two kinds of cost:

1. incremental write bookkeeping
2. occasional exact overlay or consolidation work

#### 6.8.1 Incremental write cost

For an inserted or updated document $D$, the deferred write path
stores a delta record containing the new token array plus heap-TID
metadata. At the logical level, this bookkeeping is linear in the
document representation:

$$
O(|D|)
$$

This is substantially cheaper than forcing an eager full rebuild over
the corpus:

$$
O\left(\sum_{i=1}^{N} |D_i| \log |D_i| + U\right)
$$

#### 6.8.2 Overlay construction cost

When deferred maintenance remains within configured bounds, the first
exact read may build an overlay from:

- the persisted base payload
- the persisted delta records
- the tombstone/replacement set

Let:

- $N_b$: number of base documents
- $U_b$: base sparse nonzero count
- $L_b$: total token count reconstructed from the base payload
- $N_{\Delta}$: number of pending delta documents
- $L_{\Delta}$: total token count of delta documents

The overlay path must:

1. load deferred state
2. reconstruct the base corpus from the sparse payload
3. exclude deleted or replaced documents
4. rebuild an exact index on the resulting visible corpus

The base reconstruction step is not free. In the current code it
allocates explicit document arrays from `doc_lengths`, then walks the
CSC postings and expands each posting by its stored term frequency.
That contributes at least:

$$
O(N_b + U_b + L_b)
$$

The subsequent exact rebuild over the visible overlay corpus costs:

```math
O\left(
\sum_{D \in C_{\mathrm{overlay}}} |D| \log |D|
+ U_{\mathrm{overlay}}
\right)
```

So a safer practical bound for the full overlay path is:

```math
O\left(
N_b + U_b + L_b
+ \sum_{D \in C_{\mathrm{overlay}}} |D| \log |D|
+ U_{\mathrm{overlay}}
\right)
```

where $C_{\mathrm{overlay}}$ is the visible corpus after applying
deltas and tombstones.

In practice, the rebuild term is usually the dominant one for ordinary
document lengths, but the reconstruction term is still part of the real
cost and should be stated explicitly. This is exactly why the design
bounds deferred maintenance by policy. Exact overlays are correct, but
they must remain bounded to stay operationally useful.

#### 6.8.3 Consolidation policy as a bound

The reloptions

- `auto_rebuild_threshold`
- `auto_rebuild_delta_bytes`
- `auto_rebuild_churn_ratio`

do not change BM25 semantics. They only bound how much deferred state
the system is willing to absorb before consolidating. One way to read
them mathematically is:

- tuple threshold bounds $N_{\Delta}$
- byte threshold bounds serialized deferred payload size
- churn ratio bounds deferred state relative to the base size

So the policy layer converts a potentially unbounded maintenance
problem into a bounded optimization problem.

### 6.9 Storage overhead of exact-stats support

Compared with a minimal sparse payload containing only:

- sparse scores
- document indices
- CSC pointers

the enhanced payload adds:

- one term-frequency integer per sparse nonzero
- one document-length integer per document
- one document-frequency integer per vocabulary term

Asymptotically this does not change the storage class:

$$
O(U + N + |V|)
$$

It increases the constant factors, but not the order. This is an
important design property: the project pays extra space for exact
mutable maintenance, but it does not leave the sparse-index regime.

## 7. Correctness, Durability, and Replication

The design treats PostgreSQL durability as a first-class requirement.

Current mainline behavior includes:

- persisted index pages in PostgreSQL storage
- WAL-logged index updates
- restart-safe maintenance metadata
- crash-recovery coverage
- physical replication coverage

The project also includes stress and smoke validation around:

- restart
- crash recovery
- physical replication
- concurrent maintenance behavior

This is a major difference from treating the retrieval structure as an
external artifact managed outside the database.

## 8. Performance Model

The project keeps a similar high-level performance philosophy to eager
sparse BM25 retrieval:

- do expensive scoring work ahead of query time where possible
- keep the hot retrieval path in native code
- preserve exact top-k as the default behavior

At the same time, the database setting introduces overheads that do not
exist in a standalone retrieval library:

- executor and function-call overhead
- SQL marshalling and tuple materialization
- MVCC visibility checks
- maintenance coordination

The implementation therefore separates performance concerns into two
categories:

1. canonical read performance on `rowset` retrieval
2. write/maintenance cost under row churn

The project benchmarks both dimensions in the repository documentation.
Read performance remains the primary optimization target; maintenance is
designed to be more flexible, but not at the expense of collapsing the
exact read path.

### 8.1 Current benchmark reference

The current public cross-engine benchmark reference is the refreshed PG18
`15 x 5` BEIR matrix published on `2026-04-02`. It compares:

- Python reference implementation `bm25s`
- `psql_bm25s ids`
- `psql_bm25s text[]`
- ParadeDB `pg_search`
- TensorChord `vchord_bm25`

Third-party engine names are included to identify the measured paths and
make the benchmark reproducible. The numbers depend on the measured
versions, configuration, hardware, workload, and query settings; they
should not be read as general claims about those projects outside this
specific benchmark scope.

Run shape:

- all 15 BEIR datasets used in the BM25S benchmark set
- Google Cloud `us-east4-a`
- `n2-standard-16`
- PostgreSQL `18`
- uploaded dataset cache
- `top_k = 1000`
- base matrix: `2026-03-31` full `15 x 5`, one dataset-engine task per VM
- refresh matrix: `2026-04-02` `psql_bm25s ids/text[]` refresh run on the same
  GCP PG18 shape

The public matrix is intentionally anchored to the pretokenized input
paths:

- `psql_bm25s ids` uses `int4[]`
- `psql_bm25s text[]` uses `text[]`

Scalar `text` and `varchar` source-column support is part of the
extension design, but it is not the basis of the current cross-engine
headline table.

The rolled-up current matrix was checked against the retained raw cells
in:

- `docs/performance/data/raw/pg18-beir-current-matrix-2026-04-02/`

The refreshed matrix keeps `45/75` Python reference implementation
`bm25s` / `pg_search` / `vchord_bm25` cells from the stable `2026-03-31`
PG18 run and replaces `30/75` `psql_bm25s` cells with the `2026-04-02`
refresh run. All `75/75` dataset-engine cells matched exactly on:

- `stats`
- `build_ms`
- `query.count`
- `query.qps`

This matrix is the current public benchmark reference for this
repository.

### 8.2 Current PG18 cross-engine matrix

Current query-throughput matrix, with dataset size and QPS:

| Dataset | Docs | Queries | Python reference implementation `bm25s` QPS | `psql_bm25s ids` QPS | `psql_bm25s text[]` QPS | `pg_search` QPS | `vchord_bm25` QPS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 8,674 | 1,406 | 1158.34 | 1402.63 | 1112.01 | 115.94 | 78.77 |
| `climate-fever` | 5,416,593 | 1,535 | 3.04 | 57.78 | 50.75 | 2.84 | 5.25 |
| `cqadupstack` | 457,199 | 13,145 | 111.56 | 443.13 | 438.42 | 13.99 | 60.51 |
| `dbpedia-entity` | 4,635,922 | 467 | 3.47 | 128.19 | 91.19 | 5.21 | 23.66 |
| `fever` | 5,416,568 | 123,142 | 3.15 | 97.56 | 80.15 | 5.62 | 12.13 |
| `fiqa` | 57,638 | 6,648 | 810.51 | 1409.52 | 1186.41 | 17.76 | 190.57 |
| `hotpotqa` | 5,233,329 | 97,852 | 4.16 | 55.40 | 49.86 | 3.42 | 9.31 |
| `msmarco` | 8,841,823 | 509,962 | 1.61 | 96.67 | 82.13 | 4.44 | 18.20 |
| `nfcorpus` | 3,633 | 3,237 | 3155.35 | 3373.94 | 3326.96 | 1132.17 | 1252.75 |
| `nq` | 2,681,468 | 3,452 | 10.55 | 174.34 | 176.69 | 6.28 | 21.96 |
| `quora` | 522,931 | 15,000 | 90.56 | 637.98 | 619.64 | 13.26 | 154.36 |
| `scidocs` | 25,657 | 1,000 | 1203.09 | 1835.85 | 1614.92 | 17.89 | 367.04 |
| `scifact` | 5,183 | 1,109 | 2964.86 | 2557.47 | 2240.18 | 500.04 | 629.42 |
| `trec-covid` | 171,332 | 50 | 210.50 | 191.94 | 154.48 | 8.75 | 75.66 |
| `webis-touche2020` | 382,545 | 49 | 240.36 | 82.97 | 74.10 | 8.14 | 86.04 |

Scale-versus-throughput view:

![QPS vs dataset scale](performance/reports/pg18-qps-vs-dataset-scale-2026-04-02.svg)

Summary readout:

- Median query-throughput ratios versus Python reference implementation `bm25s` are
  `3.97x` for `psql_bm25s ids`, `3.93x` for `psql_bm25s text[]`,
  `0.54x` for `vchord_bm25`, and `0.17x` for `pg_search`.
- Dataset counts at or above Python reference implementation `bm25s` are `12/15` for
  `psql_bm25s ids`, `11/15` for `psql_bm25s text[]`, `7/15` for
  `vchord_bm25`, and `3/15` for `pg_search`.

Index construction should be read alongside query throughput, because
the build side determines how expensive bulk loads, refreshes, and
repeatable deployments are. The next matrix reports index-construction
time only; it excludes query execution and orchestration overhead.

Current build-time matrix:

| Dataset | Docs | Queries | Python reference implementation `bm25s` build | `psql_bm25s ids` build | `psql_bm25s text[]` build | `pg_search` build | `vchord_bm25` build |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 3,633 | 3,237 | 167ms | 82ms | 113ms | 254ms | 326ms |
| `scifact` | 5,183 | 1,109 | 224ms | 99ms | 150ms | 326ms | 449ms |
| `arguana` | 8,674 | 1,406 | 318ms | 101ms | 168ms | 376ms | 448ms |
| `scidocs` | 25,657 | 1,000 | 974ms | 395ms | 633ms | 490ms | 1.10s |
| `fiqa` | 57,638 | 6,648 | 1.88s | 672ms | 1.06s | 705ms | 1.32s |
| `trec-covid` | 171,332 | 50 | 6.67s | 2.65s | 4.61s | 2.86s | 3.80s |
| `webis-touche2020` | 382,545 | 49 | 21.18s | 9.84s | 16.51s | 10.65s | 13.39s |
| `cqadupstack` | 457,199 | 13,145 | 15.68s | 5.83s | 9.29s | 7.21s | 19.56s |
| `quora` | 522,931 | 15,000 | 5.80s | 788ms | 1.06s | 936ms | 1.69s |
| `nq` | 2,681,468 | 3,452 | 1.2m | 23.71s | 40.80s | 31.46s | 55.57s |
| `dbpedia-entity` | 4,635,922 | 467 | 1.7m | 26.37s | 45.05s | 40.73s | 1.9m |
| `hotpotqa` | 5,233,329 | 97,852 | 1.8m | 30.79s | 50.38s | 41.97s | 1.9m |
| `fever` | 5,416,568 | 123,142 | 2.6m | 52.83s | 1.6m | 1.2m | 2.7m |
| `climate-fever` | 5,416,593 | 1,535 | 2.7m | 52.74s | 1.5m | 1.2m | 2.5m |
| `msmarco` | 8,841,823 | 509,962 | 3.2m | 56.06s | 1.5m | 1.2m | 1.7m |

Scale-versus-build-time view:

![Index build time vs dataset scale](performance/reports/pg18-build-vs-dataset-scale-2026-04-02.svg)

Build-side readout:

- Median per-dataset build-time ratios versus Python reference
  implementation `bm25s` are `0.34x` for `psql_bm25s ids` and `0.56x`
  for `psql_bm25s text[]`.
- Total build-time ratios versus Python reference implementation
  `bm25s` are `0.31x` for `psql_bm25s ids` and `0.52x` for
  `psql_bm25s text[]`.
- Dataset counts at or below Python reference build time are `15/15` for
  `psql_bm25s ids`, `15/15` for `psql_bm25s text[]`, `12/15` for
  `pg_search`, and `7/15` for `vchord_bm25`.
- Build-time, query-throughput, and the index-size matrix in the
  performance reference should be read together because each path makes
  a different operational trade-off.

Total build-time summary:

- Python reference implementation `bm25s`: `848046.35 ms`
- `psql_bm25s ids`: `262955.79 ms`
- `psql_bm25s text[]`: `443975.35 ms`
- `pg_search`: `356944.25 ms`
- `vchord_bm25`: `739014.63 ms`

Current relevance-quality matrix:

The quality matrix is a PG18 relevance validation run over the same 15 BEIR
datasets. It measures `NDCG@10`, `MAP@100`, `Recall@100`, and
`Precision@10` with `top_k = 100`.

This table uses qrels-bearing queries only. That means the evaluated
query count can be smaller than the full query count shown in the QPS
table.

All five comparison engines were available for this relevance validation
run: Python reference implementation `bm25s`, `psql_bm25s ids`, `psql_bm25s text[]`,
`pg_search`, and `vchord_bm25`.

The primary chart is an absolute-score heatmap rather than a
dataset-scale line chart. Each cell is the metric value for one engine
on one dataset. Bold cells are within `0.001` of the best engine for
that dataset and metric.

![Quality score heatmap](performance/reports/pg18-quality-score-heatmap-2026-05-06.svg)

For the complete per-dataset metric table, see the
[performance benchmark reference](performance/README.md#quality-matrix).

Quality readout:

- All five engines sit in a close relevance band on this BM25 quality
  matrix. Average `NDCG@10` ranges from `0.3976` to `0.4019` across
  the compared engines.
- `psql_bm25s ids` and `psql_bm25s text[]` remain quality-neutral exact
  PostgreSQL paths in this run. Their largest absolute metric
  difference from the Python reference implementation is below
  `0.0030`, while their engineering cost profile is covered by the QPS,
  build-time, and index-size data in the performance reference.
- `pg_search` and `vchord_bm25` are also competitive on relevance in
  this quality validation run, but they have different throughput and
  storage trade-offs in the main performance matrix.
- These quality metrics do not replace the QPS, build-time, or
  index-size data in the performance reference. They only show that the
  compared engines are retrieving similarly relevant top-100 candidates
  under the BEIR qrels.

## 9. Current Limitations

The present design still has boundaries.

- Canonical exact retrieval is strongest on the explicit `rowset`
  APIs. SQL convenience layers are broader but not always equally cheap.
- `<=>` only matches the exact BM25 ordering when PostgreSQL is
  executing a real `psql_bm25s` index scan.
- Post-delete cleanup still follows PostgreSQL's heap lifecycle, but
  exact corpus-statistics convergence is handled by automatic or
  explicit index maintenance. Operators should keep autovacuum and the
  `psql_bm25s` maintenance worker path healthy rather than treating
  manual `VACUUM` as the synchronization primitive.
- The current text processing stack is practical and configurable, but
  it is not intended to replace a full external search-text ecosystem.
- Hybrid BM25/vector retrieval is a SQL composition pattern. Dense
  vector indexing is intentionally delegated to vector extensions such
  as pgvector or VectorChord.
- Large-query-set Python reference implementation runs can dominate
  total benchmark wall-clock even after the PostgreSQL engines have
  already completed.

## 10. Conclusion

`psql_bm25s` should be understood as a PostgreSQL-native extension that
implements BM25-family eager sparse retrieval in a PostgreSQL-native
access method.

The key retrieval idea is:

- eager sparse scoring with exact BM25-family semantics

The main project-specific contribution is:

- turning that retrieval model into a durable, replicated,
  SQL-addressable, and more maintainable PostgreSQL index

`psql_bm25s` is not a wrapper around `bm25s`, and this repository does
not vendor or copy source code from the Python reference implementation
`bm25s`. It is a database-oriented system design that preserves the BM25
contract while adding the storage, maintenance, and operational
properties that a PostgreSQL extension must have.

## 11. Related Work and Benchmark Scope

This report keeps the main technical narrative focused on this project's
retrieval model, storage design, maintenance strategy, fusion surfaces,
and measured benchmark position.

The technical foundation is the BM25S retrieval formulation described in
the references below. Other PostgreSQL search extensions appear in this
report as public benchmark comparison targets where direct measurements
are useful for understanding `psql_bm25s` in context.

## 12. References

- BM25S technical report:
  <https://arxiv.org/abs/2407.03618>
- BM25S codebase:
  <https://github.com/xhluca/bm25s>
- `psql_bm25s` project repository:
  <https://github.com/Intelligent-Internet/psql_bm25s>
- `psql_bm25s` performance benchmark reference:
  <https://github.com/Intelligent-Internet/psql_bm25s/tree/main/docs/performance>
