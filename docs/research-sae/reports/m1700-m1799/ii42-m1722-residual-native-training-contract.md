# M1722 Residual-Native Balanced Posting Contract

## Corrected Evidence Boundary

M1720 and M1721 reject two post-hoc representations:

- an orthogonal dense subspace obtained from lexical covariance;
- a fixed rank-32 or global linear factorization of a hard rank-residual
  matrix.

They do not reject a high-dimensional nonlinear posting representation trained
on lexical residual retrieval utility. The prior final report overstated this
boundary. M1722 is authorized only because it changes the causal variable and
uses an explicit control; it is not a rank, ridge, threshold, or depth sweep.

## Research Question

M1600 already proved that a balanced 4,096-key document source has sufficient
candidate capacity. Its source oracle reached dense O@100 `1.000000` and
O@256 `0.992906` at `0.130539x` reads, while its trained dense-neighborhood
router reached only `0.824180/0.708773`. Therefore another source-capacity
oracle would add no information.

M1722 asks the missing causal question:

> Does training query-key compatibility only on dense evidence not already
> covered by BM25 make the useful posting action more observable from the
> frozen dense-root query vector?

The canary uses one frozen M1600 source and trains two otherwise identical
routers:

1. `dense_control`: the original dense-top256 key-utility teacher;
2. `residual`: the same teacher after removing documents already admitted by
   frozen BM25 top256.

Both variants use the same source, query vectors, rank-128 router,
initialization, optimizer, steps, read budget, validation rows, and checkpoint
rule. Only supervision differs. This makes a residual gain causally
interpretable.

## Product Shape

The intended runtime remains one physical inverted index and one accumulator:

```text
query text
  -> frozen lexical terms
  -> learned semantic query keys

document text
  -> frozen lexical terms
  -> balanced semantic document keys

lexical and semantic namespaces
  -> one posting map
  -> one candidate accumulator
```

The M1722A source replay uses exact dense scores only as an admission upper
bound, matching M1600. It cannot be promoted as a product scorer. A passing
M1722A only authorizes joint residual-native source/scorer training.

## Frozen Canary

- dense root: `BAAI/bge-base-en-v1.5` cached by M1600;
- train/validation: the disjoint M1600 4,000/500 MS MARCO text pools;
- frozen source: M1600 8 groups x 512 keys, one key per group/document;
- lexical publisher: BM25 with fixed `k1=0.9`, `b=0.4`, top256;
- semantic read budget: `0.15x` corpus documents;
- router: rank 128 residual over frozen source logits;
- training: 1,500 updates, batch 32, evaluation every 100 updates;
- seed: 1722;
- qrels, cross-encoder scores, positive labels, dataset IDs, and per-query
  thresholds: unused.

BM25 is materialized independently on each text pool. The cache records the
text source and dense-cache signatures. Dense and BM25 rankings are produced
before router training.

## Teacher And Objective

For query `q`, let `D256(q)` be frozen dense top256 and `B256(q)` be frozen
BM25 top256. The residual target is:

```text
R256(q) = D256(q) - B256(q)
```

The existing M1600 document postings define the available semantic keys. A
qrels-free greedy teacher chooses keys that cover uncovered target documents
per posting read, with 10x weight on documents whose original dense rank is
below 100. The residual teacher applies this procedure to `R256`; the control
applies it to all `D256`.

The router objective contains:

- listwise cross-entropy over teacher key utility;
- hard selected-key versus non-selected-key pairwise pressure;
- a small residual-output norm penalty.

Training loss never selects a checkpoint. Selection requires simultaneous
non-regression in unified O@100/O@256 and residual recovery at 100/256 on the
disjoint validation pool.

## Canary Measurements

For control and residual variants, record:

- teacher-key AUC and selected-key recall under the frozen source logits;
- unified dense O@100 and O@256 after unioning BM25 and semantic candidates;
- semantic recovery of dense documents absent from BM25;
- exact semantic posting reads, candidate union ratio, and maximum DF;
- fraction of the teacher-to-base gap captured by the trained checkpoint;
- training-depth curves through update 1,500.

The dense control is mandatory. A residual checkpoint is not new evidence if
the equal-capacity dense control improves as much or more.

## Scale Gate

M1722A authorizes the 10,000/1,000 full surface only if all conditions pass:

- residual teacher unified O@100 `>=0.95` and O@256 `>=0.90`;
- a trained residual checkpoint is selected;
- unified O@100 improves by at least `0.005` and O@256 by `0.002`;
- residual recovery improves by at least `0.01` at 100 and `0.005` at 256;
- the checkpoint captures at least 5% of the teacher gap at O@100 and O@256;
- the residual variant score exceeds the dense control by at least `0.002`;
- semantic reads remain at or below `0.15x`.

If the canary passes, M1722B repeats the unchanged experiment on the existing
10,000/1,000 M1600 pools for 3,000 updates. A second seed is required before
any architecture integration.

## Follow-On Training

Only a replicated full-scale pass authorizes a new source model. That model
must jointly learn semantic document assignments and query compatibility from
the residual teacher, because the frozen M1600 source was optimized for all
dense neighbors. It must retain:

- 4K or larger balanced semantic namespace;
- one physical lexical plus semantic posting map;
- direct max-DF and exact-read constraints;
- frozen BM25 publishing;
- disjoint cross-corpus validation before BEIR qrels are opened.

Direct additive scoring and native replay are separate gates after source
admission. Exact dense reranking, ANN, and qrels-selected policies cannot
promote a checkpoint.

## Stop Rules

- If the residual teacher source upper fails, stop the frozen source.
- If the residual target is no more observable than the dense control and
  training captures less than 5% of its gap, stop this router formulation.
- If deeper training lowers loss after hard overlap peaks, retain the selected
  earlier checkpoint and do not extend the schedule.
- Do not respond to failure with rank, learning-rate, loss-weight, BM25-k,
  seed, or threshold sweeps.
- Joint source training is allowed only after a canary signal that is stronger
  than the dense control, or after a new observability audit identifies a
  source-level mismatch that frozen routing cannot answer.

M1722 must end with a conclusion-bearing report that distinguishes teacher
capacity, query observability, training depth, product scoring, and native
index readiness.
