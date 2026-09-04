# M1914 Granite Fixed-Support Calibration Contract

Date: 2026-07-12

Status: **complete; see
`docs/research-sae/reports/m1900-m1999/ii42-m1914-granite-fixed-support-calibration-report.md`**

## Question

M1913 established a useful but asymmetric frontier: Granite 30M Sparse has
higher FiQA Recall and CUB than OpenSearch sparse-v2, and its exact BMP index
is smaller and faster, but NDCG, MAP, and MRR are lower. M1914 asks the first
minimal question before changing model support:

> Can a corpus-independent impact calibration recover teacher ordering while
> preserving every Granite posting and the released 192/50 document/query
> support budgets exactly?

This is an output-head calibration test, not a new sparse representation and
not a FiQA-tuned reranker.

## Frozen Inputs

- Parent checkpoint: `ibm-granite/granite-embedding-30m-sparse`, revision
  `ad82b1fd09541c998c8d45045d601c51fdb8a9b7`.
- Parent encoder and pooling: frozen M1913 implementation with exact official
  Sentence Transformers parity.
- Support: top 192 document dimensions and top 50 query dimensions selected
  before calibration. Calibration may not add, remove, or replace a dimension.
- Training data: M1518's pinned 10,000-row MS MARCO teacher set.
- Selection data: M1518's query-disjoint 1,000-row MS MARCO validation set.
- Each row has one positive, eight fixed negatives, and frozen teacher scores.
- FiQA qrels are evaluation-only and may not select a branch, checkpoint,
  regularizer, power range, or term scale.

The M1518 data comes from
`hanhainebula/bge-multilingual-gemma2-data` revision
`ef165e19513da39a1e23bd09d78a3f1b38967e93`, config `en_msmarco`, with the
existing deterministic seed-1518 split. No BEIR text or labels enter M1914
training.

## Deployable Transform

For every active dimension `t`, M1914 scores a query/document pair as:

```text
q'_t = q_t ^ gamma_q * sqrt(exp(a_t)) * exp(s)
d'_t = d_t ^ gamma_d * sqrt(exp(a_t))
score(q, d) = sum_t q'_t * d'_t
```

where `gamma_q`, `gamma_d`, global score scale `s`, and optional shared term
log-scale `a_t` are learned. Inactive dimensions remain exactly zero. The
transform can therefore be folded into the query/document output impacts and
executed by the unchanged BMP index.

Three predeclared branches isolate capacity:

1. `global_power`: learn `gamma_q`, `gamma_d`, and `s`; keep every `a_t=0`.
2. `diagonal`: keep both powers at one; learn `a_t` and `s`.
3. `combined`: learn powers, term scales, and `s`.

Powers are bounded to `[0.5, 2.0]`; term log-scales are bounded to `[-1, 1]`
and regularized toward zero. These bounds prevent an apparent ranking gain
from silently deleting or exploding postings.

## Training And Selection

- Optimizer: AdamW, fixed seed 1914.
- Objective: candidate-set teacher KL plus positive-margin MSE, with the same
  nine candidate ordering as the frozen rows.
- Term-scale regularization: mean squared log-scale over touched dimensions.
- Train each branch for the same fixed step budget and batch order.
- Evaluate step zero and every fixed checkpoint on the complete query-disjoint
  validation set.

Checkpoint selection is lexicographic and qrels-free:

1. positive-vs-negative pairwise accuracy;
2. teacher top-1 agreement;
3. positive top-1 accuracy;
4. teacher-distribution KL.

A branch is eligible only if it improves validation pairwise or teacher top-1
over the unmodified M1913 impacts without reducing either metric by more than
0.5 percentage points. A lower KL without ranking improvement is not eligible.

The winning branch is selected on MS MARCO validation before any complete
FiQA calibration surface is generated. Branches that lose heldout ranking are
not evaluated on FiQA.

## Native Gate

The selected transform is applied once to the frozen complete M1913 FiQA
surface and compiled through the unchanged exact BMP path.

M1914 passes only if all hold:

1. document/query support identities and nnz are exactly unchanged;
2. BMP strict-boundary and returned-score exactness remain `1.0`;
3. BMP Recall is at least `0.663725` (M1913 minus 0.002 absolute);
4. BMP CUB is no lower than M1913 by more than 0.002;
5. at least two of NDCG, MAP, and MRR improve, and none falls by more than
   0.001 absolute;
6. index bytes and p95 remain no worse than `1.02x` M1913.

Passing M1914 proves that fixed support had recoverable score-shape error. It
does not authorize a support-changing model update until a second untouched
row confirms the gain.

### Locked Transfer Confirmation

Because the selected global-power branch passed FiQA without using FiQA for
selection, the pre-support-change confirmation is fixed to three complete
official rows before they are encoded:

- ArguAna;
- NFCorpus;
- SciFact.

The same selected artifact is applied unchanged. No dataset-specific power,
scale, threshold, active dimension, or checkpoint is allowed. Each parent and
calibrated surface is compiled through exact BMP. Transfer passes only if the
broad3 macro improves at least two of NDCG, MAP, and MRR, macro Recall falls by
no more than 0.002, and no row loses more than 0.005 Recall. A row-level head
metric loss larger than 0.001 is reported as harm and may not be hidden by the
macro. Transfer fails if any dataset loses more than 0.001 in two or more of
NDCG, MAP, and MRR.

## Stop Rules

- Stop fixed-support calibration if all branches improve only KL or train
  metrics but not query-disjoint ranking.
- Do not tune on FiQA or use FiQA to choose among branches.
- Do not add BM25, IDF derived from FiQA, lexical features, a second index, or
  query-time reranking.
- Do not proceed to M1915 support reallocation unless M1914 either passes or
  cleanly demonstrates that heldout errors require dimensions outside the
  frozen support.
- Do not interpret unchanged posting count as unchanged cost; measure exact
  BMP bytes and latency.

## Evidence Basis

- M1905 showed that a mature output basis survives the full regularization
  ramp where the local from-scratch SAE basis does not.
- Unified LSR identifies term weighting as a major effectiveness component and
  provides a successful asymmetric sparse training control.
- MLM-head rescaling shows that sparse retrieval is sensitive to output scale.
- M1913 already passes representation and engine gates, so calibration is the
  smallest remaining degree of freedom.

## Required Artifacts

- frozen M1914 overlap-feature cache and provenance manifest;
- per-branch train and heldout trajectories;
- selected calibrator checkpoint and foldable impact-transform artifact;
- complete FiQA float and exact BMP summaries if a branch is eligible;
- `docs/research-sae/reports/m1900-m1999/ii42-m1914-granite-fixed-support-calibration-report.md`.
