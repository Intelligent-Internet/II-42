# M1260 Large-Movement Exception Anatomy

## Question

M1259 showed that the movement-constrained policy has the best macro frontier,
but its hard veto blocks a few high-value `reserve1` movements.

M1260 asks a narrower question:

> Inside the rejected large-movement region, do existing qrels-free movement
> features expose a supported positive exception bucket?

If yes, a bounded exception replay may be justified.  If no, we should stop
movement-threshold work and move the signal into source/objective design.

## Runs

Smoke:

- `runs/m1260_large_movement_exception_anatomy_smoke_v1/`
- rejected query count was only `1`, so it was not informative.

Full `shared15`:

- JSON:
  `runs/m1260_large_movement_exception_anatomy_v1/m1260_large_movement_exception_anatomy.json`
- Markdown:
  `runs/m1260_large_movement_exception_anatomy_v1/m1260_large_movement_exception_anatomy.md`

## Rejected Region

Rejected means M1258/M1259 would fall back from `reserve1` to `reserve0` because
the low-tail atom caused either:

- `top10_new > 0`
- or `top100_new >= 3`

Full rejected-region summary:

| Count | Help | Harm | HelpRate | HarmRate | dRecall | dMAP | dNDCG | dMRR | dCUB | MeanScore |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 165 | 14 | 41 | 0.0848 | 0.2485 | -0.000036 | +0.000365 | -0.003206 | -0.000174 | -0.000510 | -0.100531 |

The rejected region is net negative.  The veto is directionally justified.

## Exception Search

No supported bucket with positive mean score was found.

The best supported buckets were still negative, for example:

| Bucket | Count | HelpRate | HarmRate | MeanScore |
| --- | ---: | ---: | ---: | ---: |
| `top100_rank_shift:down` | 25 | 0.0400 | 0.2400 | -0.003431 |
| `top100_new:1` | 37 | 0.1892 | 0.1622 | -0.012947 |
| `reason:large100` | 72 | 0.0694 | 0.2639 | -0.014166 |
| `top100_jaccard:small` | 60 | 0.1333 | 0.1833 | -0.062038 |
| `top100_new:3-4` | 86 | 0.0581 | 0.2791 | -0.091631 |

The top individual helpful movements are real, but sparse.  They mostly involve
`top10_new == 1` or `top100_new == 3`, which are also common in high-harm rows.
That means the current movement feature family cannot safely distinguish
large-movement rescue from large-movement damage.

## Decision

Stop movement-threshold/exception tweaking.

Keep the evidence:

- M1258/M1259 prove native movement context is useful.
- M1260 proves hard vetoed large movement is net risky.
- M1260 also proves the current qrels-free movement buckets do not expose a
  supported safe exception region.

Do not run another movement exception replay from the same feature family.

## Next Step

Move to `M1261` source/objective construction.

The next route should use the learned evidence as training structure:

1. `signed_sum_s1` remains the positive query-time delta signal.
2. CUB-specific teacher/action-source structure remains the target surface.
3. Movement-risk should be used as a constraint or penalty inside source
   construction/objective design, not as a hard post-hoc threshold.
4. Large-movement rescue must be learned or constructed with additional signal;
   it is not separable by the current movement buckets.
5. The first gate for M1261 should be target/harm separability before native
   replay.

This aligns with the broader M1000+ diagnosis:

> Useful added atoms exist, but safe qrels-free selection remains the bottleneck.
> Current post-hoc gates are too weak; the signal must be built into the source
> or objective.

## Artifacts

- Script: `scripts/audit_m1260_large_movement_exception_anatomy.py`
- Smoke JSON:
  `runs/m1260_large_movement_exception_anatomy_smoke_v1/m1260_large_movement_exception_anatomy.json`
- Full JSON:
  `runs/m1260_large_movement_exception_anatomy_v1/m1260_large_movement_exception_anatomy.json`
