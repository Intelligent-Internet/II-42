# M671 P1 TopK Preserve Guard Report

## Scope

M671 continues the first-stage dense-equivalence route after M670 fixed dense
top256 regressions but still had a Recall@100 regression on seed `6547`.

The M671 projection guard is still qrels-free:

- freeze document postings and index geometry;
- train only the query-side compiler;
- keep `output_blend_scale=0.25`;
- keep output active-lock;
- preserve dense hits at top100/top256;
- additionally preserve P1 baseline top10/top20/top100 membership;
- do not use BM25, reranker, learned gate, or qrels loss.

The new option is:

```bash
--output-preserve-p1-top-k-values 10,20,100
```

If a candidate query loses any P1 baseline topK member for these K values, that
query is reverted to the P1 query posting. This directly protects top100
retrieval coverage without inspecting qrels.

## Result Matrix

| Seed | Pass | Selected | Rejected | Failed checks | O@100 | O@256 | Recall@100 | MAP@100 | CUB | support | changed queries | O@256 gain queries |
| ---: | --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `true` | `e6/s102` | `false` | `` | `0.000000000` | `+0.000139415` | `0.000000000` | `-0.000021406` | `0.000000000` | `-0.000000405` | `11` | `5` |
| 6546 | `true` | `e4/s68` | `false` | `` | `0.000000000` | `+0.000026042` | `0.000000000` | `+0.000084400` | `0.000000000` | `-0.000000199` | `4` | `1` |
| 6547 | `true` | `e11/s187` | `false` | `` | `0.000000000` | `+0.000217818` | `0.000000000` | `-0.000003861` | `0.000000000` | `-0.000000886` | `20` | `8` |
| 6548 | `true` | `e1/s17` | `false` | `` | `0.000000000` | `+0.000060877` | `0.000000000` | `-0.000002109` | `0.000000000` | `0.000000000` | `3` | `2` |

Aggregate:

- pass count: `4/4`;
- selected rejected checkpoints: `0/4`;
- query-level O@100 regressions: `0`;
- query-level O@256 regressions: `0`;
- query-level Recall@100 regressions: `0`;
- minimum macro O@100 delta: `0`;
- minimum macro O@256 delta: `+0.000026042`;
- mean macro O@256 delta: `+0.000111038`;
- mean MAP@100 delta: `+0.000014256`.

## Interpretation

M671 is the first variant in this local sequence that satisfies the strict
first-stage gate across all four tested seeds while still producing positive
dense tail movement.

Compared with M670:

- M670 proved dense top100/top256 regressions were controllable;
- M670 still failed seed `6547` on Recall@100;
- M671 fixes that by preserving P1 top10/top20/top100 membership;
- the fix is qrels-free and therefore remains first-stage aligned.

The result is intentionally conservative. It does not try to improve top100
retrieval directly. It protects the existing P1 top100 set and allows movement
mainly in the top256 tail. That is consistent with the current evidence: the
main unstable boundary was top256, not top100.

## Decision

Keep M671 as the current best first-stage guard candidate.

Do not promote it to the final P1 replacement yet, because this is still a
four-seed shared15 validation. The next gate should be broader replay, not
more scalar tuning.

Recommended next step:

1. replay M671 on a broader shared15/native matrix;
2. compare against M549U/P1 baseline and M670;
3. verify that the P1 topK guard does not collapse useful movement on larger
   surfaces;
4. only then consider M671 as the next first-stage frozen candidate.

This route should continue. The failure mode was not training depth; it was an
under-specified projection surface. M671 adds the missing qrels-free baseline
stability constraint and gets a clean 4/4 pass.
