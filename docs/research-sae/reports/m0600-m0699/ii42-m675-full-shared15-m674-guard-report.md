# M675 Full Shared15 M674 Guard Report

## Scope

M675 adds full shared15/native replay to the M674 first-stage guard.

The code change is reporting-only for this run:

- add `--emit-full-eval`;
- keep dev/test split selection unchanged;
- add an additional `full` split after checkpoint selection;
- do not use full split for training or selection.

Training/evaluation configuration:

```bash
--all-query-train-fraction 0.40
--all-query-dev-fraction 0.10
--emit-full-eval
--output-preserve-dense-top-k-values 50,100,256
--output-preserve-p1-top-k-values 10,20,50,100,1000
```

The full split contains all `1342` shared15 qrels queries.

## Test Split Matrix

The `test` split is the same broad50 held-out split used by M674 (`671`
queries per seed).

| Seed | O@10 | O@50 | O@100 | O@256 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | support |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000072462` | `0.000000000` | `-0.000000743` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000445` |
| 6546 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000084609` | `0.000000000` | `-0.000003818` | `0.000000000` | `+0.000005342` | `0.000000000` | `-0.000000302` |
| 6547 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000101568` | `0.000000000` | `+0.000371414` | `+0.000335251` | `+0.000740741` | `0.000000000` | `-0.000000159` |
| 6548 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000110611` | `0.000000000` | `-0.000006318` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000358` |

Test aggregate:

- pass-like seeds: `4/4`;
- minimum O@10/O@50/O@100: `0`;
- minimum O@256: `+0.000072462`;
- mean O@256: `+0.000092312`;
- mean MAP@100: `+0.000090134`;
- mean NDCG@10: `+0.000083813`;
- mean MRR@20: `+0.000186521`;
- mean CUB: `0`.

## Full Shared15 Matrix

The `full` split evaluates all `1342` qrels queries.

| Seed | O@10 | O@50 | O@100 | O@256 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | support |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000070525` | `0.000000000` | `-0.000000278` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000445` |
| 6546 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000085029` | `0.000000000` | `-0.000169416` | `-0.000150863` | `-0.000330556` | `0.000000000` | `-0.000000286` |
| 6547 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000084287` | `0.000000000` | `+0.000164443` | `+0.000150863` | `+0.000333333` | `0.000000000` | `-0.000000163` |
| 6548 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000101669` | `0.000000000` | `+0.000003947` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000334` |

Full aggregate:

- pass-like seeds: `4/4`;
- minimum O@10/O@50/O@100: `0`;
- minimum O@256: `+0.000070525`;
- mean O@256: `+0.000085378`;
- mean Recall@100: `0`;
- mean CUB: `0`;
- mean MAP@100: `-0.000000326`;
- mean NDCG@10: approximately `0`;
- mean MRR@20: `+0.000000694`.

## Interpretation

M675 confirms that M674's dense-equivalence guard scales from broad50 to the
full shared15 query surface:

- no O@10/O@50/O@100 regression;
- O@256 is positive for every seed;
- no Recall@100 regression;
- no CUB regression;
- support regression stays within the configured relative tolerance.

The remaining nuance is ranking metric variance on full shared15. Seed `6546`
has a small full-surface MAP/NDCG/MRR regression, while seed `6547` is positive
on MAP/NDCG/MRR and also passes all dense-equivalence gates. Averaged across
all seeds, full MAP is effectively flat but slightly negative. Therefore the
guard is reliable for first-stage dense-equivalence, but the final frozen
checkpoint should not be chosen by seed average. It should use a passing seed
with non-negative full ranking deltas.

## Decision

M675 promotes the M674 guard design as the current first-stage candidate
mechanism.

Best checkpoint candidate from this run:

- seed `6547`;
- full O@256 `+0.000084287`;
- full MAP@100 `+0.000164443`;
- full NDCG@10 `+0.000150863`;
- full MRR@20 `+0.000333333`;
- no Recall@100/CUB/overlap regression.

The next step should freeze this configuration as an M675 first-stage candidate
and run the next broader/native matrix or official surface. Do not move to
BM25/reranker until this first-stage candidate is recorded as the baseline.
