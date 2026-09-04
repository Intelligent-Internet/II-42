# M720 Pair-Interaction Native Replay

M720 replays the M719 pair-interaction selector through the native
`P1.3 / M549U` signed-dot scorer. This is the first check that M719's
target-set recovery signal can move dense-boundary pairs in the real native
path.

This remains first-stage only:

- no BM25;
- no reranker;
- no qrels-driven objective;
- no learned gate;
- no dataset-specific tuning;
- frozen doc posting/index geometry.

## Motivation

M719 showed that dense-boundary pair-interaction features can recover useful
target atom sets. That result could still have failed if selected atoms did not
move native scores safely. M720 tests the native movement directly.

The key guard is dense-faithfulness: pair success can improve only if overlap
does not fall below the floor. The canary gate is:

- eval pair success improves over baseline;
- `fixed_pairs > regressed_pairs`;
- `mean_top95_overlap >= 0.97`.

## Run

```bash
python3 scripts/train_m720_pair_interaction_native_replay.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --limit-queries 25 \
    --budgets 32,64 \
    --scales 0.02,0.05,0.1 \
    --output-root runs/m720_pair_interaction_native_replay_v1
```

Runtime was `30.96s`.

## Result

M720 passes the canary gate.

Best safe eval row:

| Variant | Pair success | Baseline | Fixed | Regressed | Top95 | Target recall | Target selected | Negative-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_hgb_rp1_b64_s0.05` | 0.623333 | 0.520000 | 31 | 0 | 0.971579 | 0.860495 | 0.530078 | 0.001172 |

Highest movement row:

| Variant | Pair success | Baseline | Fixed | Regressed | Top95 | Target recall | Target selected | Negative-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_hgb_rp1_b64_s0.1` | 0.723333 | 0.520000 | 61 | 0 | 0.943684 | 0.860495 | 0.530078 | 0.001172 |

The high-scale row is not acceptable as a dense-equivalence candidate because
it spends too much top95 overlap. It is still important diagnostically: it
shows that the pair-interaction selector has strong causal leverage over the
dense-boundary pairs.

## Eval Surface

Safe eval rows with `top95 >= 0.97`:

| Variant | Pair success | Baseline | Fixed | Regressed | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `pair_hgb_rp1_b64_s0.05` | 0.623333 | 0.520000 | 31 | 0 | 0.971579 |
| `pair_hgb_rp05_b64_s0.05` | 0.620000 | 0.520000 | 30 | 0 | 0.971842 |
| `pair_hgb_rp0_b64_s0.05` | 0.610000 | 0.520000 | 27 | 0 | 0.970526 |
| `pair_hgb_rp05_b32_s0.05` | 0.570000 | 0.520000 | 15 | 0 | 0.981579 |
| `pair_hgb_rp1_b32_s0.05` | 0.570000 | 0.520000 | 15 | 0 | 0.982368 |
| `pair_hgb_rp0_b32_s0.05` | 0.566667 | 0.520000 | 14 | 0 | 0.980000 |
| `pair_hgb_rp0_b64_s0.02` | 0.553333 | 0.520000 | 10 | 0 | 0.986579 |
| `pair_hgb_rp05_b64_s0.02` | 0.550000 | 0.520000 | 9 | 0 | 0.986842 |
| `pair_hgb_rp1_b64_s0.02` | 0.550000 | 0.520000 | 9 | 0 | 0.988158 |

The scale curve is coherent:

- `0.10` gives large boundary movement but violates the overlap floor.
- `0.05` gives a meaningful safe gain.
- `0.02` is very safe but smaller.

## Interpretation

This is a real positive signal for the current first-stage route.

The fast M716-M718 iterations were not prematurely abandoned due to shallow
training. They failed because scalar candidate-row features could not identify
the target atoms. M719 changed the feature surface, and M720 confirms that the
new surface can safely move native dense-boundary pairs at moderate scale.

The result also clarifies the next risk. We should not simply scale the
high-movement setting. The useful region is a constrained scale/budget band
where pair success rises while top95 overlap stays above floor.

## Decision

Promote to broader validation, not to final model.

Next step M721:

1. Freeze the safe candidates `pair_hgb_rp1_b64_s0.05`,
   `pair_hgb_rp05_b64_s0.05`, and `pair_hgb_rp0_b64_s0.05`.
2. Replay on a broader shared15/native surface with the same first-stage gates.
3. Add dense overlap@100/@256 and candidate upper bound if available.
4. Reject if gains are isolated to the four canary datasets.
5. Reject if broader top95/top100 overlap drops below floor.

## Files

- `scripts/train_m720_pair_interaction_native_replay.py`
- `runs/m720_pair_interaction_native_replay_v1/m720_summary.json`
- `runs/m720_pair_interaction_native_replay_v1/m720_report.md`

