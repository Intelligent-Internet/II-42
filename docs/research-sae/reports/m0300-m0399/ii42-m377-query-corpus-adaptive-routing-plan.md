# II-42 M377 Query/Corpus Adaptive Routing Plan

## Motivation

M376 should not be promoted as-is.

It showed that `trec-covid` contains enough target-distribution signal to fix
the M375 zero-shot scorer failure, but adapting to a named dataset is not a
clean scientific result. It risks becoming benchmark-specific optimization.

The correct next step is to learn an adapter from observable query/corpus
features. The adapter must not know the dataset name, must not use heldout
qrels, and must not choose hyperparameters per dataset.

## Boundary

Allowed:

- query embedding statistics;
- candidate pool statistics;
- posting-list statistics for active query coordinates;
- corpus-level distribution statistics computed from documents and postings;
- scorer confidence, score histogram, score gaps, and agreement between fixed
  scorers;
- dense teacher labels on training queries from source corpora;
- dense teacher labels on non-eval calibration queries only if the protocol is
  framed as online/corpus calibration and evaluated on disjoint future queries.

Not allowed:

- dataset id or one-hot dataset features;
- per-dataset hyperparameter choice;
- per-dataset model promotion selected by qrel metrics;
- using target eval queries for dense teacher training;
- using qrels for the adapter loss.

## Key Lesson From Previous Runs

M370 showed admission is almost solved: dense reranking selected candidates can
match dense quality at low touch budgets.

M371 showed dense-top100 BCE is the wrong ranking objective.

M372 showed dense-score regression is the right clean baseline:

- 10% touched docs;
- NDCG@10 `0.7674`;
- dense baseline `0.7759`;
- no BM25;
- no qrel training.

M373 and M374 were useful negatives:

- simple pairwise dense-order loss loses calibration;
- local z-score/rank features alone do not beat M372.

M375 showed the zero-shot issue is concentrated:

- LODO macro NDCG@10 drops to `0.7379`;
- most datasets remain close;
- `trec-covid` collapses to `0.4502`;
- `trec-covid` dense-rerank upper bound remains `0.8626`.

M376 showed target-distribution calibration is possible:

- source-only `trec-covid` eval split is around `0.80` NDCG@10;
- six disjoint target dense-teacher queries lift it to `0.85-0.86`;
- this uses no qrels, but still should not be treated as a dataset-specific
  product result.

## M377 Hypothesis

The scoring failure is not a lack of candidate evidence. It is a failure to
choose the right scoring/calibration regime from the observed query/corpus
shape.

A global adapter can learn that regime without seeing dataset id.

## Candidate Design

Train a query/corpus-conditioned router over fixed dense-only posting scorers.

Fixed scorers:

- M372 dense-score regression scorer;
- M370-style logistic admission score;
- raw-coordinate sparse dot score;
- pca-coordinate sparse dot score;
- M374 query-local score, retained as a weak alternative;
- optional dense-score-regression variants with different seeds as an ensemble
  member, but not selected per dataset.

Router inputs:

- corpus size and log corpus size;
- active query coordinate mass entropy;
- top coordinate mass ratio;
- posting df mean, max, variance, and entropy for active query dims;
- candidate pool size before budget;
- top score gap and score entropy from each fixed scorer;
- raw-vs-pca candidate overlap;
- scorer agreement@10 and agreement@100;
- query-local score histogram percentiles;
- touch budget and candidate saturation flags.

Router outputs:

- either choose one scorer per query;
- or produce a convex blend over fixed scorer scores;
- optionally output a risk flag that falls back to dense-score rerank on the
  selected 10% candidates when confidence is low.

Training target:

- dense teacher ranking on source train queries;
- optimize dense-overlap@10 or dense-score NDCG proxy, not qrels;
- qrels remain evaluation-only.

## Evaluation Protocol

Primary gate: leave-one-dataset-out.

For each heldout dataset:

1. Train fixed scorers and router on all other datasets.
2. Evaluate on heldout dataset.
3. The router sees no dataset id.
4. The router sees only heldout query/corpus/candidate features available at
   retrieval time.
5. No heldout qrels are used for training, model selection, or hyperparameter
   selection.

Secondary gate: target-query online adaptation simulation.

For each dataset:

1. Split target queries into calibration and eval queries.
2. Use calibration queries only through dense teacher, not qrels.
3. Apply one globally fixed adaptation procedure.
4. Report macro over all datasets and both split seeds.

This is acceptable only if presented as online/corpus calibration, not as
dataset-specific benchmark optimization.

## Stop Rule

Promote M377 only if it improves M375 LODO without overfitting one dataset:

- macro NDCG@10 improves over M375 `0.7379`;
- no major dataset loses more than `0.02` NDCG@10 versus M375;
- `trec-covid` improves materially without using its dataset id;
- dense-rerank upper bound remains visible so admission and ranking are not
  confused.

If the router does not help, stop scalar-feature routing and move to richer
posting-edge encoders:

- small set encoder over matched posting edges;
- per-coordinate impact sketches;
- interaction between query coordinate order and document posting rank.

## Immediate Experiment

M377A should be a small local canary:

- heldout datasets: `trec-covid`, `scifact`, `scidocs`, `fiqa`;
- fixed scorers: M372, raw sparse dot, pca sparse dot, M374;
- router: logistic or shallow MLP over query/corpus/candidate statistics;
- target: choose the scorer with best dense-teacher NDCG proxy on source train
  queries;
- evaluation: qrels on heldout queries only.

If M377A improves `trec-covid` without damaging the other three, expand to all
15 datasets.
