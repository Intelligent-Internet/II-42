# M1219 Proposal Observability

M1219 diagnoses why M1218 failed before doing more replay or training.

Question:

> Can a qrels-free model using only baseline query atoms recover the M1217
> contrastive teacher atoms?

If the answer is no, then larger generated-posting training should not use the
same input shape.

## Hard-Row Smoke

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Variant | TargetRecall | Precision | AnyHit | HarmPrecision | HarmQuery | TargetQuery | PredCount | TargetCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `cooc_h0_top8` | 0.3200 | 0.0080 | 0.0643 | 0.0100 | 0.0723 | 0.1606 | 8.000 | 0.201 |
| `prior_top8` | 0.3200 | 0.0080 | 0.0643 | 0.0120 | 0.0843 | 0.1606 | 8.000 | 0.201 |
| `cooc_h0.5_top8` | 0.2400 | 0.0060 | 0.0482 | 0.0100 | 0.0723 | 0.1606 | 8.000 | 0.201 |
| `cooc_h1_top8` | 0.1800 | 0.0045 | 0.0361 | 0.0070 | 0.0562 | 0.1606 | 8.000 | 0.201 |

Hard rows showed some target recall, but precision was extremely low and harm
overlap was comparable to true target overlap.

## Full Shared15

| Variant | TargetRecall | Precision | AnyHit | HarmPrecision | HarmQuery | TargetQuery | PredCount | TargetCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `prior_top8` | 0.0899 | 0.0059 | 0.0425 | 0.0053 | 0.0410 | 0.2548 | 8.000 | 0.522 |
| `cooc_h0_top8` | 0.0827 | 0.0054 | 0.0380 | 0.0045 | 0.0343 | 0.2548 | 8.000 | 0.522 |
| `cooc_h0.5_top8` | 0.0685 | 0.0045 | 0.0335 | 0.0039 | 0.0298 | 0.2548 | 8.000 | 0.522 |
| `cooc_h1_top8` | 0.0456 | 0.0030 | 0.0231 | 0.0034 | 0.0253 | 0.2548 | 8.000 | 0.522 |

Full shared15 confirms the weak observability:

- best target recall is only `0.0899`
- best precision is only `0.0059`
- harm precision is nearly the same as target precision
- only `4.25%` of queries get any target hit from the best prior baseline

## Interpretation

This explains M1218.

The M1217 teacher is valid enough to replay safely, but its atoms are not
recoverable from baseline query-atom cooccurrence.  A tiny cooccurrence model
mostly proposes generic or harm-overlapping atoms, so native replay damages
CUB/Recall/NDCG.

This does not kill the contrastive teacher branch.  It kills the current input
shape.

## Decision

- Stop baseline-query-atom-only proposal models.
- Do not tune `harm_lambda`, `top_n`, or `scale`.
- Keep M1217 as a supervision target.
- Move to retrieval-conditioned candidate atom features.

## Next Direction

Run M1220: retrieval-conditioned candidate atom observability.

Instead of predicting atoms from baseline query atoms only, build candidate
atom rows from the native proposal sources already available at query time:

- high: `bm25_top_docs3_s0.10`
- mid: `bm25_top_docs3_s0.075`
- low: `fused_tail_docs3_s0.10`

For each candidate atom, use qrels-free features:

- high/mid/low signed delta
- high/mid/low absolute delta
- source presence flags
- train-fold contrastive atom prior
- base-query membership
- action/source agreement

Then measure held-out target recall/precision before replay.

Acceptance for continuing:

- target recall materially above M1219 `0.0899`
- precision materially above harm precision
- stable on hard-row and full shared15

If M1220 cannot separate target atoms from harm atoms, this branch should stop
before larger training.

## Artifacts

- Script: `scripts/audit_m1219_proposal_observability.py`
- Smoke JSON: `runs/m1219_proposal_observability_smoke_v1/m1219_proposal_observability.json`
- Smoke Markdown: `runs/m1219_proposal_observability_smoke_v1/m1219_proposal_observability.md`
- Full JSON: `runs/m1219_proposal_observability_v1/m1219_proposal_observability.json`
- Full Markdown: `runs/m1219_proposal_observability_v1/m1219_proposal_observability.md`
