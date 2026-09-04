# M670 Top256 Preserve Guard Report

## Scope

M670 tests a conservative first-stage projection guard after M669 showed that
fixed scalar damping fails by narrow top256 tail churn.

The change is query-side only:

- freeze document postings and index geometry;
- keep `output_blend_scale=0.25`;
- keep output active-lock;
- add `--output-preserve-dense-top-k-values 100,256`;
- do not use BM25, reranker, learned gate, or qrels loss.

The guard compares each candidate query against the P1 baseline for the same
query. If the candidate loses any P1-preserved dense hit at top100 or top256,
that query is reverted to its P1 query posting.

## Result Matrix

| Seed | Pass | Selected | Rejected | Failed checks | Test gain/loss dense docs | O@100 | O@256 | Recall@100 | MAP@100 | CUB | support |
| ---: | --- | --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `true` | `e11/s187` | `false` | `` | `3/0` | `+0.000296296` | `+0.000296905` | `0.000000000` | `-0.000024961` | `0.000000000` | `-0.000001037` |
| 6546 | `true` | `e2/s34` | `false` | `` | `0/0` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000674` | `0.000000000` | `-0.000000052` |
| 6547 | `false` | `e11/s187` | `false` | `recall_safe` | `5/0` | `+0.000274281` | `+0.000304927` | `-0.000256410` | `-0.000036742` | `0.000000000` | `-0.000001001` |
| 6548 | `true` | `e1/s17` | `false` | `` | `0/0` | `0.000000000` | `+0.000060877` | `0.000000000` | `-0.000002109` | `0.000000000` | `0.000000000` |

Aggregate:

- pass count: `3/4`;
- selected rejected checkpoints: `0/4`;
- dense-hit lost docs: `0`;
- dense-hit gained docs: `8`;
- query-level O@100 regressions: `0`;
- query-level O@256 regressions: `0`;
- minimum macro O@100 delta: `0`;
- minimum macro O@256 delta: `0`;
- mean MAP@100 delta: `-0.000016122`.

## Interpretation

M670 proves that the M669 failure mode is controllable. The top100/top256
dense-hit guard removes the recurring O@256 tail losses without falling back to
untrained checkpoints.

However, it is not yet promotable. Seed `6547` fails `recall_safe` even though
it gains dense hits and loses no dense hits. This is an important diagnostic:
preserving dense hits is not sufficient to preserve P1 top100 retrieval
coverage. A query can gain dense-aligned documents while displacing a
qrels-positive document that was not part of the protected dense-hit set.

That failure is not evidence that deeper training is needed. It is evidence
that the projection guard is still missing a baseline-stability constraint.

## Decision

Keep:

- query-side post-training compiler;
- active-lock;
- top100/top256 dense-hit preservation guard;
- strict first-stage gate.

Do not promote:

- M670 as the first-stage default, because `seed6547` fails Recall@100.

Next step:

- add a qrels-free P1 top100 set preservation option;
- keep every P1 top100 document in the candidate top100, or revert that query;
- rerun the same 4 seeds;
- accept only if dense overlap, Recall@100, CUB, and support gates all pass.

This remains a first-stage dense-equivalence/stability experiment. The next
guard should use only P1 baseline ranking and dense ranking, not BM25 or qrels.
