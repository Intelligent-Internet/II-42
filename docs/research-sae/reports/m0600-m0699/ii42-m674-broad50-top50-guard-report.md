# M674 Broad50 Top50 Guard Report

## Scope

M674 addresses the only remaining M673 weakness: small negative O@50 deltas.

It keeps the same first-stage constraints:

- frozen document postings and index geometry;
- query-side compiler only;
- `output_blend_scale=0.25`;
- output active-lock;
- no BM25, reranker, learned gate, or qrels loss.

The guard is now:

```bash
--output-preserve-dense-top-k-values 50,100,256
--output-preserve-p1-top-k-values 10,20,50,100,1000
```

This protects the full reported dense-overlap surface used by the current
goal: O@10/O@50/O@100/O@256, plus P1 top1000 candidate coverage.

## Result Matrix

| Seed | Pass | Failed checks | O@10 | O@50 | O@100 | O@256 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | support |
| ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `true` | `` | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000072462` | `0.000000000` | `-0.000000743` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000445` |
| 6546 | `true` | `` | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000084609` | `0.000000000` | `-0.000003818` | `0.000000000` | `+0.000005342` | `0.000000000` | `-0.000000302` |
| 6547 | `true` | `` | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000101568` | `0.000000000` | `+0.000371414` | `+0.000335251` | `+0.000740741` | `0.000000000` | `-0.000000159` |
| 6548 | `true` | `` | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000110611` | `0.000000000` | `-0.000006318` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000358` |

Aggregate:

- pass count: `4/4`;
- query-level O@100 regressions: `0`;
- query-level O@256 regressions: `0`;
- query-level Recall@100 regressions: `0`;
- query-level CUB regressions: `0`;
- minimum macro O@10 delta: `0`;
- minimum macro O@50 delta: `0`;
- minimum macro O@100 delta: `0`;
- minimum macro O@256 delta: `+0.000072462`;
- mean macro O@256 delta: `+0.000092312`;
- mean MAP@100 delta: `+0.000090134`;
- mean NDCG@10 delta: `+0.000083813`;
- mean MRR@20 delta: `+0.000186521`;
- mean CUB delta: `0`.

## Interpretation

M674 is the cleanest current first-stage result.

It resolves the sequence of observed failure modes:

- M668: scalar blend failed by top256 boundary churn;
- M670: dense topK preservation fixed O@256 but exposed Recall loss;
- M671: P1 top100 preservation fixed Recall loss;
- M672: broad50 exposed CUB loss;
- M673: P1 top1000 preservation fixed CUB loss but left O@50 slightly weak;
- M674: dense/P1 top50 preservation fixes O@50 while retaining O@256 gains.

This is still conservative. It does not optimize qrels and does not use BM25.
It preserves the P1 baseline candidate set and only allows movement that
passes the dense-equivalence guard.

## Decision

Promote M674 as the current broad50 first-stage candidate.

Do not call the overall goal complete yet. The next required gate is a full
shared15/native replay or the largest feasible native replay, using the same
guard:

```bash
--output-preserve-dense-top-k-values 50,100,256
--output-preserve-p1-top-k-values 10,20,50,100,1000
```

If full shared15 preserves the M674 broad50 properties, then this route can be
considered the next first-stage frozen candidate.
