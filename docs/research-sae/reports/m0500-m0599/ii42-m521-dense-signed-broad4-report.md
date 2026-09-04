# M521 Dense-Signed Broad4 Promotion Gate

M521 promotes the M520 winner, `dense_signed`, from the FiQA target ablation to
a broad4 route-subset gate.  The purpose is to check whether the direct
row-int8 dense-coordinate support target is a general route direction rather
than a FiQA-only artifact.

Training remains dense-only: no BM25 and no qrels in the loss.  Qrels are used
only by the route evaluator.

## Run

- Host: `spark-1`
- Output: `outputs/m521/dense_signed_broad4/`
- Remote run dir: `/home/huoju/leask/runs/ii42-m521-dense-signed-broad4-v1`
- Tasks: `ArguAna, FiQA2018, SCIDOCS, TRECCOVID`
- Target mode: `dense_signed`
- Route prefixes: `64,96,128`
- Route training groups: `121`
- Elapsed: `1213.695s`

The run reused the M520 container dependency directory.  Transformers still
reported remote-code download messages despite offline env flags; this must be
fixed with an explicit model revision before a larger promotion run.

## Macro Matrix

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m511_lora_support_candidates_p64` | 0.46527 | 0.64067 | 0.52570 | 0.24172 | 0.82038 | 0.51453 |
| `m511_lora_support_candidates_p96` | 0.46943 | 0.65469 | 0.51963 | 0.24563 | 0.87384 | 0.60140 |
| `m511_lora_support_candidates_p128` | 0.47213 | 0.65634 | 0.51985 | 0.24663 | 0.90510 | 0.66063 |
| `m506_structural_candidates_p64` | 0.47205 | 0.65534 | 0.51451 | 0.24777 | 0.89963 | 0.73592 |
| `m506_structural_candidates_p96` | 0.47560 | 0.66043 | 0.52338 | 0.24919 | 0.95473 | 0.84263 |
| `m506_structural_candidates_p128` | 0.47668 | 0.66254 | 0.52335 | 0.24932 | 0.97676 | 0.89803 |
| `route_subset_materialized_dense` | 0.48043 | 0.66173 | 0.53335 | 0.25565 | 1.00000 | 1.00000 |
| `route_subset_teacher_row_int8_dense` | 0.48032 | 0.66040 | 0.53348 | 0.25556 | 1.00000 | 1.00000 |

## Per-Task Learned Best

| Task | Source | NDCG@10 | R@100 | MAP@100 | Cand R@100 | Touch |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `ArguAna` | `p128` | 0.41843 | 1.00000 | 0.28472 | 0.99875 | 0.87184 |
| `FiQA2018` | `p96` | 0.46922 | 0.87786 | 0.41721 | 0.96391 | 0.68870 |
| `SCIDOCS` | `p128` | 0.22510 | 0.57891 | 0.15463 | 0.90609 | 0.65673 |
| `TRECCOVID` | `p128` | 0.77795 | 0.16857 | 0.13036 | 0.73680 | 0.36413 |

## Interpretation

`dense_signed` survives broad4.  The p128 learned route reaches roughly the
same macro NDCG as the non-learned structural p64 route while touching fewer
documents: `0.47213` NDCG at `0.66063` touch, compared with structural p64
`0.47205` at `0.73592` touch.

It does not yet replace dense.  The gap to row-int8 dense is about `0.0082`
NDCG, `0.0136` MRR, and `0.0089` MAP.  R@100 is close to dense macro, but this
is partly a task mix effect; candidate recall remains the main bottleneck:
`0.90510` learned p128 vs dense `1.0`.

The result is better framed as a compression route with real signal, not a
complete dense substitute.  The direction is still alive because it improves
touch substantially while keeping much of dense quality.  The next question is
whether the remaining candidate gap is just prefix/support capacity or a deeper
encoder limit.

## Next Step

M522 should run a prefix/capacity sweep on the same `dense_signed` broad4 gate:

1. Prefixes `128,160,192` to see whether candidate recall closes smoothly.
2. Larger route groups if p160/p192 helps but remains below dense.
3. A pinned PPLX revision and tokenizer setting before any broad8/bire run.

Promotion rule: continue only if a higher prefix closes at least half of the
dense NDCG gap while keeping touch clearly below dense.  Otherwise the next
work should move to deeper first-stage training rather than more route tuning.
