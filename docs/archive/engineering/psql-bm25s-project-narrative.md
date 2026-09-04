# Building `psql_bm25s` with an Engineering Harness

> **Historical project narrative.** This article records an earlier BM25-focused
> phase and its dated performance evidence. Its APIs, architecture language,
> and benchmark claims are not the current product contract. Start with the
> [documentation map](../../README.md), [architecture](../../architecture-and-design.md),
> and [testing contract](../../testing-and-validation.md).

This is not the formal technical report. It is the project story:
why `ii42` exists, how it evolved, and how we used an AI coding
agent together with a performance-first engineering loop to push it
into a shape that is actually useful for AI knowledge bases and RAG
systems.

<img width="360" height="360" alt="II-42 logo" src="../../logo.png" />

If you want the formal material, start with the
[technical report](../../technical-report-psql_bm25s.md), the
[architecture and design notes](../../architecture-and-design.md), and the
[performance benchmark reference](../../performance/README.md). This post is
the narrative behind those documents.

## The Problem We Started From

The project did not start from "we should build another BM25
extension." It started from an operational problem.

We had already lived through the constraints of `bm_search`, and one
limitation mattered more than almost anything else for production AI
systems: it did not fit cleanly into the kind of PostgreSQL-native
operational model we wanted. In particular, the lack of a clean path
for normal primary-replica synchronization made it a poor fit for the
kind of knowledge-base and RAG service we actually wanted to run.

For an AI retrieval layer, the database story matters as much as the
scoring formula:

- the index has to survive restarts
- it has to behave correctly under replication
- it has to coexist with normal PostgreSQL operations
- it has to support mutable data without turning every change into a
  rebuild event

That was the real origin of `ii42`: not "let's reimplement BM25
for fun," but "let's build a lexical retrieval plugin that behaves like
a real PostgreSQL component for AI workloads."

## Why We Chose `bm25s`

Once we decided to rebuild the retrieval layer properly, we looked for
the right algorithmic foundation.

That led us to [`bm25s`](https://github.com/xhluca/bm25s). The appeal
was immediate. Compared with older BM25 implementations, `bm25s`
showed a much more performance-conscious formulation:

- eager sparse scoring
- query-time work shifted toward sparse access and accumulation
- exact top-k retrieval without pretending approximate shortcuts are
  "close enough"

For our purposes, this was the right starting point. We did not want a
generic search engine bolted onto PostgreSQL. We wanted a PostgreSQL
extension that kept the exact BM25-family semantics aligned with
`bm25s`, but reworked storage, access method design, maintenance, and
query surfaces for PostgreSQL itself.

That is the key distinction behind the project:

- `bm25s` gave us the algorithmic shape
- PostgreSQL gave us the systems constraints
- `ii42` is the result of reconciling both

The formal write-up of that foundation is in
[Section 2 of the technical report](../../technical-report-psql_bm25s.md#2-algorithmic-foundation-from-bm25s).

## What We Were Actually Trying to Build

Very early, the project goals became unusually clear:

1. Keep BM25-family ranking exact.
2. Make the extension feel native to PostgreSQL.
3. Make it suitable for AI knowledge bases and RAG serving.
4. Do not accept poor operational behavior in exchange for benchmark
   headlines.

That design pressure produced the mainline shape of the extension:

- a PostgreSQL-native access method
- canonical exact retrieval APIs such as
  `ii42_query_ids(...)` and
  `ii42_query_tokens(...)`
- SQL convenience surfaces for developer ergonomics
- mutable index maintenance that does not collapse under row churn
- crash recovery and physical replication compatibility

The current mainline maintenance model is not a "wait for manual
VACUUM" design. PostgreSQL's normal cleanup lifecycle still provides
the delete signal where the heap makes that necessary, but index
convergence is handled by the maintenance machinery: automatic policies
record durable debt, the shared-preload deployment path runs a
timer-based catch-up supervisor, and deployments without preload can use
lightweight touch wakeups or explicit maintenance helpers. The important
operational point is that expensive rebuild work is kept out of normal
foreground reads and writes whenever the chosen consistency policy
allows it.

Those ideas are reflected throughout the documentation:

- [Architecture and Design](../../architecture-and-design.md)
- [API Reference](../../api-reference.md)
- [Index Policy](../../index-policy.md)
- [Technical Report](../../technical-report-psql_bm25s.md)

## Fusion and Hybrid Retrieval

For AI knowledge bases, plain single-field lexical search is rarely the
whole query shape. A production search request often has titles,
abstracts, body text, metadata filters, semantic vector matches, and a
final reranking stage. The mainline extension is designed so BM25 can
participate in that shape instead of being sealed inside one opaque
single-field search box.

There are two related capabilities.

Fusion is about combining lexical signals. A document can be spread
across several homogeneous text-like columns and still be indexed as one
BM25 document. When the application needs column identity, the
field-aware path preserves field-scoped evidence so that title, abstract,
body, or other document regions can contribute differently. Applications
can also keep separate BM25 indexes for separate fields and combine
their ranked evidence explicitly. The common design goal is the same:
keep BM25 exact, keep field structure visible, and let PostgreSQL return
one ordered lexical candidate set.

Hybrid retrieval is about combining different retrieval types inside the
database. In that model, `ii42` owns the lexical candidate path and
the candidate-fusion layer, while a vector extension such as pgvector or
VectorChord owns dense-vector indexing. PostgreSQL can then build BM25
candidates, vector candidates, and ordinary filtered candidate sets in
one query plan, normalize or rank those source-local signals, de-duplicate
documents, and return one database-side top-k result for later AI
reranking or direct use.

This is the retrieval shape we care about for RAG: vector search can
carry semantic recall, BM25 can preserve exact lexical and field-aware
evidence, and the database can do the first-stage aggregation before
the application spends model time on reranking.

See the detailed docs for the individual surfaces:

- [Multicolumn Fusion Indexes](../../multicolumn-indexes.md)
- [Multi-Field Search](../../multi-index-fusion.md)
- [Field-Aware Indexes](../../field-aware-indexes.md)
- [Hybrid Vector/BM25 Search](../../hybrid-search.md)
- [Hybrid Fusion Engine](../../hybrid-fusion-engine.md)

## The Engineering Harness

This project became much more interesting in the middle and late
stages, because we stopped thinking about "coding features" and started
thinking in terms of an engineering harness.

The harness had a simple rule:

> every serious performance idea had to survive a loop of
> hypothesis -> implementation -> benchmark -> analysis -> keep or
> reject

That sounds obvious, but it changes how a project evolves. Instead of
accumulating optimization folklore, we forced the work through a repeat
process:

- identify the hot path
- design a narrowly targeted change
- run before/after measurements
- keep the change only if the signal is real
- archive the result, including failed experiments

This is what made later project phases productive instead of chaotic.

We built:

- benchmark matrices
- local and cloud BEIR runs
- focused query-only harnesses
- profiling loops
- raw-data archives
- status matrices
- branch-level result summaries

That work now lives in the public documentation and raw-data archive:

- [Performance and Benchmarks](../../performance/README.md)
- [Performance data index](../../performance/data/README.md)

## Learning from Existing Search Systems

One of the most useful shifts in the project was to treat existing
search systems as engineering input, not as marketing targets.

We read code and documentation from strong nearby systems such as
[VectorChord-BM25](https://github.com/tensorchord/VectorChord-bm25),
[ParadeDB](https://github.com/paradedb/paradedb), and
[ParadeDB's Tantivy fork](https://github.com/paradedb/tantivy). The
goal was not comparison marketing; it was to understand what good
systems did well and translate the useful parts into our own
PostgreSQL-native constraints.

That work pushed us to look much harder at:

- bounded collectors
- candidate narrowing
- top-k specialization
- phrase and verified-query pruning
- execution-path specialization instead of generic elegance
- SQL-level composition for lexical, field-aware, and vector-assisted
  retrieval

The important part is that we converted outside lessons into a
structured experimental program inside our own extension. We repeatedly
asked:

- does this idea fit a PostgreSQL-native BM25 extension?
- does it preserve our exactness contract?
- does it actually win on our benchmarks?
- is it worth the complexity it adds?

That last question mattered a lot. Several ideas were implemented,
measured, and rejected. That is a feature of the process, not a sign of
failure.

## Using an AI Coding Agent as Part of the Loop

The project also became a serious experiment in AI-assisted software
engineering.

The useful insight was not "an agent can write code." That is the least
interesting part.

The more important insight was that a coding agent can become a stable
part of a disciplined engineering loop if the harness is strong enough.
In practice, that meant:

- the agent could read the codebase and inspect hot paths
- the agent could implement targeted changes
- the agent could run the benchmarks and tests
- the agent could archive the results
- the agent could compare branches and reject its own bad ideas
- the agent could continue unattended for long-running evaluations

What mattered was not free-form generation. What mattered was
constraint:

- exact acceptance criteria
- benchmark-backed decisions
- explicit rollback of bad experiments
- written records of what was tried and why

That turns "vibe coding" into something much closer to an engineering
discipline.

In our case, the agent was especially effective in the repetitive but
high-value parts of the loop:

- generating narrow hypotheses from profiling output
- implementing focused changes in C and SQL
- running A/B measurements repeatedly
- turning branch outcomes into durable documentation
- coordinating long unattended benchmark runs and system checks

The result was not automatic genius. The result was a tighter feedback
loop.

## The Performance Story

The project now has a much stronger benchmark story than it had early
on, because performance work was pushed through the harness rather than
through intuition alone.

The current public cross-engine reference is the PG18 `15 x 5` BEIR
matrix:

- upstream Python `bm25s`
- `ii42 ids`
- `ii42 text[]`
- ParadeDB `pg_search`
- TensorChord `vchord_bm25`

See:

- [Performance and Benchmarks](../../performance/README.md)
- [Technical Report, current PG18 matrix](../../technical-report-psql_bm25s.md#82-current-pg18-cross-engine-matrix)

The current scale chart is:

![QPS vs dataset scale](../../performance/reports/pg18-qps-vs-dataset-scale-2026-04-02.svg)

And the build-time view is:

![Index build time vs dataset scale](../../performance/reports/pg18-build-vs-dataset-scale-2026-04-02.svg)

At the time of writing, the published PG18 matrix shows:

- `ii42 ids` median throughput at `3.97x` upstream `bm25s`
- `ii42 text[]` median throughput at `3.93x` upstream `bm25s`
- `ii42 ids` total build time at `0.31x` upstream
- `ii42 text[]` total build time at `0.52x` upstream

That is the public result. But the more interesting internal result was
how we got there.

During the research-driven optimization campaign, we studied mature
search systems, ran a long sequence of local branch comparisons, and
then refreshed the cloud matrix. The closeout showed two things:

1. some optimizations were clearly worth keeping
2. many plausible-sounding ideas were not

The analysis notes include both:

- the local `main` vs optimization-branch results
- the later cloud rerun that refreshed the published `ii42`
  cells

This is exactly the kind of material that an engineering harness makes
possible: not just success stories, but a durable map of what did and
did not pay off.

## A New Development Style

The broader lesson from this project is that AI-native software
development does not have to mean sloppy software development.

In fact, the opposite can be true.

If you give an AI coding agent:

- a strong benchmark harness
- reproducible tests
- explicit acceptance criteria
- a performance archive
- permission to iterate and reject ideas

then you get a development style that is unusually good at:

- wide exploration
- rapid implementation
- disciplined pruning
- preserving useful history

That is the development mode we ended up using here. It is closer to a
continuous engineering lab than to ordinary feature work.

The agent was not a replacement for architecture. It was a force
multiplier for disciplined iteration.

And that turned out to be a very good fit for a project like this one:

- systems-heavy
- performance-sensitive
- benchmark-driven
- full of ideas that need to be tested rather than admired

## Where to Go Next

If you want the formal system description:

- [Technical Report](../../technical-report-psql_bm25s.md)
- [Architecture and Design](../../architecture-and-design.md)

If you want the practical API surface:

- [API Reference](../../api-reference.md)
- [Query Semantics](../../query-semantics.md)
- [Multi-Field Search](../../multi-index-fusion.md)
- [Multicolumn Fusion Indexes](../../multicolumn-indexes.md)
- [Field-Aware Indexes](../../field-aware-indexes.md)

If you want the benchmark and raw-data trail:

- [Performance and Benchmarks](../../performance/README.md)
- [Performance data index](../../performance/data/README.md)

And if you want the short version of this whole post, it is this:

`ii42` was not built by chasing a single benchmark trick. It was
built by combining a clear database-native retrieval goal, a strong
algorithmic foundation from `bm25s`, and an engineering harness that
made AI-assisted iteration measurable, rejectable, and therefore useful.
