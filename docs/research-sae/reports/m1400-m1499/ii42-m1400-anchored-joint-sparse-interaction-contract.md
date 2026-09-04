# M1400 Anchored Joint Sparse Interaction Contract

Date: 2026-07-09
Status: active route reset

## Decision

M1400 closes the query-only generated-tail and post-hoc selector lineage at
M1338. The retained anchors are P1.3 and M549U. They are not the same
embedding surface: the local P1.3 native shared15 proof uses the
768-dimensional Snowflake sample root, while M549U is the canonical
1024-dimensional PPLX teacher. The new research line must change the
representation and scoring interface before it changes model size, training
depth, or selector features.

The hypothesis under test is:

> A P1.3 dense-faithful anchor plus jointly produced query/document sparse
> token representations and signed late interaction can preserve dense
> behavior while adding retrieval capacity that a frozen document index and a
> query-only delta cannot express.

This is a bounded falsification program. A failed capacity gate ends the line
before encoder training.

## Frozen Evidence

The following conclusions are immutable inputs to M1400:

1. P1.3 signed-dot scoring is the current native dense-faithful engineering
   baseline on its aligned Snowflake root.
2. M549U's deterministic compiler remains the canonical PPLX first-stage
   teacher. It is not interchangeable with the P1.3 shared15 root.
3. M1182 found that positive pair movement is joint-interaction dominated:
   query-only and document-only first-order effects were negative on average.
4. M615 found that most remaining under-ranked positives are dense misses.
5. M728 showed that lower surrogate loss does not imply a better dense gate.
6. M1330 proved that useful source mixtures exist under oracle choice.
7. M1331-M1338 failed to make that choice observable and safe at query time.

M1400 therefore forbids another query-only tail, threshold, source router,
atom blacklist, or post-hoc safety selector as the primary experiment.

## What Is New

M22 and M1020 used token hidden states, but pooled token activations into one
query or document sparse row before retrieval. M1400 does not repeat that
shape. It retains token groups until scoring and compares:

1. P1.3-style signed single-vector sparse dot;
2. token-derived joint single-vector sparse dot;
3. non-negative sparse MaxSim;
4. magnitude-routed signed MaxSim.

The exact Signed-MaxSim inner-product construction is retained as a parity
control. Token-level signed MaxSim is a separate capacity surface and must not
be described as exact dense parity unless measured parity proves it.

## Data Contract

### M1401 capacity audit

- The equal-budget capacity panel must use one frozen PPLX revision for every
  method. PPLX sentence roots and token states must come from the same model
  forward interface.
- `P1.3-style single sparse dot` in that panel means the P1.3 scoring shape
  applied to the same PPLX root. It is not a claim that the historical
  Snowflake P1.3 row has been reproduced.
- Historical P1.3 native metrics remain a separate external engineering
  anchor and must not be mixed into same-root capacity deltas.
- May use BEIR qrels only for reporting an oracle-augmented candidate surface.
- Must not fit parameters, select thresholds, or choose a method from test
  qrels.
- Dense-only and qrels-augmented candidate surfaces must be reported
  separately.
- The PPLX model revision and every input path must be recorded.

### M1402 and later training

- BEIR test qrels and test-derived action labels are forbidden in training.
- Initial training data must come from non-test query/document pairs,
  corpus-derived synthetic queries, and mined negatives.
- Every checkpoint must regenerate both query and document sparse geometry.
- Dataset-specific alpha, thresholds, source choices, and gates are forbidden.

## Capacity Metrics

M1401 reports, at equal or explicitly measured sparse cost:

- dense overlap at 10, 50, and 100;
- Recall@100, MAP@100, NDCG@10, and MRR@20;
- candidate upper bound;
- score correlation against the PPLX dense root;
- query atom occurrences;
- query and document unique atoms;
- candidate-local atom document frequency;
- PPLX sentence-root/token-pooling parity;
- candidate-local posting touches and touched-document fraction;
- exact Signed-MaxSim/sparse-dot parity error.

No capacity result is promoted from fewer than 25 held-out queries. Smaller
runs are wiring smokes only.

## Gates

### Gate A: representation capacity

At a comparable posting budget, signed late interaction must form a genuine
Pareto improvement over both single-vector sparse rows. A metric gain bought
only by near-full token or posting expansion is a failure.

### Gate B: frozen-root learning

M1402 may proceed only after Gate A. A trained checkpoint must:

- beat epoch 0 on held-out candidate-list retrieval;
- keep dense overlap and candidate upper bound inside their hard floors;
- avoid a recurring hard-row regression;
- keep measured fanout inside the selected capacity frontier.

### Gate C: shared15 LODO

Required deltas against the frozen native anchor are:

- Recall@100 at least `+0.002`;
- MAP@100, NDCG@10, MRR@20, and CUB non-negative;
- O@100 loss no worse than `-0.002`;
- no more than two small negative dataset rows;
- no oracle or qrels-derived inference feature.

## Training Shape After Gate A

The first trainable model keeps PPLX frozen and learns a shared sparse token
dictionary for queries and documents. The objective is a curriculum, not a
single weighted metric shortcut:

```text
MultiTopK reconstruction
+ active-support orthogonality
+ dense candidate-distribution distillation
+ query-document contrastive/listwise retrieval
+ DF-FLOPS and touched-posting control
```

The P1.3 path remains an exact anchor skip path. The learned late-interaction
channel is a residual inside the same model and index contract, not an
external reranker. Backbone unfreezing is forbidden until the frozen-root head
passes the full M1404 gate.

## Stop Rules

Stop this route when any of the following is established:

1. Signed late interaction does not beat the single-vector capacity frontier.
2. Training loss improves while held-out overlap or CUB repeatedly declines.
3. Epoch 0 remains the selected checkpoint after two objective corrections.
4. Gains require oracle action identity or test qrels at inference.
5. Fanout approaches a full scan.
6. Shared15 gains disappear after native document reindexing.
7. The same failure mechanism repeats twice without new observability.

If Gate A fails, retain P1.3 as the final engineering baseline and end the
trainable encoder line. If single-vector rows fail but signed late interaction
passes, close the single-vector claim and continue only the grouped posting
scorer.

## Required Artifacts

- `docs/research-sae/reports/m1400-m1499/ii42-m1401-signed-maxsim-capacity-report.md`
- `runs/m1401_signed_maxsim_capacity_v1/*.json`
- M1402 checkpoint and frozen-root training report, only after Gate A
- M1403 three-seed hard-row matrix, only after Gate B
- M1404 shared15 LODO native matrix
- final M1400-M1406 go/no-go report
