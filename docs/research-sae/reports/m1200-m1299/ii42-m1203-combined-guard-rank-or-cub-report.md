# M1203 Combined Guard Rank-or-CUB Label

M1203 reruns the M1202 combined guard with the stricter
`rank_or_cub_harm` label to test whether the learned veto improves the
shared15 frontier.

## Result

The alternate label produced the same macro surface as M1202:

| Source | Take | Macro negative metrics | Dataset negative cells | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `proposal` | 1.000 | 1 | 7 | +0.000379 | +0.000833 | +0.000347 | +0.000938 | -0.000035 |
| `rule_o1000_099` | 0.882 | 0 | 9 | +0.000144 | +0.000560 | +0.000201 | +0.000306 | +0.000059 |
| `model_veto` | 0.756 | 0 | 6 | +0.000061 | +0.000336 | +0.000223 | +0.000709 | +0.000113 |

## Interpretation

`rank_or_cub_harm` does not create a new frontier beyond M1202.  It confirms
that the current qrels-free movement-feature guard has one useful learned
safety point, but it does not justify more label swapping on this script.

## Decision

- Treat M1203 as a negative extension.
- Keep M1202 `contract_harm` as the retained learned-guard artifact because it
  reaches the same result with the original contract framing.
- Move the next work to proposal source redesign or row-local harm-targeted
  training, not more guard-label variants.

## Artifacts

- JSON: `runs/m1203_combined_guard_rank_or_cub_shared15_v1/m1202_combined_guard_shared15.json`
- Markdown: `runs/m1203_combined_guard_rank_or_cub_shared15_v1/m1202_combined_guard_shared15.md`
- Script: `scripts/audit_m1202_combined_guard_shared15.py`
