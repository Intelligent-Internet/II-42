# M641 / P1.3 Nonlinear Native-Boundary Ranker Report

Status: `breakthrough_candidate_not_default`.

## Goal

M641 tests the most direct remaining second-stage hypothesis after M629 and
M640:

- M629 showed that a global native-boundary scorer can improve Recall@100, but
  linear/cross/listwise variants only reached `+0.001923` Recall@100 and
  `+0.000078` MAP@100.
- M640 showed that first-stage query compilation can push boundary positives
  across top100, but local boundary MAP/NDCG still regresses.

M641 therefore keeps the first-stage P1 candidate pool frozen and tests whether
the native-boundary scorer was capacity-limited.  It uses the same M629 row
contract and native replay evaluator, but replaces the linear scorer with a
global `HistGradientBoostingClassifier`.

## Audit Rules

M641 intentionally stays inside the existing native scorer proof surface:

- no dataset-specific thresholds
- no query id, doc id, dataset id, dense rank, or dense hit features
- no learned gate
- no BM25 alpha search
- no candidate generator change
- replay through the same M629 native-boundary metric contract

The new script also caches eval-row tree scores before grid replay.  The first
uncached attempt reached replay and then spent excessive time repeatedly
calling `predict_proba` per grid setting.  Cached replay preserves the metric
contract and makes nonlinear scorer iteration practical.

## Artifacts

| Artifact | Path |
| --- | --- |
| Script | `scripts/train_m641_nonlinear_native_boundary_ranker.py` |
| Tests | `tests/test_train_m641_nonlinear_native_boundary_ranker.py` |
| Canary run | `runs/m641_p1p3_nonlinear_native_boundary_ranker_canary_v1/` |
| Validation run | `runs/m641_p1p3_nonlinear_native_boundary_ranker_validation_v1/` |

## Runs

| Run | Train rows | Trees | Grid | Best alpha | Preserve | Window | Status |
| --- | ---: | ---: | --- | ---: | ---: | ---: | --- |
| Canary | 80,000 | 80 | alpha 0.10/0.20/0.35, preserve 95/99, window 200/300 | 0.10 | 95 | 300 | `nonlinear_breakthrough_candidate` |
| Validation | 160,000 | 100 | alpha 0.05/0.10/0.15/0.20, preserve 95/98/99, window 300 | 0.10 | 95 | 300 | `nonlinear_breakthrough_candidate` |

## Native Replay Results

Baseline for both runs is the same P1.3-a010 proxy eval split from M629.  M629-B
reference delta is `Recall@100 +0.001923`, `MAP@100 +0.000078`.

| Run | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | Promoted | Displaced | Net top100 positives |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M629-B reference | +0.000000 | +0.000078 | +0.001923 | +0.000000 | 30 | 23 | +7 |
| M641 canary | +0.000000 | +0.000184 | +0.004131 | +0.000000 | 53 | 44 | +9 |
| M641 validation | +0.000000 | +0.000131 | +0.004468 | +0.000000 | 57 | 49 | +8 |

Validation macro:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Candidate UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `P1.3-a010 proxy` | 0.745306 | 0.688652 | 0.889652 | 0.838500 | 0.993377 |
| `P1.3-a010+M641` | 0.745306 | 0.688783 | 0.894120 | 0.838500 | 0.993377 |

## Per-Dataset Validation Recall Delta

| Dataset | dRecall@100 |
| --- | ---: |
| arguana | +0.000000 |
| climate-fever | +0.000000 |
| cqadupstack | +0.000972 |
| dbpedia-entity | +0.000424 |
| fever | +0.000000 |
| fiqa | +0.052632 |
| hotpotqa | +0.000000 |
| msmarco | -0.002041 |
| nfcorpus | +0.005316 |
| nq | +0.000000 |
| quora | +0.000000 |
| scidocs | +0.006897 |
| scifact | +0.000000 |
| trec-covid | +0.000873 |
| webis-touche2020 | +0.000000 |

## Interpretation

M641 is the first second-stage scorer after M629 that clearly improves the
native-boundary result without changing the candidate pool.  The repeated best
configuration is also stable: alpha `0.10`, preserve top `95`, rerank window
`300`.

This means M629's weak result was not solely caused by bad labels or impossible
feature geometry.  Model capacity matters: nonlinear interactions recover
roughly twice the M629-B Recall@100 delta while keeping MAP positive and
NDCG/MRR unchanged.

However, M641 is not yet a new default:

- validation Recall@100 gain is `+0.004468`, close to but still below the
  `+0.005` promotion threshold
- net top100 positives are still small (`+8` on the eval split)
- gains concentrate in FiQA, NFCorpus, Scidocs, and small positive deltas on a
  few other rows; MSMARCO has a small negative delta
- this is still the M629 eval split, not a full official native DB evaluation

## Decision

Keep M641 as a breakthrough candidate and the current best second-stage scorer
direction.  Do not promote it as the frozen P1 default yet.

The next useful step is not another linear scorer or alpha sweep.  The next step
should scale M641 carefully:

1. Run a full-grid M641 with cached replay and fixed best neighborhood:
   alpha around `0.08-0.15`, preserve top `95-98`, window `300`.
2. Add a dominance gate: reject if macro gain is driven by one dataset while
   more than one major dataset regresses.
3. Export per-query promotion/displacement rows for the M641 validation model
   and compare recovered positives against M629-B.
4. If full-grid M641 reaches or exceeds `+0.005` Recall@100 with non-negative
   MAP and no NDCG/MRR regression, move to native DB/plugin replay.
5. If it plateaus below `+0.005`, preserve M641 as evidence that nonlinear
   scorer capacity helps, but return to first-stage generated-posting objective
   with M641 examples as supervised boundary diagnostics.

## Verification

Completed before writing this report:

- `python3 -m py_compile scripts/train_m641_nonlinear_native_boundary_ranker.py`
- `pytest -q tests/test_train_m641_nonlinear_native_boundary_ranker.py`
