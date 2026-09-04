# M1286 Source-Attribution Compression Audit

## Question

M710 showed that the expanded atom interface exposes the target atoms, but the
current order cannot compress them into a practical budget.  M1286 tests a
structural alternative:

> If candidate atoms retain source-document attribution, can a setwise
> coverage/diversity compression recover more target atoms than scalar
> `abs_vote` ordering?

This is a pre-native compression audit.  It does not run native replay.

## Surface

Limit-25 smoke over the M704/M710 expanded-interface datasets:

- `arguana`
- `cqadupstack`
- `fiqa`
- `scidocs`

Settings:

- expanded source: `corpus_support_tail`
- doc atom head: `48`
- candidate cap: `1536`
- budgets: `96`, `192`, `384`
- coverage pool limit: `512`

## Result

Eval frontier:

| Variant | TargetRecall | VisibleTargetRecall | TargetVis | NegOnly |
| --- | ---: | ---: | ---: | ---: |
| `abs_vote_b384` | 0.703234 | 0.715023 | 0.983513 | 0.057292 |
| `coverage_l025_b384` | 0.703234 | 0.715023 | 0.983513 | 0.057292 |
| `coverage_l05_b384` | 0.703234 | 0.715023 | 0.983513 | 0.057292 |
| `coverage_l1_b384` | 0.703234 | 0.715023 | 0.983513 | 0.057292 |
| `coverage_l2_b384` | 0.703234 | 0.715023 | 0.983513 | 0.057292 |
| `band_balanced_b384` | 0.703234 | 0.715023 | 0.983513 | 0.057292 |

The same collapse happens at lower budgets:

- budget 192: all variants `0.518072` target recall;
- budget 96: all variants `0.379835` target recall.

## Interpretation

M1286 does not find source-attribution compression lift.

The important result is not merely that coverage-aware variants lose.  They
collapse to the same selection as `abs_vote`.  This means source-document
coverage, as represented here, does not add useful selection degrees of
freedom.  The expanded candidate rows already saturate source-doc coverage, so
greedy coverage does not recover the M704 oracle gap.

This supports the M710 diagnosis but sharpens it:

- the wide source exposes targets;
- scalar atom order fails;
- source-doc diversity alone also fails;
- the missing information is likely pair/witness-level or set-objective
  supervision, not generic document coverage.

## Decision

Do not native-replay M1286.

Do not continue with:

- source-doc coverage greedy variants;
- band-balanced source quotas over the current aggregated atom rows;
- more diversity weights over the same source-doc attribution.

The next route must keep richer witness identity or train directly against a
pair/set objective.  Concretely, the next useful step is an observability audit:
can pair/witness-level features separate M704 oracle-selected atoms from
non-selected visible atoms?  If not, this expanded-interface route needs a new
supervision source rather than another compression heuristic.

## Artifacts

- Script:
  `scripts/audit_m1286_source_attribution_compression.py`
- JSON:
  `runs/m1286_source_attribution_compression_limit25_v1/m1286_source_attribution_compression.json`
- Markdown:
  `runs/m1286_source_attribution_compression_limit25_v1/m1286_source_attribution_compression.md`
