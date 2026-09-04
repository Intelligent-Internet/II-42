# SAE Full15 Candidate-Budget Training Report

## Scope

This pass follows the adaptive-gate diagnostics. The gate oracle showed that
the student often contains the right documents across different weights, but
runtime-safe query features could not reliably select the oracle weight in
leave-one-dataset-out evaluation. The next hypothesis was therefore:

```text
Train the student under a query atom traversal budget instead of only tuning
the final SAE score weight.
```

## Implementation

The text-to-atoms trainer now has an opt-in candidate-budget loss:

```text
--candidate-budget-loss-weight
--candidate-budget-top-dims
--candidate-budget-margin
```

The loss reuses the coverage-teacher examples, but it scores candidates with
only the top-N predicted query atoms. This approximates the future inverted
index path where a query can only open a limited number of atom posting lists.

The loss is separate from the previous coverage loss. The older coverage loss
uses the full predicted query atom vector; candidate-budget loss evaluates the
same positives/negatives after applying the fixed query atom budget.

## Runs

All runs use:

- shared teacher: `shared_sae_8192_64`;
- full 15-BEIR qrels-preserving matrix;
- dense-teacher pair supervision;
- retrieval loss weight `0.05`;
- student weight grid `0.25, 0.5, 0.75, 1.0, 1.5`.

| Run | Best Recall@100 | Best MRR@20 | Best NDCG@10 | Best MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `full15-shared-dense-teacher-token-char` | 0.8240 | 0.8231 | 0.7111 | 0.6827 |
| `full15-shared-dense-budget8-token-char` | 0.8239 | 0.8195 | 0.7110 | 0.6819 |
| `full15-shared-dense-budget16-token-char` | 0.8280 | 0.8264 | 0.7154 | 0.6837 |
| `full15-shared-dense-budget32-token-char` | 0.8277 | 0.8247 | 0.7162 | 0.6833 |
| `full15-shared-dense-coverage-token-char` | 0.8223 | 0.8218 | 0.7094 | 0.6783 |
| `full15-shared-dense-teacher-largepairs-token-char` | 0.8254 | 0.8133 | 0.7039 | 0.6771 |

Budget16 loss-weight sweep:

| Loss Weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Elapsed Seconds |
| ---: | ---: | ---: | ---: | ---: | ---: |
| `0.005` | 0.8240 | 0.8255 | 0.7135 | 0.6826 | 1404.2 |
| `0.010` | 0.8280 | 0.8264 | 0.7154 | 0.6837 | 1234.9 |
| `0.020` | 0.8307 | 0.8270 | 0.7182 | 0.6872 | 1377.7 |
| `0.040` | 0.8324 | 0.8291 | 0.7225 | 0.6913 | 1349.1 |
| `0.080` | 0.8327 | 0.8255 | 0.7223 | 0.6891 | 1439.9 |

## Findings

Candidate-budget loss is the first full15 follow-up that improves all four main
metrics over the dense-pair baseline.

The best balanced run is:

```text
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-token-char/
```

Compared with the previous best balanced run:

| Metric | Previous | Budget16 | Delta |
| --- | ---: | ---: | ---: |
| Recall@100 | 0.8240 | 0.8324 | +0.0084 |
| MRR@20 | 0.8231 | 0.8291 | +0.0060 |
| NDCG@10 | 0.7111 | 0.7225 | +0.0114 |
| MAP@100 | 0.6827 | 0.6913 | +0.0086 |

Top-8 is too strict. It keeps the model close to baseline and does not give the
same quality lift. Top-32 is also strong and gives the best NDCG@10, but top-16
is the better balanced point and is more aligned with a future fast traversal
budget.

The budget16 loss-weight sweep shows that `0.01` was under-regularized.
Quality improves through `0.04`. At `0.08`, Recall@100 still rises slightly,
but MRR@20 and MAP@100 decline. That is the first observed balance tradeoff, so
`0.04` is the current balanced frontier.

The previous coverage loss and larger dense-pair budget are weaker. This
supports the representation-level hypothesis: forcing the query encoder to make
useful top atoms matters more than adding more pair examples or full-vector
coverage pressure.

## Decision

Promote candidate-budget loss with `top_dims = 16` and loss weight `0.04` as
the new balanced training frontier.

Do not remove the dense-teacher pair loss. The current best result comes from
combining dense-pair supervision with candidate-budget pressure.

## Next Steps

1. Evaluate candidate-budget student latents in the physical block/max simulator
   to confirm that quality gains also reduce opened postings or candidate rows.
2. Test whether `top_dims = 8, 16, 32` under stronger loss keeps the same
   quality ordering once physical candidate cost is included.
3. Add query/document asymmetric capacity so query atoms can specialize for
   traversal while document atoms stay stable for indexing.
