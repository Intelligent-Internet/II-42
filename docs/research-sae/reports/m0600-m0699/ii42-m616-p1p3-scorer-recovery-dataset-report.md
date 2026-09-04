# II-42 M616 P1.3 Scorer-Recovery Dataset Report

Date: 2026-07-06

## Objective

M616 is the controlled transition out of the M606/M607 dense-only recovery
loop.  It does not train or promote a scorer.  It freezes the P1.3-a010
candidate surface and builds a native scorer-recovery dataset from M604/M615
evidence.

The goal is to prepare the next second-stage experiment without repeating the
old M605 mistake.  M605 remains a benchmark only; M617 must be a new scorer
line whose training/evaluation is explicitly scoped to candidate-present
under-ranked examples.

## Why This Is Allowed

The active rule says to restart a scorer line only if the P1 candidate pool is
strong but a scorer gap remains measurable.  M615 satisfies that condition:

- P1.3-a010 and aligned dense have nearly identical positive top100 hit rates.
- Remaining under-ranked positives are overwhelmingly not aligned-dense top100.
- Dense-only first-stage losses cannot teach the dominant remaining examples.

Therefore the next useful work is not another M612-M614 dense-only loss-family
variant.  It is a new second-stage scorer dataset that focuses on what the
reranker can affect.

## Implementation

Script:

`scripts/build_m616_scorer_recovery_dataset.py`

Inputs:

- Gap root: `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Aligned dense root: `runs/m608_p1p3_aligned_dense_rankings_shared15_v1`

Outputs:

- JSONL:
  `runs/m616_p1p3_scorer_recovery_dataset_v1/m616_p1p3_scorer_recovery_dataset.jsonl`
- Summary JSON:
  `runs/m616_p1p3_scorer_recovery_dataset_v1/m616_p1p3_scorer_recovery_dataset_summary.json`
- Summary Markdown:
  `runs/m616_p1p3_scorer_recovery_dataset_v1/m616_p1p3_scorer_recovery_dataset_summary.md`

The builder emits only examples a scorer can affect:

- candidate-present positives,
- under-ranked positives,
- current top100 non-positive candidates.

Candidate misses are counted in the summary but excluded from the JSONL because
a reranker cannot recover documents outside the candidate pool.

## Macro Result

| Metric | Value |
| --- | ---: |
| Rows scanned | 1,946,411 |
| Rows selected | 153,608 |
| Positive rows | 39,742 |
| Top100 positives | 13,618 |
| Under-ranked dense-hit positives | 375 |
| Under-ranked dense-miss positives | 19,033 |
| Candidate-miss positives | 6,716 |
| Top100 negative dense-hit | 111,282 |
| Top100 negative dense-miss | 9,300 |
| Selected rate | 0.078919 |
| Under-ranked dense-miss share | 0.980678 |
| Train queries | 1,064 |
| Eval queries | 278 |

## Interpretation

The dataset reproduces the M615 bottleneck in a trainable form:

- The dominant recovery target is candidate-present but under-ranked and
  dense-miss.
- This cannot be solved by a pure dense-equivalent teacher.
- The top100 negative pool is mostly dense-hit, which explains why old
  same-query separability was weak: many blockers are also dense-plausible.

This means M617 cannot be a simple alpha/grid reranker or a rerun of M605.  It
needs either a stronger query-local ordering signal, a guarded listwise
objective, or features that describe why a dense-miss positive is still
relevant inside the P1/BM25 candidate pool.

## Next Step

Start M617 with this dataset as the only allowed training source:

1. Train a global model only; no dataset id and no per-dataset thresholds.
2. Optimize under-ranked positives against current top100 blockers.
3. Preserve saturated rows and avoid recall loss on guard rows.
4. Evaluate first on the M616 eval split.
5. Only after split-level improvement, replay through the native DB/plugin path.
6. Compare against BM25, dense, M549U, P1.3-a010, and old M605 benchmark rows.

Stop M617 if it only learns dataset-specific behavior, improves MAP while
hurting Recall@100, or loses gains in the native path.

## Verification

Local verification passed:

- `python3 -m py_compile scripts/build_m616_scorer_recovery_dataset.py`
- `pytest -q tests/test_build_m616_scorer_recovery_dataset.py`
- `git diff --check`
