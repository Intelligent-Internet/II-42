# M1650 Dense-Teacher Sparse Foundation Contract

## Route Decision

M1650 leaves the closed pooled-dense output-head family. It starts from a
retrieval-trained vocabulary-sparse encoder and transfers dense/P1 capability
through training supervision:

```text
training only:
    frozen BGE/P1 dense teacher -----+
                                      -> normalized candidate distribution
    frozen sparse-root teacher ------+
                                                |
                                                v
deployment:
    text -> one sparse encoder -> one 30,522-term posting map -> one index
```

This is not ANN plus BM25, not an external reranker, and not two production
indexes. Dense and sparse teachers disappear after training.

## Evidence

- M1540-M1545 closed learned projections over the same pooled dense source:
  representation quality required broad accumulation and failed exact posting
  access. M1650 therefore does not add another pooled output head.
- M1640 shows that a mature normalized-vocabulary sparse encoder has useful
  zero-shot quality, controlled DF, and exact top100 behavior on this project's
  data. Its engine result is handled separately.
- OpenSearch sparse distillation (`2411.04403`) reports that a score-normalized
  dense+sparse ensemble teacher and IDF-aware cost can improve a single sparse
  student.
- Vocabulary Transfer (`2607.00004`) shows that complete vocabulary/backbone
  compatibility is stronger than head-only adaptation and that activation
  calibration prevents dead or dense collapse.
- Dense-to-sparse probabilistic expansion control (`2402.17535`) identifies
  co-activation and semantic deviation as the failure modes of naive dense
  transfer.

## S0: Engine Separation

M1640B/C determines whether the current fixed-block simulator is adequate.
Its failure cannot be repaired by model loss. M1650 may run a bounded training
signal canary after the representation control passes shared3, but no model may
be promoted until a real BMP/BP-ordered or native plugin engine passes its own
latency, memory, and exactness gate.

## S1: Teacher Observability Audit

Use the pinned Apache-2.0 OpenSearch sparse checkpoint as student initialization
and sparse teacher. Use pinned `BAAI/bge-base-en-v1.5` as dense teacher.

Build one deterministic surface from the existing MS MARCO training artifact:

- 4,096 candidate sets for training;
- 512 disjoint candidate sets for validation;
- one positive and the first three stored hard negatives per query;
- no BEIR query, qrel, score, or dataset-specific choice.

For each candidate set, report:

- dense-teacher and sparse-teacher positive top1/pair accuracy;
- per-query z-score-normalized teacher agreement and complementarity;
- equal-weight ensemble positive top1/pair accuracy;
- score variance, ties, and non-finite rows;
- initial student query/document nonzeros and batch DF/FLOPS.

Authorize training only if the ensemble is finite and either improves heldout
positive top1 over the sparse teacher or recovers at least 5% of sparse-teacher
errors without increasing dense-teacher errors. If the two teachers provide no
complementary target, stop before gradient updates.

## S2: 4K Frozen-Contract Canary

Continue the complete sparse checkpoint at low learning rate; do not replace
its vocabulary or randomly initialize a new head. The candidate score remains
the exact sparse dot product used at inference.

The primary objective is candidate-set KL from the student's score distribution
to the fixed equal-weight normalized teacher ensemble. Cost and forgetting are
constraints rather than freely tuned reward terms:

- initial sparse-teacher distribution remains an anchor;
- mean query/document nonzeros may not exceed 1.10 times initialization;
- batch maximum DF and FLOPS may not exceed 1.10 times initialization;
- special-token output dimensions remain disabled.

Use one 500-step run with fixed seed and evaluate steps 0, 100, 250, and 500.
Do not select a checkpoint from training loss alone.

### Canary Gate

A trained checkpoint is valid only if, on the fixed 512-row validation set:

- ensemble KL improves at least 5% relative to step 0;
- positive pair accuracy does not regress;
- sparse-anchor candidate-score Spearman remains at least 0.95;
- all nonzero/DF/FLOPS constraints pass;
- the selected checkpoint is trained, not epoch/step zero.

If no checkpoint passes, stop this objective. Do not adjust teacher weights,
temperature, loss weights, candidate count, or training depth.

## S3: Shared3 Native Gate

Encode locked shared NFCorpus, SciFact, and FiQA with the selected checkpoint
and run exact sparse retrieval. Compare against the unmodified OpenSearch root
and frozen BGE dense.

Authorize scale only if:

- macro NDCG@10, MAP@100, Recall@100, and MRR@20 do not regress from the sparse
  root;
- dense overlap@100 improves by at least 0.01 macro;
- no row loses more than 0.01 Recall@100 or NDCG@10;
- posting nonzeros, max DF, and exact-engine parity remain within the canary
  constraints.

One macro gain with row-level harm is not a pass.

## S4: Scale Only After Causal Pass

If S3 passes, increase training data once to at least 50K candidate sets. Add
the paper-supported IDF-aware/DF-FLOPS objective only after an audit proves its
gradient targets the measured high-DF dimensions. Keep the equal-weight teacher
fixed. Then run shared15 and official heldout native evaluation.

The scaled gate requires gains over the original sparse root and no material
regression relative to dense on NDCG@10, Recall@100, or MRR@20. Product
promotion additionally requires the real native engine gate from S0.

## Stop Rules

Stop when:

- teacher complementarity is absent;
- only step zero passes constraints;
- KL improves while pair accuracy, row quality, or sparse-anchor agreement
  falls;
- cost improvement depends on post-hoc TopK/threshold pruning;
- gains require changing teacher weights or dataset-specific tuning;
- native-engine gains disappear outside the offline scorer.

No ordinary selector, gate, alpha, block-size, temperature, or loss-weight grid
is authorized by this contract.
