# M673 Broad50 P1 Top1000 Guard Report

## Scope

M673 extends M672 after broad50 exposed small CUB regressions on seeds `6546`
and `6548`.

The M673 guard remains first-stage and qrels-free:

- frozen document postings and index geometry;
- query-side compiler only;
- `output_blend_scale=0.25`;
- output active-lock;
- preserve dense top100/top256 hits;
- preserve P1 top10/top20/top100/top1000 membership;
- no BM25, reranker, learned gate, or qrels loss.

The added constraint is:

```bash
--output-preserve-p1-top-k-values 10,20,100,1000
```

This protects P1 candidate upper bound by preserving the P1 top1000 set.

## Result Matrix

| Seed | Pass | Selected | Failed checks | O@10 | O@50 | O@100 | O@256 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | support |
| ---: | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `true` | `e11/s99` | `` | `0.000000000` | `+0.000008184` | `0.000000000` | `+0.000072462` | `0.000000000` | `-0.000000743` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000461` |
| 6546 | `true` | `e10/s90` | `` | `0.000000000` | `-0.000002419` | `0.000000000` | `+0.000101414` | `0.000000000` | `-0.000000456` | `0.000000000` | `+0.000011447` | `0.000000000` | `-0.000000397` |
| 6547 | `true` | `e5/s45` | `` | `0.000000000` | `-0.000105188` | `0.000000000` | `+0.000101568` | `0.000000000` | `+0.000371592` | `+0.000335251` | `+0.000740741` | `0.000000000` | `-0.000000159` |
| 6548 | `true` | `e9/s81` | `` | `0.000000000` | `-0.000111428` | `0.000000000` | `+0.000110611` | `0.000000000` | `-0.000006318` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000370` |

Aggregate:

- pass count: `4/4`;
- query-level O@100 regressions: `0`;
- query-level O@256 regressions: `0`;
- query-level Recall@100 regressions: `0`;
- query-level CUB regressions: `0`;
- minimum macro O@256 delta: `+0.000072462`;
- mean macro O@256 delta: `+0.000096514`;
- mean MAP@100 delta: `+0.000091019`;
- mean NDCG@10 delta: `+0.000083813`;
- mean MRR@20 delta: `+0.000188047`;
- mean CUB delta: `0`.

## Interpretation

M673 fixes the M672 candidate-upper-bound failure while keeping the O@256 gains
positive on every seed. This is the strongest first-stage result in the current
sequence:

- M668 showed scalar damping was not enough;
- M670 fixed dense top256 loss but exposed Recall loss;
- M671 fixed Recall loss but exposed CUB loss on broad50;
- M673 fixes CUB loss and keeps average MAP/NDCG/MRR positive.

The result is still conservative. It mostly allows safe movement inside the
protected P1 top1000 candidate set.

Remaining issue: O@50 is slightly negative on three seeds, even though O@10,
O@100, O@256, Recall@100, and CUB are safe. This happens because the current
dense guard protects top100/top256 but not top50. Since the goal tracks
overlap@10/@50/@100/@256, the next version should add dense top50 preservation
before promotion.

## Decision

Keep M673 as the current best broad50 first-stage candidate.

Do not promote it as final yet. The next narrow fix is clear:

```bash
--output-preserve-dense-top-k-values 50,100,256
```

Optionally mirror P1 top50 as well:

```bash
--output-preserve-p1-top-k-values 10,20,50,100,1000
```

If that preserves the M673 pass rate and removes O@50 regression, then the
route should move to full shared15/native replay.
