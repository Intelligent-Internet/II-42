# II-42 M610 P1.3 Guarded Admission Report

Date: 2026-07-06

## Objective

M610 tests whether the remaining M604 scorer gap can be reduced without
returning to the failed M605 free-reranker family.

The frozen input is P1.3-a010:

- First-stage atom surface: P1.3 active512 signed-dot query atoms
- Native candidate evidence:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1/`
- Baseline decision context:
  `docs/research-sae/reports/m0600-m0699/ii42-m604-p1p3-native-scorer-gap-audit-report.md`
- Dense-equivalence reference:
  `runs/m608_p1p3_native_shared15_aligned_dense_v1/m603_p1_native_p1p3_signed_dot_query_shared15_aligned_dense_matrix.json`

M610-A does not train an encoder, does not alter the P1.3 atom generator, and
does not use dataset-specific parameters.  It probes conservative top100
admission policies over native M604 candidate rows.

## Method

The new probe script is:

`scripts/probe_m610_guarded_admission.py`

It evaluates global rank-calibration policies using a query-level stable
train/eval split.  The current tested policy family is slot admission:

1. Preserve the native baseline head exactly.
2. Re-rank only the remaining tail by a global RRF-style score over P1 rank,
   BM25 rank, and fused rank.
3. Admit only enough tail documents to fill the lower top100 slots.
4. Keep candidate upper bound unchanged.

This is materially different from M605:

- M605 allowed free scorer movement or broad tail reranking.
- M610-A tests lower-slot admission while preserving the head ranking.
- M610-A is a bounded recovery probe, not a promoted scorer.

## Artifacts

Aggressive slot probe:

- JSON:
  `runs/m610_p1p3_guarded_admission_v1/m610_p1p3_slot_admission_probe.json`
- Report:
  `runs/m610_p1p3_guarded_admission_v1/m610_p1p3_slot_admission_probe.md`

Bottom-slot probe:

- JSON:
  `runs/m610_p1p3_guarded_admission_v1/m610_p1p3_bottom_slot_admission_probe.json`
- Report:
  `runs/m610_p1p3_guarded_admission_v1/m610_p1p3_bottom_slot_admission_probe.md`

## Aggressive Slot Probe

Configuration space:

- preserve top-k: `50,75,90`
- RRF k: `10,60,100`
- global P1/BM25/fused weight triplets

Selected result:

`slot_admission_top50_k10_p2_b1_f0`

| Split | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dCand UB | dUnder-ranked |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| train | +0.000000 | -0.000505 | +0.000475 | +0.000000 | +0.000000 | +0.002698 |
| eval | +0.000000 | -0.000393 | +0.001670 | +0.000000 | +0.000000 | +0.004914 |
| all | +0.000000 | -0.000483 | +0.000716 | +0.000000 | +0.000000 | +0.003095 |

Decision:

Rejected.  It improves query-average Recall@100, but MAP regresses and the
weighted under-ranked-positive rate gets worse.  It also harms `quora` and
`cqadupstack` guard rows.

## Bottom-Slot Probe

Configuration space:

- preserve top-k: `95,98,99`
- RRF k: `10,60,100`
- global P1/BM25/fused weight triplets

Selected result:

`slot_admission_top98_k100_p1_b1_f1`

| Split | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dCand UB | dUnder-ranked |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| train | +0.000000 | +0.000060 | -0.000045 | +0.000000 | +0.000000 | -0.000613 |
| eval | +0.000000 | +0.000040 | +0.000836 | +0.000000 | +0.000000 | -0.000281 |
| all | +0.000000 | +0.000056 | +0.000132 | +0.000000 | +0.000000 | -0.000554 |

Eval dataset deltas for the selected config:

| Dataset | dMAP@100 | dRecall@100 | dUnder-ranked |
| --- | ---: | ---: | ---: |
| msmarco | -0.000645 | -0.001330 | +0.000865 |
| nfcorpus | -0.000099 | -0.001246 | +0.000000 |
| scidocs | +0.000286 | +0.009524 | -0.009709 |
| trec-covid | +0.000350 | +0.000431 | -0.000259 |
| webis-touche2020 | +0.000909 | +0.005682 | -0.004673 |

Other eval rows were unchanged.

Decision:

Weak positive, not promoted.  The selected bottom-slot policy passes the
mechanical eval gate and does not harm the configured guard rows, but the
effect size is tiny and not broad.  It mostly trades a small gain on
`scidocs`, `trec-covid`, and `webis-touche2020` against small losses on
`msmarco` and `nfcorpus`.

## Interpretation

M610-A gives a bounded answer:

1. Free or aggressive reranking remains unsafe.
2. Extremely conservative lower-slot admission can produce a tiny positive
   signal while keeping candidate upper bound unchanged.
3. The signal is too small to call the current bottleneck solved.
4. The next step should not be a larger slow grid over the same implementation.

The practical lesson is that the remaining scorer gap is real but difficult to
recover with simple global rank fusion.  Top100 admission is safer than M605,
but only when it touches the very bottom of the top100.

## Recommendation

Keep `P1.3-a010` as the frozen benchmark candidate.

Do not promote M610-A as a new scorer.  Preserve it as evidence that
head-preserving bottom-slot admission is the only currently safe reranking
shape, but the current effect size is below the threshold for a mainline
component.

Next step:

1. Optimize the M610 evaluator with precomputed per-query arrays before any
   larger search.
2. Run a score-separability diagnostic on under-ranked positives versus
   bottom-top100 negatives.
3. If separability is weak, return to first-stage score calibration rather than
   another reranker.
4. If separability is strong, implement a native replay of the bottom-slot
   admission policy and require a full native matrix before promotion.

## M611 Follow-Up

The follow-up separability diagnostic has now been run:

`docs/research-sae/reports/m0600-m0699/ii42-m611-p1p3-score-separability-report.md`

Result:

`return_to_score_calibration_low_separability`

M611 confirms the conservative interpretation of M610-A.  The bottom-slot
oracle ceiling shows that candidate-present positives exist near the top100
boundary, but existing native features cannot distinguish them safely from
bottom-top100 negatives within the same query.

Key evidence:

| Feature | Global AUC | Query-pair AUC |
| --- | ---: | ---: |
| p1_score | 0.85716 | 0.00702 |
| bm25_score | 0.71001 | 0.41923 |
| bm25_rr | 0.44132 | 0.51699 |

This means a larger M610 grid would mostly search over weak or inverted
signals.  The M610-A result should stay as a bounded diagnostic, not a
promoted scorer.

Updated next step:

Stop M610 expansion for now and move to M612 P1.4 query-local score
calibration.  Revisit guarded admission only after the first-stage score
surface has better same-query dense-order preservation.
