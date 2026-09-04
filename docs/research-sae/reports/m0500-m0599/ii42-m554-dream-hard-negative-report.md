# M554 DREAM Hard-Negative Candidate-Set Report

M554 tests the remaining useful DREAM-derived idea after M553:
candidate-set competition may depend on which candidates compete.  The
experiment keeps M551's BM25-free first-stage objective and output-layer
encoder, then changes only the qrels-free candidate-set construction and
rank-interface gate.

## Implementation

Code path:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m554_dream_hard_negative_posting_spark.sh
```

New arguments:

- `--negative-mode random|dense_tail|mixed`
- `--hard-negative-pool-k`
- `--hard-negative-ratio`

Default behavior remains M551-compatible:

```text
negative-mode=random
```

The M554 runner uses:

- `TEACHER_TARGET_MODE=query_dense`
- M551 promoted temperatures: `0.025/0.050`
- M551 promoted `RESIDUAL_SCALE=0.025`
- qrels only for held-out evaluation.

## Baseline

Same-task M551 promoted seed552 baseline on:

```text
FiQA2018, SCIDOCS, TRECCOVID
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `m551_shared_locked_support_residual_dream_lite` | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |

## Smokes

All rows below are deltas against the same-task M551 baseline.

| Variant | Selected epochs | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `dense_tail` | `[3, 2, 4]` | -0.00403 | -0.00016 | +0.00158 | -0.00952 | +0.00076 |
| `mixed50` | `[3, 2, 4]` | -0.00128 | -0.00023 | +0.00146 | -0.00076 | +0.00065 |
| `mixed25` | `[3, 2, 4]` | -0.00078 | -0.00011 | -0.00104 | -0.00077 | +0.00016 |
| `mixed50_rankguard` | `[0, 2, 0]` | +0.00080 | -0.00111 | -0.00260 | -0.00389 | -0.00259 |
| `random_rankguard` | `[0, 2, 0]` | +0.00116 | -0.00088 | -0.00260 | -0.00342 | -0.00269 |

Remote results:

```text
/home/huoju/leask/runs/ii42-m554-dream-hard-negative-posting-v1/dense_tail_pool512_seed552/m554_dense_tail_pool512_seed552.json
/home/huoju/leask/runs/ii42-m554-dream-hard-negative-posting-v1/mixed_pool512_seed552/m554_mixed_pool512_seed552.json
/home/huoju/leask/runs/ii42-m554-dream-hard-negative-posting-v1/mixed25_pool512_seed552/m554_mixed25_pool512_seed552.json
/home/huoju/leask/runs/ii42-m554-dream-hard-negative-posting-v1/mixed50_rankguard_seed552/m554_mixed50_rankguard_seed552.json
/home/huoju/leask/runs/ii42-m554-dream-hard-negative-posting-v1/random_rankguard_seed552/m554_random_rankguard_seed552.json
```

## Interpretation

Hard negatives are not useless, but they do not improve the first-stage
surface enough to replace M551.

- Dense-tail negatives improve recall and dense overlap, but damage top-rank
  behavior.
- Mixed negatives reduce the damage but still do not beat M551 on NDCG/MRR.
- Qrels-free rank guard can increase NDCG slightly, but it does so by selecting
  earlier epochs and losing recall, MRR, and dense overlap.

This matches the current diagnosis: DREAM-style candidate competition is real,
but naive harder candidate sets and stricter rank gates introduce a
top-rank/admission tradeoff rather than a clean improvement.

## Decision

Do not scale M554 to broad10. Keep M551 as the promoted BM25-free first-stage
milestone.

The paper value that has survived local validation is:

- candidate-set/listwise competition is useful when support is locked;
- frozen-LM likelihood proxies are not sufficient;
- harder candidate sets produce diagnostics, but not a better milestone;
- rank-interface gates need a better objective than zero-drop top-k/top1
  constraints.

The next valuable route is no longer more DREAM-lite micro-tuning.  It should
either implement a materially closer DREAM interface, or return to the broader
encoder/posting roadmap using M551 as the stable teacher-fit baseline.
