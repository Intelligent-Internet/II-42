# M672 Broad50 M671 Guard Report

## Scope

M672 expands M671 from the 10% test split to a broader shared15 split:

- `train=40%`
- `dev=10%`
- `test=50%`
- test queries per seed: `671`

The mechanism is unchanged from M671:

- query-side compiler only;
- frozen document postings and index geometry;
- `output_blend_scale=0.25`;
- output active-lock;
- preserve dense top100/top256 hits;
- preserve P1 top10/top20/top100 membership;
- no BM25, reranker, learned gate, or qrels loss.

## Result Matrix

| Seed | Pass | Selected | Failed checks | O@100 | O@256 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | support |
| ---: | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `true` | `e8/s72` | `` | `0.000000000` | `+0.000118722` | `0.000000000` | `-0.000024596` | `-0.000019818` | `0.000000000` | `0.000000000` | `-0.000000461` |
| 6546 | `false` | `e10/s90` | `cub_safe` | `0.000000000` | `+0.000192882` | `0.000000000` | `-0.000009995` | `0.000000000` | `+0.000011447` | `-0.000002864` | `-0.000000668` |
| 6547 | `true` | `e9/s81` | `` | `0.000000000` | `+0.000213878` | `0.000000000` | `-0.000022881` | `-0.000024520` | `+0.000007055` | `0.000000000` | `-0.000000572` |
| 6548 | `false` | `e11/s99` | `cub_safe` | `0.000000000` | `+0.000195347` | `0.000000000` | `+0.000271077` | `+0.000311411` | `+0.000775194` | `-0.000022927` | `-0.000000763` |

Aggregate:

- pass count: `2/4`;
- selected rejected checkpoints: `0/4`;
- query-level O@100 regressions: `0`;
- query-level O@256 regressions: `0`;
- query-level Recall@100 regressions: `0`;
- minimum macro O@256 delta: `+0.000118722`;
- mean macro O@256 delta: `+0.000180207`;
- mean MAP@100 delta: `+0.000053401`;
- mean NDCG@10 delta: `+0.000066768`;
- mean MRR@20 delta: `+0.000198424`;
- mean CUB delta: `-0.000006448`.

## Interpretation

M672 validates the M671 stability constraint on a much larger surface for
top100 and top256 dense-equivalence:

- no O@100 regression;
- no O@256 regression;
- no Recall@100 regression;
- positive O@256 movement on every seed;
- positive mean MAP/NDCG/MRR.

The remaining failure is candidate upper bound. Seeds `6546` and `6548` lose a
small amount of Recall@1000 while still preserving P1 top100 membership. This
is consistent with the guard: it protects top100 but not the full candidate
pool used by CUB.

This is not a reason to abandon the route. It is a precise next constraint:
the broader first-stage gate requires preserving P1 candidate coverage at
top1000, not only top100.

## Decision

Do not promote M671 yet, because M672 broad50 fails `cub_safe` on 2/4 seeds.

Keep the route:

- the dense topK guard is effective;
- the P1 topK guard fixes top100 Recall stability;
- broad50 shows useful dense tail movement and positive average ranking
  metrics.

Next step:

- extend the existing qrels-free P1 preservation guard to include top1000:
  `--output-preserve-p1-top-k-values 10,20,100,1000`;
- rerun broad50 on the failing seeds first;
- accept only if CUB, Recall@100, O@100, O@256, and support gates all pass.
