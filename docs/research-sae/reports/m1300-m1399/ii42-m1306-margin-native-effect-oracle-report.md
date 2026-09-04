# M1306 Margin Native-Effect Oracle Replay

## Question

M1305 showed that requiring immediate pair-success improvement is too strict:
the oracle becomes safe but almost inert.  M1306 relaxes the step objective:

- still use the M1288 pair/witness candidate pool;
- still require no pair regression;
- still require query-level top95 floor `0.95`;
- greedily add atoms that improve native score-margin delta, not immediate
  pair-success.

This tests whether M1288 movement can be recovered through margin accumulation
under a protected floor.

## Run

Command:

```bash
python3 scripts/replay_m1305_marginal_native_effect_oracle.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,oracle_margin \
  --budgets 192 \
  --risk-penalties 0.5 \
  --scales 0.05 \
  --feature-groups pair \
  --oracle-candidate-counts 16 \
  --oracle-max-selects 8 \
  --safe-top95-floors 0.95 \
  --limit-queries 25 \
  --output-root runs/m1306_margin_native_effect_oracle_limit25_v1
```

Artifacts:

- JSON: `runs/m1306_margin_native_effect_oracle_limit25_v1/m1305_native.json`
- Markdown: `runs/m1306_margin_native_effect_oracle_limit25_v1/m1305_native.md`

The script schema is still `m1305_marginal_native_effect_oracle_v1`; M1306 is
the margin-objective run using the updated `oracle_margin` source.

## Result

| Variant | PairSuccess | Fixed | Regressed | Top95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_pair_risk0.5_b192_s0.05` | 0.700000 | 54 | 0 | 0.959211 | 0.981611 | 0.008333 |
| `oracle_margin_pair_risk0.5_c16_m8_f0.95_s0.05` | 0.536667 | 5 | 0 | 0.990263 | 0.188332 | 0.000000 |

Per dataset:

| Dataset | Oracle PairSuccess | Pair Control PairSuccess | Oracle Fixed | Pair Fixed | Oracle Top95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.500000 | 0.708333 | 1 | 21 | 0.990283 |
| `cqadupstack` | 0.500000 | 0.666667 | 1 | 11 | 0.992105 |
| `fiqa` | 0.558824 | 0.647059 | 2 | 8 | 0.989474 |
| `scidocs` | 0.592105 | 0.763158 | 1 | 14 | 0.989474 |

## Interpretation

M1306 fails.

Margin accumulation is less inert than the strict M1305 pair-success oracle,
but it remains far below the pair-only control:

- fixed pairs improve from M1305's `2` to `5`, but pair control fixes `54`;
- target recall rises to `0.188332`, but pair control is `0.981611`;
- top95 is very safe, but that safety comes from not moving enough.

The conclusion is now stronger:

> The M1288 pair/witness movement cannot be recovered by a small protected
> marginal oracle over the same candidate rows.

This does not invalidate M1288 as a teacher-side movement result.  It says the
movement is not available as a small sequence of locally safe additions under
the current native score geometry.

## Decision

Stop the M1288 pair/witness protected set-objective family:

- no more coverage-weight grids;
- no more protected-head scoring terms over the same candidate rows;
- no more single-atom or marginal native-effect labels from this source;
- no full shared15 scale-up for M1301-M1306.

The next route should change representation or scoring geometry, not selector
mechanics.  The likely bottleneck is that the useful pair/witness signal needs
a coordinated query-vector/posting delta with calibrated weights, not a sparse
subset of locally safe added atoms.
