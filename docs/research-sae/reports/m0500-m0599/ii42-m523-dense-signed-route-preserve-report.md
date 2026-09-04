# M523 Dense-Signed Route Preserve Probe

M523 tests whether M522's remaining candidate gap can be closed by a more
protective route phase instead of simply widening prefix.  It keeps the same
M522 broad4 `dense_signed` setup, but changes the route phase to M514
`load_preserve` with three route epochs.

Training is still dense-only: no BM25 and no qrels in the loss.

## Run

- Host: `spark-1`
- Remote run dir:
  `/home/huoju/leask/runs/ii42-m523-dense-signed-route-preserve-v1`
- Local output: `outputs/m523/dense_signed_route_preserve/`
- Tasks: `ArguAna, FiQA2018, SCIDOCS, TRECCOVID`
- Target mode: `dense_signed`
- Route preset: `load_preserve`
- Route epochs override: `3`
- Route positive-k override: `16`
- Route negative-k override: `32`
- Route prefixes: `160,192`
- Elapsed: `1589.751s`

The run completed cleanly and left no active Docker task.  The same PPLX
revision/tokenizer warnings remain and should be fixed before broad promotion.

## Macro Matrix

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m511_lora_support_candidates_p160` | 0.46999 | 0.64830 | 0.52768 | 0.24404 | 0.89423 | 0.69788 |
| `m511_lora_support_candidates_p192` | 0.47352 | 0.65287 | 0.52558 | 0.24532 | 0.91127 | 0.72964 |
| `m506_structural_candidates_p160` | 0.47650 | 0.66391 | 0.52316 | 0.24951 | 0.98847 | 0.93017 |
| `m506_structural_candidates_p192` | 0.47674 | 0.66194 | 0.52316 | 0.24950 | 0.99308 | 0.94991 |
| `route_subset_materialized_dense` | 0.48043 | 0.66173 | 0.53335 | 0.25565 | 1.00000 | 1.00000 |
| `route_subset_teacher_row_int8_dense` | 0.48032 | 0.66040 | 0.53348 | 0.25556 | 1.00000 | 1.00000 |

## Comparison To M522

| Run | Best source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M522 | `p192` | 0.47640 | 0.66053 | 0.52394 | 0.24817 | 0.94054 | 0.73666 |
| M523 | `p192` | 0.47352 | 0.65287 | 0.52558 | 0.24532 | 0.91127 | 0.72964 |

## Route Trace

| Epoch | Loss | Load | Positive overlap | Negative overlap |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 2.23424 | 4.91491 | 5.30071 | 0.01117 |
| 2 | 1.59551 | 7.80519 | 3.92171 | 0.00696 |
| 3 | 1.42847 | 10.61947 | 3.18275 | 0.00836 |

## Interpretation

M523 is a negative result.  Route loss decreased across three epochs, but the
load metric increased strongly and the final candidate recall regressed:
`0.94054 -> 0.91127` at p192 versus M522.  NDCG also fell from `0.47640` to
`0.47352`.

This suggests the `load_preserve` objective and longer route phase are not
preserving the useful dense-signed support.  They likely overfit or distort the
learned support surface while reducing the candidate coverage that M522 had
recovered through prefix capacity.

The actionable conclusion is to keep M522 as the current learned route
frontier.  Do not continue this exact `load_preserve + 3 epochs` branch.

## Next Step

The next useful M524 should not be another load-preserve rerun.  Two options
remain plausible:

1. Pin PPLX revision/tokenizer first, then rerun the M522 p192 gate as the
   clean reproducible baseline.
2. Add an explicit candidate-coverage auxiliary objective or diagnostic that
   directly rewards teacher TopK coverage, instead of relying on the current
   route overlap/load proxy.

Promotion criterion should remain strict: beat M522 p192 candidate recall and
NDCG at comparable touch before broad8 or BEIR/MTEB promotion.
