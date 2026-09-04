# M605 / P1 Global Reranker

Status: stopped under the guarded native-matrix acceptance rule.

M605 confirmed the M604 scorer-gap hypothesis on a small seen-regression
surface, but none of the tested global reranker families is promotable. The
accepted stop condition is now active: gains either harm a guard dataset or
collapse the ranking metrics while candidate upper bound remains unchanged.

The strongest positive result remains a diagnostic, not a release candidate:
`P1-a0125+M605` improved the target3 seen-regression macro matrix, but the
guard checker rejected it because `cqadupstack` Recall@100 regressed. The final
M605-B pairwise-linear probe was worse and failed immediately. Therefore no
M605 scorer should be used as a new default, and no full official expansion is
justified for this scorer family.

## Scope

M605 keeps the P1 encoder/posting generator frozen. It trains only a global
scorer over native candidate features exported by M604:

- No dataset id feature.
- No per-dataset threshold.
- No learned gate reuse.
- No change to atom generation.
- Candidate upper bound must remain unchanged; otherwise the evaluation is not
  testing a reranker.

The first implementation is deliberately interpretable: balanced logistic
regression over P1/BM25/fused score and rank features. It is M605-A, not the
final listwise reranker.

## Inputs

Initial training/eval inputs:

- `runs/m604_p1_scorer_gap_audit_v1/webis_touche2020_full/webis-touche2020_m604_p1_scorer_gap.jsonl`
- `runs/m604_p1_scorer_gap_audit_v1/nfcorpus_full/nfcorpus_m604_p1_scorer_gap.jsonl`

The split is query-level with salt `m605-a-webis-nfcorpus-v1`, using 80% train
queries and 20% eval queries.

## Initial Result

Artifacts:

- `runs/m605_p1_global_reranker_v1/m605_a_webis_nfcorpus_logistic.json`
- `runs/m605_p1_global_reranker_v1/m605_a_webis_nfcorpus_logistic.md`
- `runs/m605_p1_global_reranker_v1/m605_a_webis_nfcorpus_logistic_model.json`

Macro eval split:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Candidate UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1-a0125 fixed scorer | 0.301461 | 0.118230 | 0.282587 | 0.513045 | 0.692911 |
| M605-A logistic | 0.333835 | 0.139130 | 0.309284 | 0.538157 | 0.692911 |

Per-dataset eval split:

| Dataset | Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 |
| --- | --- | ---: | ---: | ---: | ---: |
| `nfcorpus` | P1-a0125 fixed scorer | 0.307703 | 0.116565 | 0.265103 | 0.498291 |
| `nfcorpus` | M605-A logistic | 0.337449 | 0.135424 | 0.282126 | 0.526061 |
| `webis-touche2020` | P1-a0125 fixed scorer | 0.245282 | 0.133212 | 0.439942 | 0.645833 |
| `webis-touche2020` | M605-A logistic | 0.301312 | 0.172487 | 0.553703 | 0.647024 |

## Interpretation

The first reranker probe is directionally positive:

- Recall@100 improves from `0.282587` to `0.309284`.
- MAP@100 improves from `0.118230` to `0.139130`.
- NDCG@10 and MRR@20 also improve.
- Candidate upper bound is unchanged, as required for a reranker-only test.

This supports the M604 diagnosis that a non-trivial portion of P1-a0125 loss is
ranking/scoring loss, not candidate-generation loss.

The result is still insufficient for promotion because:

- It only uses two datasets.
- It has not yet included `cqadupstack` or `quora`, two known weak rows.
- It is not yet integrated as a native reranker evaluation path.
- It is a query-split seen-regression probe, not a full official matrix.

## Target-Row Extension

The second M605-A run adds the M604 `cqadupstack` stable q40 postings shard to
the original `webis-touche2020` and `nfcorpus` rows.

Additional input:

- `runs/m604_p1_scorer_gap_audit_v1/cqadupstack_q40_postings/cqadupstack_m604_p1_scorer_gap.jsonl`

Artifacts:

- `runs/m605_p1_global_reranker_v1/m605_a_webis_nfcorpus_cq40_logistic.json`
- `runs/m605_p1_global_reranker_v1/m605_a_webis_nfcorpus_cq40_logistic.md`
- `runs/m605_p1_global_reranker_v1/m605_a_webis_nfcorpus_cq40_logistic_model.json`

Macro eval split:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Candidate UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1-a0125 fixed scorer | 0.292231 | 0.132947 | 0.351368 | 0.513314 | 0.687785 |
| M605-A logistic | 0.323288 | 0.148905 | 0.357097 | 0.511974 | 0.687785 |

Per-dataset eval split:

| Dataset | Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 |
| --- | --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | P1-a0125 fixed scorer | 0.222222 | 0.243226 | 0.666667 | 0.239418 |
| `cqadupstack` | M605-A logistic | 0.310043 | 0.255398 | 0.694444 | 0.313255 |
| `nfcorpus` | P1-a0125 fixed scorer | 0.297396 | 0.097594 | 0.238614 | 0.534498 |
| `nfcorpus` | M605-A logistic | 0.312996 | 0.103547 | 0.233071 | 0.510543 |
| `webis-touche2020` | P1-a0125 fixed scorer | 0.311507 | 0.222335 | 0.658306 | 0.584444 |
| `webis-touche2020` | M605-A logistic | 0.376523 | 0.284584 | 0.700402 | 0.637500 |

Interpretation:

- `cqadupstack` and `webis-touche2020` both improve on Recall@100, MAP@100,
  NDCG@10, and MRR@20.
- Macro Recall@100, MAP@100, and NDCG@10 improve with candidate upper bound
  unchanged.
- `nfcorpus` has a small Recall@100 and MRR@20 guard regression. This blocks
  promotion but does not kill the route; it says M605 needs either stricter
  global objective constraints or a safer rerank/blend policy.
- `quora` is intentionally not included. M604 bounded audits did not reproduce
  the expected under-ranking failure mode, so adding it now would train on an
  unclear signal. A direct current rerun for one previously zero-recall quora
  query contradicted the old matrix artifact, so the quora row should be
  regenerated before it is used for M605 acceptance.

## Saved-Model Replay

`scripts/apply_m605_global_reranker.py` replays a saved M605 model JSON over
M604 native candidate JSONL rows. This separates the training script from model
application and is the first step toward a native reranker evaluation path.

Replay artifact:

- `runs/m605_p1_global_reranker_v1/m605_a_webis_nfcorpus_cq40_saved_replay.json`
- `runs/m605_p1_global_reranker_v1/m605_a_webis_nfcorpus_cq40_saved_replay.md`

All-query replay over the same three native candidate row sources:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Candidate UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1-a0125 fixed scorer | 0.309861 | 0.165747 | 0.356395 | 0.505219 | 0.716440 |
| Saved M605-A logistic | 0.355744 | 0.192641 | 0.363045 | 0.547037 | 0.716440 |

This replay is not a promotion result because it includes rows used during
training. Its purpose is narrower: the saved model can be applied independently
to native-path candidate evidence, and candidate upper bound remains unchanged.

## Matrix Integration Smoke

`scripts/apply_m605_global_reranker.py` can now emit M603-compatible per-dataset
`eval.json` rows under an eval root. This lets
`scripts/compile_m603_native_surface_matrix.py` include `P1-a0125+M605` as a
matrix source without adding a separate compiler.

Smoke artifacts:

- `runs/m605_p1_global_reranker_v1/m605_a_webis_nfcorpus_cq40_saved_replay_v3.json`
- `runs/m605_p1_global_reranker_v1/m605_replay_target3_native_matrix_v3.json`
- `runs/m605_p1_global_reranker_v1/m605_replay_target3_native_matrix_v3.md`

The matrix compiler now records `query_count_consistency` and prints a warning
when source query counts differ. The target3 smoke intentionally warns:

```text
BM25/dense/M549U/P1-a0125 query_count = 13517
P1-a0125+M605 query_count = 412
```

Therefore this matrix is proof of integration shape only, not an
apples-to-apples official comparison. The next valid comparison needs M605
candidate rows for the same query set as each baseline source.

## Same-Query Native Smoke

The native baseline and P1 evaluators now support `--query-ids-file`, allowing
BM25, dense, M549U, P1-a0125, and M605 replay to be compared on the same query
set.

Target3 same-query matrix:

- Query sets: M604 `nfcorpus` full, `webis-touche2020` full, and
  `cqadupstack` q40 postings audit rows.
- Matrix:
  `runs/m605_p1_global_reranker_v1/m605_same_query_target3_native_matrix_v2.json`
- Report:
  `runs/m605_p1_global_reranker_v1/m605_same_query_target3_native_matrix_v2.md`
- All sources have the same `412` queries.

Macro:

| Source | Queries | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Cand UB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| BM25 | 412 | 0.321691 | 0.209712 | 0.444442 | 0.461902 | 0.641067 |
| dense | 412 | 0.358989 | 0.245409 | 0.507843 | 0.510380 | 0.760992 |
| M549U | 412 | 0.223414 | 0.134373 | 0.268087 | 0.365253 | 0.618330 |
| P1-a0125 | 412 | 0.241306 | 0.147620 | 0.293025 | 0.396930 | 0.697904 |
| P1-a0125-fixed-union | 412 | 0.326067 | 0.227166 | 0.509610 | 0.480727 | 0.832017 |
| P1-a0125+M605 | 412 | 0.372627 | 0.262920 | 0.522086 | 0.527586 | 0.832017 |

The valid reranker-only comparison is `P1-a0125-fixed-union` versus
`P1-a0125+M605`, because both rows use the same union candidate pool and keep
candidate upper bound unchanged at `0.832017`. On that comparison, M605-A
improves all four ranking metrics:

- Recall@100: `0.509610` -> `0.522086`
- MAP@100: `0.227166` -> `0.262920`
- NDCG@10: `0.326067` -> `0.372627`
- MRR@20: `0.480727` -> `0.527586`

The `P1-a0125` row is the earlier M603 fused-top1000 baseline and should not be
used as the direct M605 reranker comparator here. Its lower candidate upper
bound shows that it is not evaluating the same candidate pool.

Interpretation:

- This is the strongest native-path evidence so far that M605-A can recover
  scorer loss without changing candidate generation.
- It is not sufficient for M605 acceptance. A guarded matrix check rejects this
  row because `cqadupstack` loses Recall@100 even though its MAP/NDCG/MRR
  improve.
- It is still a seen-regression matrix because all three datasets contributed
  to M605-A training/replay development.
- It is not a promotion result until the same method holds on regenerated
  current `quora` evidence and broader clean/native rows.

Guarded check:

- JSON:
  `runs/m605_p1_global_reranker_v1/m605_same_query_target3_guard_check.json`
- Report:
  `runs/m605_p1_global_reranker_v1/m605_same_query_target3_guard_check.md`
- Status: `rejected_guard_harm`
- Macro deltas versus `P1-a0125-fixed-union`:
  NDCG@10 `+0.046559`, MAP@100 `+0.035754`,
  Recall@100 `+0.012477`, MRR@20 `+0.046859`,
  Candidate UB `+0.000000`.
- Guard rows:
  `webis-touche2020` repairs all protected metrics;
  `cqadupstack` improves MAP/NDCG/MRR but drops Recall@100 by `-0.044097`;
  `quora` is missing from this matrix.

Single-query smoke:

- Query set: `runs/m604_p1_scorer_gap_audit_v1/query_sets/quora_q100149.txt`
- Matrix: `runs/m605_p1_global_reranker_v1/m605_quora_q100149_native_matrix.json`
- Report: `runs/m605_p1_global_reranker_v1/m605_quora_q100149_native_matrix.md`

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Candidate UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.000000 | 0.015625 | 1.000000 | 0.000000 | 1.000000 |
| dense | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 1.000000 |
| M549U | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 1.000000 |
| P1-a0125 | 0.630930 | 0.500000 | 1.000000 | 0.500000 | 1.000000 |
| P1-a0125+M605 | 0.500000 | 0.333333 | 1.000000 | 0.333333 | 1.000000 |

This is intentionally tiny, so it is not a quality claim. It is useful because
it proves the same-query native comparison path works and shows the current
M605-A model is not safe on this quora guard query. Quora remains excluded from
training and acceptance until regenerated full-row evidence is available.

The same guard checker marks this query as `rejected_macro_gate`:

- JSON:
  `runs/m605_p1_global_reranker_v1/m605_quora_q100149_guard_check.json`
- Report:
  `runs/m605_p1_global_reranker_v1/m605_quora_q100149_guard_check.md`
- Deltas versus `P1-a0125`: NDCG@10 `-0.130930`,
  MAP@100 `-0.166667`, Recall@100 `+0.000000`,
  MRR@20 `-0.166667`, Candidate UB `+0.000000`.

## M605-A2 Guarded Probes

M605-A2 first tried to make the saved logistic replay more conservative without
changing the candidate pool or using dataset-specific features. The replay
score supports a global `model_blend_alpha`:

- `0.0` keeps the fixed-union baseline order.
- `1.0` uses the saved M605-A logistic score directly.
- Intermediate values blend query-local normalized model score and baseline
  rank score.

Alpha sweep artifacts:

- JSON:
  `runs/m605_p1_global_reranker_v1/m605_a2_target3_alpha_sweep_summary.json`
- Report:
  `runs/m605_p1_global_reranker_v1/m605_a2_target3_alpha_sweep_summary.md`

Result:

| Alpha | Status | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | Harmed guard |
| ---: | --- | ---: | ---: | ---: | ---: | --- |
| 0.00 | `rejected_macro_gate` | 0.000000 | 0.000000 | 0.000000 | 0.000000 | - |
| 0.10 | `rejected_guard_harm` | 0.046638 | 0.035591 | 0.011174 | 0.046871 | `cqadupstack` |
| 0.20 | `rejected_guard_harm` | 0.046638 | 0.035591 | 0.011174 | 0.046871 | `cqadupstack` |
| 0.30 | `rejected_guard_harm` | 0.046638 | 0.035591 | 0.011174 | 0.046871 | `cqadupstack` |
| 0.40 | `rejected_guard_harm` | 0.046638 | 0.035611 | 0.011215 | 0.046871 | `cqadupstack` |
| 0.50 | `rejected_guard_harm` | 0.046638 | 0.035639 | 0.011254 | 0.046871 | `cqadupstack` |
| 0.75 | `rejected_guard_harm` | 0.046638 | 0.035724 | 0.011649 | 0.046871 | `cqadupstack` |
| 1.00 | `rejected_guard_harm` | 0.046559 | 0.035754 | 0.012477 | 0.046859 | `cqadupstack` |

Conclusion: conservative alpha blending does not solve the guard failure. Any
non-zero model contribution that creates macro gains still harms
`cqadupstack` Recall@100.

The second M605-A2 probe retrained the same global logistic model with a small
grid over regularization `C` and positive sample-weight multiplier. It still
uses the same native M604 rows, no dataset id, no per-dataset threshold, and no
candidate-pool change.

Grid artifacts:

- JSON:
  `runs/m605_p1_global_reranker_v1/m605_a2_logistic_grid_summary.json`
- Report:
  `runs/m605_p1_global_reranker_v1/m605_a2_logistic_grid_summary.md`

Result: all `12` tested global logistic settings are rejected by the same
`cqadupstack` guard harm. Macro deltas stay positive, with approximate ranges:

- NDCG@10: `+0.048037` to `+0.048133`
- MAP@100: `+0.035837` to `+0.035887`
- Recall@100: `+0.012981` to `+0.013004`
- MRR@20: `+0.049038`
- Candidate UB: `+0.000000`

Conclusion: the current linear/logistic M605-A family is useful as a scorer-gap
diagnostic, but it is not sufficient as the promoted reranker. The next
meaningful probe needs either a stronger global model class such as GBDT or a
true listwise objective, and it must pass the same guard checker before any
broader native matrix is trusted.

M605-A2 then tested a shallow global GBDT scorer. The model is still auditable:
`scripts/train_m605_global_reranker.py --model-kind gbdt` exports the tree
ensemble as JSON, and `scripts/apply_m605_global_reranker.py` replays it with a
local tree evaluator rather than pickle.

GBDT grid artifacts:

- JSON:
  `runs/m605_p1_global_reranker_v1/m605_a2_gbdt_grid_summary.json`
- Report:
  `runs/m605_p1_global_reranker_v1/m605_a2_gbdt_grid_summary.md`

Result: all `8` tested shallow global GBDT settings are rejected. Some settings
produce positive macro movement, but every setting is blocked by the same
`cqadupstack` guard harm. Representative range:

- NDCG@10: `+0.009407` to `+0.030332`
- MAP@100: `+0.001619` to `+0.020753`
- Recall@100: `-0.035355` to `+0.010052`
- MRR@20: `+0.000981` to `+0.022581`
- Candidate UB: `+0.000000`

Conclusion: stronger pointwise scoring does not solve the current guard
failure. The remaining meaningful M605 path is a minimal listwise or
Recall@100-aware objective over the same native candidate rows. If that also
fails the guarded matrix, this reranker route should stop rather than continue
pointwise scorer tuning.

## M605-B Pairwise Linear Probe

M605-B implemented the smallest listwise/Recall-aware proxy that still remains
auditable and replayable:

- train only a global linear scorer;
- build same-query positive-vs-hard-negative pairs from native M604 rows;
- optionally focus positives whose fixed-union rank is outside top100;
- serialize the model as JSON, not pickle;
- replay through the same native eval-root and guarded matrix checker.

Code path:

- `scripts/train_m605_global_reranker.py --model-kind pairwise_linear`
- `scripts/apply_m605_global_reranker.py` existing JSON replay path

Target3 setup:

- rows: `nfcorpus_full`, `webis_touche2020_full`,
  `cqadupstack_q40_postings`;
- objective focus: `pairwise_positive_rank_min=101`;
- hard negatives/query: `32`;
- max pairs/query: `256`.

The direct pairwise scorer failed badly on the eval split. It preserved
candidate upper bound, but moved many candidates into the wrong order:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Cand UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| baseline | 0.282946 | 0.144224 | 0.373873 | 0.474390 | 0.733695 |
| M605-B direct | 0.014418 | 0.006023 | 0.015635 | 0.033383 | 0.733695 |

A small-alpha native replay sweep also failed. Even `alpha=0.001` harmed both
present guard datasets:

| alpha | status | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | harmed guard |
| ---: | --- | ---: | ---: | ---: | ---: | --- |
| 0.001 | `rejected_macro_gate` | -0.319995 | -0.225549 | -0.497058 | -0.462620 | `cqadupstack`, `webis-touche2020` |
| 0.0025 | `rejected_macro_gate` | -0.320002 | -0.225551 | -0.497038 | -0.462746 | `cqadupstack`, `webis-touche2020` |
| 0.005 | `rejected_macro_gate` | -0.320207 | -0.225555 | -0.497038 | -0.462819 | `cqadupstack`, `webis-touche2020` |
| 0.01 | `rejected_macro_gate` | -0.320224 | -0.225560 | -0.497038 | -0.462925 | `cqadupstack`, `webis-touche2020` |
| 0.025 | `rejected_macro_gate` | -0.320237 | -0.225564 | -0.497038 | -0.462983 | `cqadupstack`, `webis-touche2020` |
| 0.05 | `rejected_macro_gate` | -0.320305 | -0.225566 | -0.497038 | -0.463003 | `cqadupstack`, `webis-touche2020` |
| 0.10 | `rejected_macro_gate` | -0.320305 | -0.225567 | -0.497038 | -0.463018 | `cqadupstack`, `webis-touche2020` |

Artifacts:

- training JSON:
  `runs/m605_p1_global_reranker_v1/m605_b_pairwise_target3_train.json`
- model JSON:
  `runs/m605_p1_global_reranker_v1/m605_b_pairwise_target3_model.json`
- guard checks:
  `runs/m605_p1_global_reranker_v1/m605_b_pairwise_target3_a*_guard_check.json`

Conclusion: M605-B is a finite negative for this simple pairwise-linear
reranker family. The failure is not candidate coverage because candidate upper
bound is unchanged. The learned score is incompatible with the fixed-union rank
geometry and damages the guard rows immediately.

## Current Recommendation

Stop M605 scorer micro-tuning for now. The attempted global scorer families are
now bounded:

1. pointwise logistic: macro-positive but guard-harmful;
2. conservative alpha/logistic grids: still guard-harmful;
3. shallow GBDT: still guard-harmful;
4. pairwise-linear Recall@100 proxy: macro-negative and guard-harmful.

The next useful work should not be another small scorer variant. It should
return to the native engineering path and/or upstream P1 training objective:
make the candidate/posting representation expose a score surface that is
already compatible with fixed-union ranking, then rerun M604/M605 only if the
audit shows under-ranked positives remain recoverable.

Keep `scripts/check_m605_guarded_matrix.py` as the acceptance gate for any
future reranker: candidate upper bound unchanged, macro Recall@100/MAP@100
improved, NDCG@10/MRR@20 guarded, and no present guard dataset harmed.

## M605.2 Replay On P1.2 Active512

After M607 promoted P1.2 active512 and M604 showed that under-ranked positives
still exceed candidate misses, I reran a bounded M605 probe on the new P1.2
native candidate evidence.

Inputs:

- `runs/m608_p1p2_m604_scorer_gap_shared15_v1/*_m604_p1_scorer_gap.jsonl`
- Frozen first-stage baseline: P1.2-a0125
- Query split: stable hash split, 80/20
- Dataset id remains excluded from features

Code update:

- `scripts/train_m605_global_reranker.py` and
  `scripts/apply_m605_global_reranker.py` now support
  `--preserve-top-k`.
- Default behavior is unchanged (`--preserve-top-k 0`).
- The new option keeps the fixed baseline top-k order and only allows the
  model to rerank the remaining tail.  This directly tests the safer
  admission-only role instead of a free top-rank scorer.

P1.2 shared15 eval-split probes:

| Probe | dCUB | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| free logistic | +0.000000 | -0.004108 | -0.004698 | -0.023301 | -0.007156 | reject |
| free pairwise tail | +0.000000 | -0.760783 | -0.690746 | -0.861086 | -0.840451 | reject |
| top50 logistic alpha 0.10 | +0.000000 | +0.000000 | -0.000206 | -0.011460 | +0.000000 | reject |
| top50 GBDT d1/e16 alpha 0.10 | +0.000000 | +0.000000 | +0.000398 | -0.000916 | +0.000000 | reject |

Artifacts:

- `runs/m605_p1p2_global_reranker_v1/m605p2_logistic_shared15_train.json`
- `runs/m605_p1p2_global_reranker_v1/m605p2_pairwise_tail_shared15_train.json`
- `runs/m605_p1p2_global_reranker_v1/m605p2_logistic_a010_top50_shared15_train.json`
- `runs/m605_p1p2_global_reranker_v1/m605p2_gbdt_d1e16_a010_top50_shared15_train.json`

Interpretation:

P1.2 made the candidate pool substantially stronger, but the current M605
feature family still does not produce a promotable global scorer.  Free
scorers harm all metrics.  Tail-preserving scorers protect NDCG/MRR by
construction, but still lose Recall@100; the small GBDT MAP gain is too small
and comes with a Recall regression.

Decision:

Stop M605.2 for this feature family.  Do not run a larger scorer grid.  The
next useful work is upstream again: make the P1.2 native score surface closer
to the dense-only active512 sparse-dot geometry, especially because native
O@100 (`0.79425`) is still far below the dense-only active512 gate (`0.94500`).

The next candidate direction should be M608/P1.3 score-interface recovery:
audit why offline signed-coordinate sparse dot and native atom scoring diverge,
then repair normalization/impact encoding before another scorer attempt.
