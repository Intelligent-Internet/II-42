# II-42 M508 Direct Posting Head Report

## Summary

M508 tests the next root-cause hypothesis after M507: the posthoc admission
gate was not the right place to recover dense behavior.  Instead, the
posting/support head itself must be trained against the dense teacher and the
high-fanout route winners.

This is still a materialized-dense stage-A gate.  It does not train raw PPLX or
LoRA adapters yet.  Its purpose is to validate whether a better target/loss
shape can make the posting surface dense-faithful before spending GPU time on a
raw text-to-posting encoder.

Result: positive.  M508 materially improves the PPLX-root posting route and
justifies a LoRA/direct-posting stage-B.

## Method

Training surface:

- frozen row-int8 PPLX dense teacher;
- `rotation_residual` support/posting head;
- structural dense-tail scorer unchanged;
- no BM25 features and no dataset-name policy.

Target shaping:

- dense-teacher top positives;
- high-fanout p768 route/dense winners;
- dense-score regression over selected pools;
- listwise dense-teacher pressure;
- anchor and load-balance regularization.

This changes the model-side support representation instead of adding another
posthoc gate over a weak support set.

## Broad10 Sampled Matrix

Run: `outputs/m508/broad10_seed5080_sampled/m508_broad10_seed5080_sampled.json`

Tasks: ArguAna, CQADupstackGamingRetrieval, CQADupstackUnixRetrieval,
ClimateFEVERHardNegatives, FEVERHardNegatives, FiQA2018,
HotpotQAHardNegatives, SCIDOCS, TRECCOVID, Touche2020Retrieval.v3.

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_materialized_dense` | 0.58749 | 0.76544 | 0.66966 | 0.44995 | 0.99672 | 1.00000 | 1.00000 |
| `teacher_row_int8_dense` | 0.58652 | 0.76544 | 0.66759 | 0.44944 | 1.00000 | 1.00000 | 1.00000 |
| `m508_rotation_residual_fixed_p768` | 0.57165 | 0.74288 | 0.65690 | 0.42624 | 0.81420 | 0.91560 | 0.64505 |
| `m508_rotation_residual_hand_count` | 0.56711 | 0.72721 | 0.65319 | 0.42100 | 0.78988 | 0.87016 | 0.47449 |
| `m508_rotation_residual_fixed_p512` | 0.56614 | 0.72437 | 0.65491 | 0.42040 | 0.77612 | 0.86072 | 0.55119 |
| `m508_rotation_residual_fixed_p256` | 0.52541 | 0.66736 | 0.63227 | 0.38337 | 0.68468 | 0.73908 | 0.39437 |
| `m508_structural_fixed_p256` | 0.48755 | 0.60477 | 0.58261 | 0.34866 | 0.56504 | 0.59428 | 0.41102 |

Key deltas:

- fixed p768 is 0.01487 NDCG@10 below row-int8 dense;
- hand-count is 0.01941 below row-int8 dense at 47.45% touch;
- fixed p512 nearly matches hand-count quality but touches more postings;
- fixed p256 remains too low for the default quality target.

Compared with M507:

- M507 full-feature diagnostic Broad10: 0.55028;
- M507 hand-count Broad10: 0.54819;
- M508 hand-count Broad10: 0.56711;
- M508 fixed p768 Broad10: 0.57165.

M508 therefore improves the route by changing the learned posting surface, not
by adding another threshold or posthoc admission feature.

## Broad4 Gate Matrix

Run: `outputs/m508/broad4_seed5080_gate/m508_broad4_seed5080_gate.json`

Tasks: ArguAna, FiQA2018, SCIDOCS, TRECCOVID.

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_materialized_dense` | 0.50047 | 0.62847 | 0.56336 | 0.27142 | 0.99670 | 1.00000 | 1.00000 |
| `teacher_row_int8_dense` | 0.49808 | 0.62848 | 0.55836 | 0.27041 | 1.00000 | 1.00000 | 1.00000 |
| `m508_rotation_residual_fixed_p768` | 0.49194 | 0.62203 | 0.55349 | 0.25698 | 0.84500 | 0.93460 | 0.76699 |
| `m508_rotation_residual_hand_count` | 0.48927 | 0.61203 | 0.55584 | 0.25378 | 0.82210 | 0.88940 | 0.60210 |
| `m508_rotation_residual_fixed_p512` | 0.48645 | 0.61823 | 0.55349 | 0.25428 | 0.81190 | 0.89120 | 0.69158 |
| `m508_rotation_residual_fixed_p256` | 0.45837 | 0.60208 | 0.55251 | 0.24421 | 0.73900 | 0.79620 | 0.54833 |
| `m508_structural_fixed_p256` | 0.41984 | 0.53903 | 0.50370 | 0.20620 | 0.61550 | 0.65510 | 0.55504 |

The Broad4 gate already showed the same pattern: direct target shaping makes
the route much closer to dense.  Broad10 confirmed that this was not a narrow
four-task artifact.

## Interpretation

The current route is not dead.  The weak subline was posthoc admission over a
weak support representation.  M508 shows that the correct pressure is
model-side: the head must learn to emit the support required by the dense
teacher and by high-fanout route winners.

This directly supports the current research thesis:

- dense semantics can be represented by a compact posting route;
- the prior loss/target shape was under-specified;
- a raw PPLX-root encoder should first learn direct posting targets under
  supervised dense preservation;
- ranking/RL should remain late-stage and constrained.

M508 does not prove that a raw text-to-posting encoder is solved.  It proves
that the target shape is now strong enough to justify training one.

## Decision

Promote M508 stage-A.

Stop:

- threshold-only calibration loops;
- linear support-only posthoc admission;
- RL before supervised posting preservation.

Continue:

- direct posting-head training;
- LoRA/adapters on top of PPLX for raw text-to-posting;
- dense-overlap and fanout gates as hard constraints;
- no-BM25 first-stage encoder training.

## Next Stage

M509 should train a raw PPLX-root direct posting encoder:

1. Keep PPLX as the root model.
2. Add LoRA/adapters and a posting/support head.
3. Distill from row-int8 dense plus M508 direct posting targets.
4. Gate first on dense overlap, candidate recall, and fanout.
5. Only after supervised preservation passes, add ranking-aware or BM25-aware
   objectives as a separate second stage.

Spark environment note: the current spark-1 environment has `torch` and
`transformers`, but `peft` and `sentence_transformers` are missing.  If M509
uses LoRA, install only the missing packages without replacing the torch stack.
