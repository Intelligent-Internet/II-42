# M679 Support-Aware Dense Selector Assessment

## Scope

M679 implements and tests the M678 recommendation: change checkpoint selection
without changing the first-stage training objective.

The selector remains first-stage and dense-only:

- no BM25
- no reranker
- no learned gate
- no qrels-driven loss
- no qrels-driven checkpoint selection
- frozen document posting/index geometry
- query-side/output compiler only

The code adds a selectable policy:

```text
--checkpoint-selection-policy support_floor_overlap
--selection-support-regression-tolerance 0.0000002
```

Default runner behavior remains `legacy`.

## Selector Rule

The `support_floor_overlap` selector ranks checkpoints by:

1. whether support cosine delta is within the configured support tolerance;
2. O@100 delta;
3. O@256 delta;
4. support cosine delta;
5. Recall@100 delta.

It deliberately excludes MAP/NDCG/MRR because those are qrels metrics and would
turn first-stage checkpoint selection into a retrieval-label selector.

## Result

M679 runs the same 36-epoch depth ladder as M677 with the support-aware
selector enabled.

| Run | Selector | Selected Epoch | Selected Step |
| --- | --- | ---: | ---: |
| M675 seed6547 | legacy short run | `5` | `45` |
| M677 depth3x | legacy | `32` | `288` |
| M679 depth3x | support_floor_overlap | `5` | `45` |

## Test Split Delta Comparison

| Metric | M675 | M677 | M679 |
| --- | ---: | ---: | ---: |
| O@10 | `0.000000000` | `0.000000000` | `0.000000000` |
| O@50 | `0.000000000` | `0.000000000` | `0.000000000` |
| O@100 | `0.000000000` | `0.000000000` | `0.000000000` |
| O@256 | `+0.000101568` | `+0.000042085` | `+0.000101568` |
| Recall@100 | `0.000000000` | `0.000000000` | `0.000000000` |
| MAP@100 | `+0.000371414` | `-0.000001445` | `+0.000371414` |
| NDCG@10 | `+0.000335251` | `-0.000004702` | `+0.000335251` |
| MRR@20 | `+0.000740741` | `0.000000000` | `+0.000740741` |
| support cosine | `-0.000000159` | `-0.000000830` | `-0.000000159` |

## Full Shared15 Delta Comparison

| Metric | M675 | M677 | M679 |
| --- | ---: | ---: | ---: |
| O@10 | `0.000000000` | `0.000000000` | `0.000000000` |
| O@50 | `0.000000000` | `0.000000000` | `0.000000000` |
| O@100 | `0.000000000` | `0.000000000` | `0.000000000` |
| O@256 | `+0.000084287` | `+0.000075627` | `+0.000084287` |
| Recall@100 | `0.000000000` | `0.000000000` | `0.000000000` |
| MAP@100 | `+0.000164443` | `-0.000008871` | `+0.000164443` |
| NDCG@10 | `+0.000150863` | `-0.000002727` | `+0.000150863` |
| MRR@20 | `+0.000333333` | `0.000000000` | `+0.000333333` |
| support cosine | `-0.000000163` | `-0.000000827` | `-0.000000163` |

## Interpretation

M679 is a useful correction to the research harness:

- M677 showed that longer training can preserve hard dense gates while
  selecting a worse ranking checkpoint.
- M678 traced that failure to selector priority: O@256 dominated support
  movement.
- M679 verifies that a dense-only support-aware selector avoids that failure
  and selects the safer M675/M676 checkpoint from the longer run.

This does not prove that longer training improves the model.  It proves that
longer training can be made safe to run without accidentally promoting a
support-drifted checkpoint.

## Decision

Keep `m675_full_shared15_m674_guard_seed6547` / M679 epoch `5` as the current
first-stage candidate.

Promote the `support_floor_overlap` selector as the safer default for future
first-stage depth ladders, but do not mark it as a quality breakthrough.

Next useful step:

1. Run a small multiseed M679 selector replay to confirm the selector does not
   overfit seed `6547`.
2. If multiseed is stable, run the broader native/official validation with the
   frozen M676/M679 candidate.
3. Do not move to BM25/reranker until this first-stage candidate is validated
   on the broader native/official surface.
