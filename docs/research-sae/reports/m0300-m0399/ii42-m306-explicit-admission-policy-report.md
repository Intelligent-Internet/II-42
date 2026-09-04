# M306 Explicit Admission Policy Report

Date: 2026-06-15

## Purpose

M306 tests whether the M304/M305 atom-feature utility surface can improve by
training explicit final admission win/loss pairs.

This is not a return to the old Stage-A rank-aware loss. The fixed contract is:

- keep the current M190/M304 frozen BM25+SAE candidate surface;
- keep the M304 atom-feature utility abstraction;
- train on qrel-positive documents missed by the base head versus high-score
  non-relevant documents from BM25/SAE/base candidates;
- evaluate against the existing BM25+SAE score-fusion baseline.

Runner:

```text
scripts/research_sae_m306_explicit_admission_policy.py
```

## Training Objective

M306 uses the same serving-safe feature families as M305, but replaces generic
soft-gate/listwise calibration with explicit admission pairs:

```text
positive:
    qrel-positive document with base rank > admission_base_k

negative:
    non-relevant high-base candidate
    OR non-relevant atom-preferred candidate
```

The loss optimizes:

- pairwise admission: positive should outrank selected negatives;
- head false-positive penalty: do not promote high-base non-relevant hits;
- small tail listwise term;
- small base distillation;
- small gate sparsity penalty.

The first run uses hard `preserve_base_top_k=50` to test safe tail admission
against M304 p50. The second run uses `preserve_base_top_k=0` only to check
whether the objective has unrestricted top-rank signal.

## Runs

All runs use the same five-dataset smoke surface as M304/M305:

```text
nfcorpus, scifact, arguana, scidocs, fiqa
```

Remote root:

```text
/home/huoju/leask/runs
```

| Run | Path | Preserve | Key settings |
| --- | --- | ---: | --- |
| M306 p50 | `ii42-m306-explicit-admission-p50-5ds-v1` | 50 | `admission_base_k=50`, `false_base_k=100`, `residual_alpha=0.10` |
| M306 p0 | `ii42-m306-explicit-admission-p0-5ds-v1` | 0 | same objective, no hard preserve |

## Macro Result

Baseline is the existing BM25+SAE score-fusion row on the same surface.

| Run | Recall@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Verdict |
| --- | ---: | ---: | ---: | ---: | --- |
| M304 p50 | +0.001049 | +0.000000 | +0.000000 | +0.000039 | best safe atom-feature admission |
| M304 p0 | +0.001009 | +0.000069 | +0.001119 | +0.000824 | best top-rank atom-feature smoke, local harm |
| M305 v1 | +0.000137 | -0.000608 | -0.000239 | -0.000105 | failed soft gate |
| M305 v2 | -0.000069 | -0.000556 | -0.000117 | -0.000067 | failed conservative soft gate |
| M306 p50 | +0.000370 | +0.000000 | +0.000000 | +0.000018 | safe but weaker than M304 p50 |
| M306 p0 | +0.000374 | -0.000541 | -0.000090 | -0.000002 | unsafe and weaker than M304 p0 |

## Per-Dataset Result

M306 p50:

```text
nfcorpus: +0.001848 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000088 MAP@100
scifact:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000001 MAP@100
arguana:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000001 MAP@100
scidocs:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, +0.000001 MAP@100
fiqa:     +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000000 MAP@100
```

M306 p0:

```text
nfcorpus: +0.001872 recall@100, -0.002941 MRR@20, -0.000489 NDCG@10, -0.000057 MAP@100
scifact:  +0.000000 recall@100, +0.000021 MRR@20, +0.000000 NDCG@10, +0.000022 MAP@100
arguana:  +0.000000 recall@100, -0.000044 MRR@20, -0.000039 NDCG@10, -0.000043 MAP@100
scidocs:  +0.000000 recall@100, +0.000000 MRR@20, +0.000000 NDCG@10, -0.000000 MAP@100
fiqa:     +0.000000 recall@100, +0.000257 MRR@20, +0.000079 NDCG@10, +0.000067 MAP@100
```

## Interpretation

The explicit admission objective did not improve over M304.

M306 p50 is safe but too weak. It recovers only about one third of the M304 p50
Recall@100 gain and does not move top-rank metrics.

M306 p0 confirms that the objective can move scores, but the movement is not
well calibrated. It produces small recall gain while damaging `nfcorpus` MRR
and NDCG. This is the same failure class as M305, only with a more explicit
pair construction.

The key negative evidence is:

- explicit qrel-positive versus false-positive pairs are not enough on this
  candidate surface;
- the current objective still cannot distinguish useful tail admission from
  harmful head disturbance;
- M304's simpler atom-feature residual remains stronger.

## Decision

Close M306 as a negative result.

Do not expand M306 to 10 datasets or official full-corpus gates.

The current active evidence remains:

- M304 p50: best safe atom-feature admission smoke.
- M304 p0: best top-rank atom-feature signal, but unsafe.
- M300 p50: best broader 10-dataset safe aggregate proxy.

The next direction should not be another small pairwise/gate variation. The
remaining blocker is not just loss wording. It is the mismatch between
candidate-level residual training and the desired unified posting traversal.

Useful next options:

1. Build a true posting-level training surface where BM25-token and SAE-atom
   impacts are learned before candidate residual scoring.
2. Split the product policy into two deterministic layers:
   safe atom-feature tail admission first, reranker/top-rank repair later.
3. Compile M304 p50-like atom utility into bucketed posting impacts and test
   whether the engine-level candidate generation improves on full BEIR, without
   trying to own final top-rank order.

