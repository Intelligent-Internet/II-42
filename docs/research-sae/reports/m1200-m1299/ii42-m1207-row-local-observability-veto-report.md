# M1207 Row-Local Observability Veto

M1207 tests whether the M1191 damage-veto frontier can be improved by adding
row-local movement observability.  It keeps the same action space and training
contract:

- default action: aggressive `top3` posting delta
- fallback action: conservative `top1` posting delta
- model: global leave-one-dataset-out logistic veto
- runtime features: qrels-free

The new information is collected from native baseline/top3/top1 rows:

- top100 entrant and exit score/rank/source statistics
- fused boundary score shifts
- entrant-vs-exit score margins
- top100 source-count and BM25/P1 composition changes

## Macro Result

| Variant | Take | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_pair` | 0.852 | +0.002599 | +0.005643 | +0.005712 | +0.004721 | +0.000419 |
| `aggressive` | 1.000 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `veto_any_metric_logistic` | 0.823 | +0.001878 | +0.003361 | +0.002607 | +0.002195 | +0.000091 |
| `veto_utility_or_cub_logistic` | 0.822 | +0.001780 | +0.003259 | +0.002616 | +0.002168 | +0.000104 |
| `veto_rank_metric_logistic` | 0.779 | +0.001884 | +0.003139 | +0.002333 | +0.002277 | +0.000077 |
| `conservative` | 1.000 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |

## Interpretation

This is a bounded positive result, not a breakthrough.

The new row-local observability makes every learned veto macro-safe and raises
CUB substantially versus M1191.  However, it gives back Recall/MAP and does not
beat the existing M1191 `veto_utility_or_cub_logistic` frontier overall.

The evidence now says the current top3/top1 action pair has a real oracle gap,
but qrels-free feature/classifier changes alone are not enough to close it.
Continuing to swap labels, thresholds, or classifiers would be a micro-tuning
loop.

## Decision

- Keep M1191 as the current best deployable control route.
- Keep M1207 as a safety-biased candidate and evidence that observability helps
  CUB, but do not promote it.
- Stop classifier/feature-only tuning on the same top3/top1 pair.

## Next Direction

Move from control-policy tuning to proposal-shape frontier testing.  The next
scientific question is whether the aggressive `top3` delta scale has a better
fixed operating point than 0.10.  A small scale frontier can determine whether
the CUB harm is simply over-amplification or structural.

## Artifacts

- JSON: `runs/m1207_row_local_observability_veto_v1/m1207_row_local_observability_veto.json`
- Markdown: `runs/m1207_row_local_observability_veto_v1/m1207_row_local_observability_veto.md`
- Script: `scripts/audit_m1207_row_local_observability_veto.py`
