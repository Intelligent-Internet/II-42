# M793 Route Evidence And Next Stage

Decision: M793 stops repeated scorer/delta micro-tuning and promotes a new proposal-generation route: context-bundle proposals with native feedback guards.

## Route Evidence Matrix

| Run | Family | Status | Gate | Clean | Applied | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `M617` | global scorer/reranker | `stop` | 0 |  |  | +0.002134 | -0.011490 | +0.015296 | +0.008280 | +0.000000 |  |
| `M629-B` | native-boundary scorer | `stop` | 1 |  | 30.0 | +0.000078 | +0.000000 | +0.001923 | +0.000000 | +0.000000 |  |
| `M630 anchor0.10` | dense-calibrated scorer | `stop` | 0 |  |  | +0.001235 | -0.000922 | +0.000000 | +0.004724 | +0.000000 | +0.000000 |
| `M752 O100` | native-context proposal policy | `keep_signal` | 1 | 1 | 17.0 | +0.000176 | +0.000059 | +0.000000 | +0.000000 | +0.000171 | +0.000000 |
| `M752 O128p99` | native-context proposal policy | `keep_signal` | 1 | 1 | 14.0 | +0.000070 | +0.000471 | +0.000000 | +0.001845 | +0.000023 | +0.000000 |
| `M758 top16` | deterministic native-context policy | `keep_diagnostic` | 1 |  | 224.0 | +0.000246 | +0.000332 | +0.000000 | +0.000000 | +0.000032 | +0.000000 |
| `M768` | multiseed rank-trace guard | `stop_thresholds` |  |  |  |  |  |  |  |  |  |
| `M777` | damage-bounded policy | `stop_thresholds` | 1 | 1 | 1.7 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `M779 strict oracle` | stable bundle oracle | `keep_oracle` | 1 | 1 | 51.7 | +0.000596 | +0.001129 | +0.000000 | +0.000201 | +0.000135 | +0.000000 |
| `M780` | stable bundle selector | `stop_selector` | 1 | 1 | 5.7 | +0.000007 | +0.000033 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `M787` | generated-delta surface | `stop` | 0 | 0 | 18.0 | -0.000045 | +0.000072 | +0.000000 | +0.000000 | +0.000026 | +0.000000 |
| `M788 logistic clean` | generated-delta frontier | `stop_boundary` | 1 | 1 | 3.7 | +0.000019 | +0.000080 | +0.000000 | +0.000000 | +0.000026 | +0.000000 |
| `M788 hgb clean` | generated-delta frontier | `stop_boundary` | 1 | 1 | 0.0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `M788 hgb recall-any` | generated-delta frontier | `reject_unsafe` | 0 | 0 | 29.0 | -0.000303 | +0.000027 | +0.000019 | -0.000037 | +0.000035 | -0.000861 |
| `M789` | per-query generated-delta safety | `stop_boundary` |  |  |  |  |  |  |  |  |  |
| `M790 support_masked` | vector-delta target oracle | `ranking_teacher_only` |  | 0 | 65.0 | +0.012280 | +0.010694 | +0.053783 | +0.002851 | +0.022229 | -0.036769 |
| `M790 full_vector` | vector-delta target oracle | `ranking_teacher_only` |  | 0 | 65.0 | +0.012280 | +0.010694 | +0.053783 | +0.002851 | +0.022229 | -0.036769 |
| `M791 protected128` | protected vector projection | `ranking_teacher_only` |  | 0 | 65.0 | +0.002209 | +0.000000 | +0.042625 | +0.000000 | +0.016023 | -0.002000 |
| `M792 preserved smoke` | small residual compiler | `stop_scale` | 0 | 1 |  | +0.000010 | +0.000000 | +0.000000 | +0.000000 | -0.000002 | -0.000714 |

## Family Summary

| Family | Entries | Clean rows | Best dMAP | Best dRecall | Statuses |
| --- | ---: | ---: | ---: | ---: | --- |
| damage-bounded policy | 1 | 1 | +0.000000 | +0.000000 | `stop_thresholds` |
| dense-calibrated scorer | 1 | 0 | +0.001235 | +0.000000 | `stop` |
| deterministic native-context policy | 1 | 1 | +0.000246 | +0.000000 | `keep_diagnostic` |
| generated-delta frontier | 3 | 2 | +0.000019 | +0.000019 | `reject_unsafe`, `stop_boundary` |
| generated-delta surface | 1 | 0 | -0.000045 | +0.000000 | `stop` |
| global scorer/reranker | 1 | 0 | +0.002134 | +0.015296 | `stop` |
| multiseed rank-trace guard | 1 | 0 | +0.000000 | +0.000000 | `stop_thresholds` |
| native-boundary scorer | 1 | 1 | +0.000078 | +0.001923 | `stop` |
| native-context proposal policy | 2 | 2 | +0.000176 | +0.000000 | `keep_signal` |
| per-query generated-delta safety | 1 | 0 | +0.000000 | +0.000000 | `stop_boundary` |
| protected vector projection | 1 | 0 | +0.002209 | +0.042625 | `ranking_teacher_only` |
| small residual compiler | 1 | 0 | +0.000010 | +0.000000 | `stop_scale` |
| stable bundle oracle | 1 | 1 | +0.000596 | +0.000000 | `keep_oracle` |
| stable bundle selector | 1 | 1 | +0.000007 | +0.000000 | `stop_selector` |
| vector-delta target oracle | 2 | 0 | +0.012280 | +0.053783 | `ranking_teacher_only` |

## Recommendations

| Action | Route | Reason |
| --- | --- | --- |
| `stop` | global scorer / dense-calibrated scorer / small residual compiler | M617/M629/M630/M792 repeatedly show safe but tiny gains or overlap-spending movement. |
| `keep` | native-context interface and rank-trace/damage witnesses | M752/M758 prove qrels-free or near-deployable context can move MAP/NDCG safely, even if threshold tuning saturates. |
| `keep_as_teacher` | M779 stable bundle oracle and M790/M791 vector targets | They expose useful utility targets, but selectors/compilers cannot deploy them safely yet. |
| `next` | M794 context-bundle proposal generator | Change proposal generation, not thresholding: generate fewer, higher-quality query-local bundle proposals, then gate them with native context, rank-trace, and damage witnesses. |

## M794 Proposed Gate

The next experiment should not train another global scorer or replay another threshold sweep.  It should change the proposal surface:

1. Generate query-local bundle proposals from stable bundle atoms and interaction-witness features.
2. Score each proposal with native-context, rank-trace, and damage witness features before accepting it.
3. Evaluate first on the existing three-surface smoke, then only expand to shared15 if all surfaces have non-negative MAP/NDCG/MRR/CUB/O@100 and the applied count is materially above M768/M777 diagnostic scale.

Stop if M794 cannot beat M768/M777 by at least 5x while staying clean, or if gains only appear through qrels-derived oracle selection.
