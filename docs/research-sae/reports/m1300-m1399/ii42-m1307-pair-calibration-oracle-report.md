# M1307 Pair Calibration Oracle Native Replay

## Question

M1301-M1306 stopped simple set selection and protected marginal objectives over
the M1288 pair/witness candidate rows.  M1307 tests one remaining possibility:
maybe the atom set is usable, but the coordinated impact calibration is wrong.

This run freezes the M1288 pair/witness selected atom set and changes only the
impact geometry:

- raw impacts
- uniform L1-matched impacts
- square-root L1-matched impacts
- power-0.75 L1-matched impacts

It also includes a query-level oracle over this calibration family with a
top95 floor.  If even this oracle cannot improve the pair/witness control, the
calibration branch should stop before any learned calibrator.

## Run

```bash
python3 scripts/replay_m1307_pair_calibration_oracle_native.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources calib,oracle \
  --budgets 192 \
  --risk-penalties 0.5 \
  --scales 0.025,0.035,0.05 \
  --feature-groups pair \
  --calibrations raw,uniform_l1,sqrt_l1,pow075_l1 \
  --safe-top95-floor 0.95 \
  --limit-queries 25 \
  --output-root runs/m1307_pair_calibration_oracle_limit25_v1
```

Artifacts:

- JSON:
  `runs/m1307_pair_calibration_oracle_limit25_v1/m1307_native.json`
- Markdown:
  `runs/m1307_pair_calibration_oracle_limit25_v1/m1307_native.md`

Surface:

- datasets: `arguana`, `cqadupstack`, `fiqa`, `scidocs`
- query limit: `25` per dataset
- variants: `13`

## Result

Best variants:

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | Top100 | Top256 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pow075_l1_pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.960263 | 0.957250 | 0.962207 | 0.981611 | 0.008333 |
| `sqrt_l1_pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.960263 | 0.957500 | 0.962109 | 0.981611 | 0.008333 |
| `uniform_l1_pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959737 | 0.957500 | 0.962207 | 0.981611 | 0.008333 |
| `raw_pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.957000 | 0.962109 | 0.981611 | 0.008333 |
| `oracle_pair_risk0.5_b192` | 0.670000 | 0.520000 | 45 | 0 | 0.974737 | 0.972250 | 0.975586 | 0.981611 | 0.008333 |

No calibration variant passes the promotion gate:

- required pair success lift over raw: at least `+0.005`
- required top95: at least raw control
- required min dataset top95: at least `0.90`

The best calibration improves top95 by only `+0.001053` and does not improve
pair success.  The oracle over calibrations becomes safer on top95 but loses
pair success and fixes fewer boundary pairs.

## Per-Dataset Shape

Best rows by dataset:

| Dataset | Best Shape | PairSuccess | Fixed | Top95 | TargetRecall |
| --- | --- | ---: | ---: | ---: | ---: |
| `arguana` | `pow075_l1_s0.05` / `sqrt_l1_s0.05` | 0.708333 | 21 | 0.959514 | 0.969912 |
| `cqadupstack` | `pow075_l1_s0.05` / raw / sqrt | 0.666667 | 11 | 0.956579 | 0.993976 |
| `fiqa` | oracle / `*_s0.035` | 0.647059 | 8 | 0.975439-0.980117 | 0.994048 |
| `scidocs` | `pow075_l1_s0.05` / `sqrt_l1_s0.05` | 0.763158 | 14 | 0.960000 | 0.976744 |

The calibration family mostly changes head overlap, not boundary-pair success.
The effect is therefore too small and too indirect to justify a learned
calibrator.

## Interpretation

M1307 fails as a capacity probe.

This is not just a poor choice of one calibration.  The query-level oracle over
the calibration family also fails to beat the raw pair/witness control.  That
means the M1288/M1301 movement is not waiting for a small monotonic impact
calibration fix.

Combined with M1301-M1306:

- set coverage adds tiny movement but spends protected head geometry;
- protected objectives recover head geometry but lose movement;
- immediate native-effect or margin oracles become safe but mostly inert;
- impact calibration preserves or slightly improves overlap but does not add
  boundary movement.

The bottleneck is no longer selector mechanics or scalar impact calibration.
The current pair/witness post-hoc compiler interface is too weak.

## Decision

Stop the M1288 pair/witness post-hoc family:

- no broader replay of M1301-M1307;
- no more coverage grids, protected marginal oracles, or calibration grids over
  the same selected atom rows;
- no learned calibrator unless a future source changes the action/candidate
  construction first.

Next work should move to source/objective construction where harm separation
is built in before replay.  The strongest retained signals remain:

- M1225 CUB-specific teacher as proof that useful atoms exist;
- M1244 action-source oracle as proof that source/action conditioning matters;
- M1251-M1253 `signed_sum_s1` as a query-time macro-positive source, but not a
  row-safe default.

The next valid branch should convert those signals into an intrinsic training
objective or candidate source, then rerun the same gate:

1. target/harm separability before native replay;
2. small native smoke;
3. LODO or broader row audit only after smoke passes;
4. full shared15 only if row harm is observable or structurally reduced.
