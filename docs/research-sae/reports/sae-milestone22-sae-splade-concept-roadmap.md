# SAE Milestone 22: SAE-SPLADE-Informed Concept Vocabulary Roadmap

Date: 2026-05-15

## Trigger

The `UBMXM001` PostgreSQL read-only payload is now SQL-visible through direct
`bytea` and by-id resident-table functions. That closes the first integration
checkpoint for a unified BM25+SAE physical payload:

```text
BM25 token heads
+ SAE atom heads
  -> one candidate union
  -> source-aware doc-vector rerank
  -> SQL-visible diagnostics
```

The next question is no longer whether a unified sparse payload can be queried
from PostgreSQL. It can. The next question is whether the atom generator should
remain a Snowflake-over-dense SAE teacher/student pipeline, or move closer to a
SPLADE-style text encoder whose output vocabulary is learned concept latents.

This milestone is informed by:

- [From Tokens to Concepts: Leveraging SAE for SPLADE](https://arxiv.org/abs/2604.21511)
- [II-Commons document page](https://commons.ii.inc/documents/arXiv%3A2604.21511)
- [SAE-SPLADE code repository](https://github.com/yzong12138/sae_splade)
- [SAE-SPLADE Vs Current Text-Atom Training Benefit Analysis](sae-splade-vs-current-training-benefit-analysis.md)
- [SAE Milestone 22 Exploration Report](sae-milestone22-exploration-report.md)
- [SAE Milestone 22 Token-Level Concept Training Report](sae-milestone22-token-level-training-report.md)

## Current Benefit Assessment

The immediate decision is not to replace the current text-to-atoms training
with ordinary SPLADE. The existing `naver/splade_v2_distil` baseline did not
beat the current BM25+SAE path on the five-dataset control. The paper is useful
because it suggests a better concept-vocabulary training shape:

```text
contextual token states
-> TopK SAE concept latents
-> SPLADE-style aggregation
-> retrieval fine-tuning with sparse-cost constraints
```

The expected benefit is therefore conditional:

- use SAE-SPLADE as a paper-aligned control for the next text-to-atoms model;
- keep the current full15 Snowflake-SAE teacher/student baseline as the quality
  bar;
- require physical sparse-cost reporting before promoting any concept encoder;
- do not move another learned sparse source into SQL unless it beats the
  current student or wins materially on candidate/posting cost.

The latest exploration checkpoint keeps that boundary. Current student atoms
remain close to the Snowflake-SAE teacher, ordinary SPLADE remains a negative
control, UBMX physical-cost reporting is available, and a product-shaped
unified `query_atoms` SQL function remains open. The new token-level
`token_lse` control is promising on quality, but only after increasing active
student atoms from 64 to 128 or 192. The first asymmetric sweep shows
`doc=128/query=96` keeps most active-128 ranking quality with fewer query atoms,
but it still needs physical sparse-cost gates before it can replace the current
pooled text student. A budget32 follow-up improves recall slightly but hurts
MRR/MAP, so budget16 remains the ranking baseline and budget32 is only a
recall-biased control. The explicit asym96 loss then improves the target
`doc=128/query=96` row across all quality metrics, but the first UBMX physical
smoke still trails the current student on Recall@100 while improving NDCG/MAP,
so candidate coverage under active8 traversal became the next bottleneck. The
budget8/asym96 follow-up recovers most of that active8 recall gap and becomes
the current best M22 checkpoint, but its payload and candidate count remain
higher than the current pooled student. Post-hoc clipping from the
doc128-trained budget8 checkpoint is now the practical deployment knob:
`doc=80/query=96` is the low-cost balance, `doc=88/query=96` is the best
ranking/cost balance, and `doc=96/query=96` is the recall-oriented point.
Query top8 DF penalty, raw candidate-exposure loss, and direct doc96 training
all lose too much ranking quality for their small or uncertain cost
improvements. The full15 deployment sweep confirms that doc88/query96 is the
best default point: it wins NDCG/MAP in both full-quality and active8 physical
payload checks, while doc80 remains the low-cost fallback and doc96 is only a
small recall-biased option. A wider active-budget sweep also tested query
`64/80/96` and document `80/88/96/128`; doc128 only wins tiny Recall/MRR
deltas and loses NDCG/MAP, so larger document rows are not a better default.
The teacher-aware layout loss is implemented, but the first five-dataset run
regressed ranking quality despite valid MPS execution and better support
overlap. Keep it as an ablation tool, not the mainline checkpoint.

## Paper Takeaways That Matter For This Project

### 1. Treat SAE Latents As The Output Vocabulary

The paper's central move is not to use SAE as a dense-vector compression layer.
It replaces SPLADE's backbone token vocabulary with an SAE-learned semantic
vocabulary. That directly supports our long-term target: one inverted sparse
engine whose atoms can represent both exact lexical evidence and semantic
concept evidence.

For us, this means `UBMXM001` should not be treated as only a payload for the
current Snowflake-derived teacher. It should become the test harness for a real
`text -> sparse atoms` encoder that emits concept atoms at query time.

### 2. Separate SAE Pretraining From IR Fine-Tuning

The paper uses a two-stage route:

```text
frozen PLM token states -> TopK SAE pretraining
trained SAE encoder + PLM -> sparse retrieval fine-tuning
```

It reports that joint SAE reconstruction and retrieval training was harmful,
which matches our own evidence that representation objectives and retrieval
objectives can fight each other. The next training path should keep a clean
pretraining stage, then fine-tune the encoder for retrieval with explicit
fanout/candidate-cost constraints.

### 3. TopK Is A Primary Cost Control, Not Just A Sparsity Detail

The paper finds that the TopK activation budget dominates FLOPs regularization
for the quality/cost frontier. It also shows that plain TopK SAE was competitive
with Hierarchical and Matryoshka variants for retrieval.

For us, this reinforces the current candidate-budget training result: query atom
budgets must be part of the model objective, not only an index-side clipping
knob. The next training run should expose `query_top_k`, `doc_top_k`, and
candidate-budget loss as first-class hyperparameters.

### 4. Add A QD-FLOPs-Style Planning Metric

The paper uses QD-FLOPs and an effectiveness-efficiency metric `E2` to compare
sparse retrievers. We should not copy their exact constants, but the metric
shape is useful:

```text
quality score - soft penalty for expected sparse interaction cost
```

Our equivalent should report both abstract and physical costs:

```text
query_active_atoms
avg_doc_active_atoms
estimated_qd_flops
sum_query_atom_df
actual_candidate_docs
actual_postings_read
rerank_doc_terms
```

This gives us a common yardstick across BM25, Snowflake-SAE teacher atoms,
student atoms, and future SAE-SPLADE-style concept atoms.

### 5. Layer Choice And Anisotropy Are Training Variables

The paper shows that applying SAE before the MLM transform head and using the
right hidden layer matters. It also uses anisotropy as a diagnostic for why some
layers produce less efficient sparse representations.

Our current Snowflake teacher is sentence-embedding based, so it does not expose
token-level layer choices. A serious concept-vocabulary path needs an
open-source token encoder where we can test layer choice. DistilBERT is the
paper-aligned control; a stronger BGE-style encoder can be a later experiment.

### 6. Latent Vocabulary Size Should Be Reopened

The paper uses `M = 2^16` latents after testing `2^15`, `2^16`, and `2^17`.
Our current 8192-latent teacher is much smaller. A larger latent namespace may
help semantic granularity, but only if QD-FLOPs and actual posting reads stay
bounded.

The next experiments should test at least:

```text
M = 8192, 16384, 32768, 65536
query k = 4, 8, 16
retrieval fine-tuning with candidate-budget loss
```

### 7. Multilingual/Policy Corpora Become A Stronger Motivation

The paper's multilingual finding is relevant to our policy corpus direction:
concept latents can overlap across translations more naturally than token
vocabularies. This does not prove quality for our policy workload, but it gives
a concrete diagnostic to add before labels exist:

```text
cross-language atom overlap
atom fanout by language
lexical-token overlap vs concept-atom overlap
```

### 8. Synonymy/Polysemy Diagnostics Are Useful Product Signals

The paper analyzes token-latent relationships with conditional probabilities to
identify synonym groups, polysemous tokens, and identity-like latents. This is a
useful debug layer for our system because it can explain why a concept atom is
broad, duplicated, or too lexical.

For RAG and memory retrieval, these diagnostics can help decide whether an atom
is safe for final scoring or should only be used as a candidate booster.

## Milestone 22 Scope

M22 is a planning and research milestone, not a productization milestone.

Goal:

```text
Turn SAE-SPLADE's concept-vocabulary lessons into a concrete next-generation
training and index-evaluation plan for the unified sparse payload.
```

Non-goals:

- no mutable index integration;
- no stable SQL/API freeze;
- no claim that dense retrieval can be removed;
- no production dependency on the SAE-SPLADE codebase before local benchmarks.

## Work Packages

### M22.1 Add Cost Metrics To The Matrix

Add a QD-FLOPs-like metric family to the existing benchmark reports:

```text
query_active_atoms
avg_doc_active_atoms
estimated_qd_flops
sum_query_atom_df
actual_candidate_docs
actual_postings_read
rerank_doc_terms
```

Gate:

```text
Every future quality row must carry both quality and sparse-cost metrics.
```

### M22.2 Build A Paper-Aligned SAE-SPLADE Control

Create a minimal local control path using an open-source token encoder:

```text
DistilBERT token states
-> TopK SAE pretraining
-> SPLADE-style max aggregation over concept latents
-> retrieval fine-tuning with BM25+SAE candidate-budget loss
```

Gate:

```text
The control first beats pure BM25 and the existing off-the-shelf SPLADE
baseline on the five-dataset matrix, then scales to full15 only if fanout stays
bounded.
```

Status:

```text
partial pass. Plain token-max aggregation failed, but token-level
length-normalized logsumexp plus candidate-budget training reached useful
ranking quality at active 128/192. The missing evidence is physical sparse
cost and a training objective that directly targets query/document asymmetric
active budgets.
```

### M22.3 Compare Against Current Snowflake-SAE Teacher

Run an apples-to-apples matrix:

```text
BM25
BM25 + Snowflake-SAE teacher atoms
BM25 + current text-student atoms
BM25 + SAE-SPLADE-style concept atoms
```

Gate:

```text
Concept atoms must either approach the Snowflake-SAE teacher, or clearly win on
latency/fanout enough to justify the quality gap.
```

Practical threshold:

```text
The concept encoder should reach within 0.01 Recall@100 and 0.02 MRR@20 of the
current text-student on the same subset, or show more than 30% lower sparse
cost at comparable quality.
```

### M22.4 Test Larger Latent Vocabularies Under Physical Cost

Test larger vocabularies only with cost gates attached:

```text
M = 8192, 16384, 32768, 65536
query k = 4, 8, 16
doc aggregation top-k / doc vector cap = 64, 128, 192
```

Gate:

```text
Do not promote a larger vocabulary if it only improves quality by opening more
postings or inflating doc vectors.
```

### M22.5 Add Runtime Query Atoms To PostgreSQL

The current `UBMXM001` SQL functions use embedded query rows. Product-shaped
read-only SQL needs runtime query atoms:

```sql
ii42_unified_payload_query_atoms(
    generation bytea,
    query_atoms int4[],
    query_weights real[],
    k int4,
    doc_tids tid[] DEFAULT NULL
)
```

and then a by-id variant.

Gate:

```text
Runtime-query SQL returns the same top-k as the embedded-query smoke for the
same query atom vector.
```

### M22.6 Add Concept Diagnostics

Add offline diagnostics for concept atoms:

```text
atom_df
atom_language_distribution
atom_top_tokens
P(token | atom)
P(atom | token)
synonym-like atoms
polysemy-splitting atoms
duplicated semantic atoms
```

Gate:

```text
The diagnostics identify high-fanout and duplicated atoms before they become a
production ranking risk.
```

### M22.7 Policy/Multilingual Diagnostic Probe

Use policy data as an unlabeled diagnostic corpus, not a quality benchmark yet:

```text
sample multilingual policy docs
encode with candidate concept model
measure cross-language atom overlap
compare with lexical-token overlap
inspect high-overlap and high-fanout atoms
```

Gate:

```text
No quality claim. Only decide whether concept atoms are promising enough for a
labeled policy retrieval set.
```

## Updated Engineering Order

The immediate engineering order becomes:

```text
1. keep doc88/query96 as the current default deployment point
2. keep teacher-aware layout training as an ablation, not a promoted checkpoint
3. use query-slice diagnostics to target semantic-heavy and short-query gaps
4. runtime-query UBMX SQL function after model-side evidence stabilizes
5. only then decide on parsed UBMX cache and product-shaped API
```

This order keeps SQL integration moving, but prevents us from prematurely
freezing an API around embedded query rows or the current Snowflake-teacher
training assumptions.

## Success Criteria

M22 succeeds if it gives a defensible answer to these questions:

1. Can a token-level concept encoder match or approach the Snowflake-SAE teacher
   while staying fully sparse and index-native?
2. Does TopK/candidate-budget training control actual posting reads better than
   index-side clipping alone?
3. Does a larger latent vocabulary improve quality without unacceptable fanout?
4. Can runtime query atoms be passed into the PostgreSQL read-only payload with
   exact parity against embedded-query reference rows?
5. Do concept diagnostics show interpretable synonymy/polysemy behavior and
   identify dangerous high-fanout atoms?

## Decision After M22

If M22 passes, the next phase should promote concept-vocabulary training as the
main model path for the unified sparse engine.

If M22 fails, keep the current Snowflake-SAE teacher/student path and focus on
runtime-query SQL, parsed payload cache, and real-workload quality sets before
attempting another model redesign.
