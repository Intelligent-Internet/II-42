# M759 Context-Policy Route Summary

## Decision

M758 changes the current route status from "oracle-only" to "deployable
but still small". The qrels-free global proposal pool is useful, and a simple
deterministic native-context policy can safely extract non-zero gains on
shared15 without spending dense overlap.

The learned M757 query-listwise selector is not the right next primitive. It
either no-ops under strict constraints or selects rows that slightly regress
MAP. The stronger signal is the auditable `margin_bundle` policy family from
M758.

## Evidence

| Run | Proposal | Policy | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M754 oracle | top32 | per-query oracle | 271 | +0.001374 | +0.001711 | +0.000291 | +0.000043 | +0.000000 | 1 |
| M754 oracle | top64 | per-query oracle | 271 | +0.001395 | +0.001711 | +0.000291 | +0.000050 | +0.000000 | 1 |
| M757 | top64 | learned constrained | 40 | -0.000005 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0 |
| M758 | top16 | deterministic context | 224 | +0.000246 | +0.000332 | +0.000000 | +0.000032 | +0.000000 | 1 |
| M758 | top32 | deterministic context | 72 | +0.000057 | +0.000684 | +0.000066 | +0.000012 | +0.000000 | 1 |
| M758 | top64 | deterministic context | 197 | +0.000177 | +0.000213 | +0.000000 | +0.000013 | +0.000000 | 1 |

## Best M758 Policies

Top16:

```json
{
  "margin_delta_100": 0.0,
  "overlap_128": 0.99,
  "overlap_256": 0.99,
  "score_name": "score_mean_margin",
  "threshold": 0.6226385235786438
}
```

Top32:

```json
{
  "margin_delta_100": null,
  "overlap_128": 0.99,
  "overlap_256": null,
  "score_name": "margin_bundle",
  "threshold": 1.3649893760681153
}
```

Top64:

```json
{
  "margin_delta_100": 0.0001,
  "overlap_128": 0.99,
  "overlap_256": 1.0,
  "score_name": "margin_bundle",
  "threshold": 0.364365565776825
}
```

`margin_bundle` is an inference-available policy score built from native
context score margins and global proposal counts. It does not inspect test
qrels or per-query qrels-derived accepted rows.

## Interpretation

The bottleneck is no longer proposal existence. M754 proved a large safe oracle
gap, and M758 proved a smaller but real deployable recovery. The remaining
problem is policy calibration:

- learned delta predictors overfit or become too conservative;
- strict dense-equivalence constraints make no-op attractive;
- deterministic native-context margins are currently more stable than black-box
  regressors;
- recall stays flat because the safe policy mostly improves rank geometry
  inside the existing top100 boundary.

This is a useful route, but it is not yet a major retrieval breakthrough. The
gain is real and safe, but still one order of magnitude below the M754 oracle
ceiling.

## Next Step

Promote M758 to the next validation target:

1. Replay the deterministic context policy on broader native surfaces, starting
   with shared15 variants and then the official/native DB path.
2. Stress-test top16 vs top32 vs top64. Top16 currently has the best
   deterministic MAP/CUB balance; top32 has the strongest NDCG/MRR movement;
   top64 has the broadest applied-query coverage.
3. Keep the policy auditable. Do not replace it with another black-box selector
   until a broader replay proves the deterministic surface is stable.
4. If broader replay fails, inspect whether the failure is caused by the
   global proposal list, margin score calibration, or the native-context
   overlap floor.

## Stop Condition

Stop this line if deterministic context policy loses dense overlap or candidate
upper bound on broader native validation, or if the only gains remain confined
to the current shared15 slice. Otherwise, this is the current best route for
support-safe, retrieval-conditioned unified posting improvement.
