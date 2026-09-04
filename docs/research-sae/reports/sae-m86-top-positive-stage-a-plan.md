# SAE M86 Top-Positive Stage-A Plan

Status: planned. M85 active-budget probing shows k512 immediately improves
Recall/NDCG, but early MRR remains below the Stage-A gate. That points to a
specific top-positive ordering issue rather than pure candidate coverage.

## Change

M86 adds an optional positive-margin loss:

```text
softplus(margin + logsumexp(negative_scores) - logsumexp(positive_scores))
```

This is narrower than multi-positive CE. It does not try to rewrite the whole
teacher distribution; it only asks the sparse score to keep at least one labeled
positive above the negative pool. The intended role is to repair MRR/top-rank
tax after teacher KL has already established the semantic score shape.

## Initial Runs

Only launch these if M85 k512 still misses MRR:

| Run | Active dims | KL | Pairwise | Positive margin | Recon |
| --- | ---: | ---: | ---: | ---: | ---: |
| `m86-k512-margin005` | 512 | 4.0 | 0.5 | 0.05 | 0.25 |
| `m86-k512-margin010` | 512 | 4.0 | 0.5 | 0.10 | 0.25 |
| `m86-k384-margin010` | 384 | 4.0 | 0.5 | 0.10 | 0.25 |

## Decision Rule

If M86 passes at k512 but not k384, Stage A is closed at a higher active budget
and the next phase is compression/adaptive support. If M86 still fails at k512,
the remaining blocker is not scalar ranking supervision and Stage A needs a
representation change before Stage B.
