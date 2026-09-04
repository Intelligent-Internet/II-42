# M678 M677 Selection Mismatch Audit

## Scope

M678 audits the M677 depth-ladder trace to explain why the 3x training probe
kept dense-equivalence gates but failed to improve over M675 seed `6547`.

No model is trained in this audit.  It uses the M677 epoch trace from:

`runs/m677_depth_ladder_m674_guard_v1/m677_depth3x_m674_guard_seed6547/m677_depth3x_m674_guard_seed6547.json`

## Key Observation

The current checkpoint selector mostly rewards safe O@256 movement after the
hard gates pass.  In M677 this selected epoch `32`, but epoch `32` already has
a negative dev MAP@100 delta.

This explains the final result:

- dense-equivalence gates remain clean;
- O@256 remains positive;
- test/full MAP and NDCG drift slightly negative.

The issue is not lack of training steps.  The issue is that deeper optimization
continues to find safe O@256 movement that is not rank-useful.

## Epoch Trace

Selected trace rows from the M677 dev split:

| Epoch | Step | Loss | O@100 | O@256 | MAP@100 | NDCG@10 | Recall@100 | Support |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 9 | `0.007093480` | `0.000000000` | `+0.000023674` | `-0.000001352` | `+0.000120069` | `0.000000000` | `0.000000000` |
| 2 | 18 | `0.007054660` | `0.000000000` | `+0.000058594` | `-0.000001478` | `+0.000120069` | `0.000000000` | `-0.000000016` |
| 3 | 27 | `0.007017880` | `0.000000000` | `+0.000061487` | `-0.000001478` | `+0.000120069` | `0.000000000` | `-0.000000048` |
| 4 | 36 | `0.006985810` | `0.000000000` | `+0.000058594` | `-0.000001266` | `+0.000120069` | `0.000000000` | `-0.000000075` |
| 5 | 45 | `0.006950060` | `0.000000000` | `+0.000091146` | `-0.000000043` | `0.000000000` | `0.000000000` | `-0.000000159` |
| 10 | 90 | `0.006814170` | `0.000000000` | `+0.000026042` | `+0.000023825` | `0.000000000` | `0.000000000` | `-0.000000393` |
| 12 | 108 | `0.006768590` | `0.000000000` | `0.000000000` | `-0.000019216` | `0.000000000` | `0.000000000` | `-0.000000536` |
| 18 | 162 | `0.006672000` | `0.000000000` | `+0.000052083` | `-0.000053979` | `0.000000000` | `0.000000000` | `-0.000000759` |
| 24 | 216 | `0.006607080` | `0.000000000` | `+0.000046074` | `-0.000008860` | `0.000000000` | `0.000000000` | `-0.000000938` |
| 30 | 270 | `0.006583230` | `0.000000000` | `+0.000046074` | `-0.000087111` | `0.000000000` | `0.000000000` | `-0.000000962` |
| 32 | 288 | `0.006561410` | `0.000000000` | `+0.000098683` | `-0.000087111` | `0.000000000` | `0.000000000` | `-0.000001065` |
| 36 | 324 | `0.006542780` | `0.000000000` | `+0.000069748` | `-0.000033632` | `0.000000000` | `0.000000000` | `-0.000001097` |

## Best Epochs by Different Criteria

Best dev O@256 epochs:

| Epoch | O@256 | MAP@100 | NDCG@10 | Support | Loss |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 32 | `+0.000098683` | `-0.000087111` | `0.000000000` | `-0.000001065` | `0.006561410` |
| 5 | `+0.000091146` | `-0.000000043` | `0.000000000` | `-0.000000159` | `0.006950060` |
| 33 | `+0.000072641` | `-0.000087111` | `0.000000000` | `-0.000001037` | `0.006560250` |

Best dev MAP@100 epochs:

| Epoch | O@256 | MAP@100 | NDCG@10 | Support | Loss |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 10 | `+0.000026042` | `+0.000023825` | `0.000000000` | `-0.000000393` | `0.006814170` |
| 5 | `+0.000091146` | `-0.000000043` | `0.000000000` | `-0.000000159` | `0.006950060` |
| 6 | `+0.000058594` | `-0.000000043` | `0.000000000` | `-0.000000203` | `0.006913880` |

## Interpretation

M675 selected epoch `5`, which is the more balanced point in this trace.  It
has almost the best dev O@256, near-zero dev MAP regression, and a much smaller
support-cosine movement than the later selected M677 epoch `32`.

M677 selected epoch `32`, which has the best dev O@256 but a clearly negative
dev MAP delta.  The full shared15 result then follows that signal: O@256 stays
positive, while MAP/NDCG/MRR fail to preserve the M675 positive ranking delta.

## Decision

The next fix should not be more depth on the exact same selection rule.

The next first-stage experiment should change checkpoint selection, not the
training scope.  Selection must remain dense-only:

1. keep the same M674/M675 hard dense-equivalence gates;
2. add a stronger support-cosine movement penalty or floor;
3. include dense rank-margin / score-calibration deformation once available;
4. use O@256 only after support and dense-rank geometry stay safe;
5. evaluate selected candidates on test/full shared15;
6. use qrels metrics only as post-hoc promotion evidence, not as training loss
   or checkpoint-selection objective;
7. reject any rule that requires BM25, reranker, qrels loss, or
   dataset-specific tuning.

This keeps M678 inside the first-stage contract: qrels explain whether a
dense-only selector is worth promoting, but they do not drive the learned
compiler or the selector.
