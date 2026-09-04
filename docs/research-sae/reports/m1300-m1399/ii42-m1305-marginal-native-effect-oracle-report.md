# M1305 Marginal Native-Effect Oracle Replay

## Question

M1290 and M1291 showed that single-atom native labels are too local.  M1305
tests a set-level variant:

> Can a greedy marginal native-effect oracle select atoms that improve
> dense-boundary pair success while preserving protected top95?

The oracle candidate source is the M1288 pair/witness pool.  Each step runs
the native scorer and accepts an atom only if it:

- improves boundary pair success immediately;
- introduces no pair regression;
- keeps query-level top95 at or above `0.95`.

## Run

Command:

```bash
python3 scripts/replay_m1305_marginal_native_effect_oracle.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,oracle \
  --budgets 192 \
  --risk-penalties 0.5 \
  --scales 0.05 \
  --feature-groups pair \
  --oracle-candidate-counts 12 \
  --oracle-max-selects 4 \
  --safe-top95-floors 0.95 \
  --limit-queries 25 \
  --output-root runs/m1305_marginal_native_effect_oracle_limit25_v1
```

Artifacts:

- JSON: `runs/m1305_marginal_native_effect_oracle_limit25_v1/m1305_native.json`
- Markdown: `runs/m1305_marginal_native_effect_oracle_limit25_v1/m1305_native.md`

## Result

| Variant | PairSuccess | Fixed | Regressed | Top95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_pair_risk0.5_b192_s0.05` | 0.700000 | 54 | 0 | 0.959211 | 0.981611 | 0.008333 |
| `oracle_pair_risk0.5_c12_m4_f0.95_s0.05` | 0.526667 | 2 | 0 | 1.000000 | 0.001268 | 0.000000 |

Per dataset, oracle success remains close to the baseline pair-success floor:

| Dataset | Oracle PairSuccess | Pair Control PairSuccess | Oracle Fixed | Pair Fixed | Oracle Top95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.489583 | 0.708333 | 0 | 21 | 1.000000 |
| `cqadupstack` | 0.500000 | 0.666667 | 1 | 11 | 1.000000 |
| `fiqa` | 0.544118 | 0.647059 | 1 | 8 | 1.000000 |
| `scidocs` | 0.578947 | 0.763158 | 0 | 14 | 1.000000 |

## Interpretation

M1305 fails strongly.

The oracle is safe, but almost inert.  Immediate pair-success improvement is
too sparse: the set-level movement in M1288 does not decompose into a small
sequence of atoms that individually cross the boundary while preserving top95.

This is consistent with M1291:

- single atoms are weak or misleading;
- useful movement is probably margin accumulation or coordinated score-shape
  movement;
- requiring immediate boundary crossing rejects most atoms that may be useful
  in combination.

## Decision

Do not train from M1305 labels and do not scale this oracle.

One final valid check remains before stopping this source family:

- replace the hard pair-success step objective with a smoother score-margin or
  rank-margin objective;
- keep the same protected top95/no-regression constraints;
- test whether margin accumulation can recover pair-control movement.

If that also fails, the M1288 pair/witness source should be treated as a
teacher-side capacity result that cannot currently be distilled into a
protected native set objective.
