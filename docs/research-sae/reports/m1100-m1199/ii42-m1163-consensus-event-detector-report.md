# M1163 Consensus Event Detector

## Purpose

M1162 showed that M1161 is a clean event-query recovery policy but its full
shared15 impact is small because only 149/1342 queries are in the M1147 event
surface.

M1163 tests a cheap next question:

> Can the existing M1144 and M1145 deployable predictions be combined into a
> high-precision event detector without training another model?

Artifacts:

- `scripts/audit_m1163_consensus_event_detector.py`
- `runs/m1163_consensus_event_detector_v1/consensus_event_detector.json`
- `runs/m1163_consensus_event_detector_v1/summary.md`

## Inputs

- M1144 native stream feature predictions:
  `runs/m1144_native_tail_gate_features_v1/features_predictions.json`
- M1145 candidate-boundary feature predictions:
  `runs/m1145_candidate_boundary_tail_gate_v1/features_predictions.json`

M1163 does not retrain.  It only tests consensus/support policies over existing
LODO predictions.

## Best Clean Policy

Best row-floor-clean policy:

- mode: `m1144_supported_by_m1145`
- M1144 confidence threshold: `0.60`
- M1145 support threshold: `0.30`
- non-abstain: `138`
- action precision: `0.181159`
- event recall: `0.315436`

Macro deltas over the 1342-query M1143 shared15 universe:

| Policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1162 projected M1161 | +0.000008 | +0.000234 | +0.000112 | +0.000251 | +0.000447 |
| M1163 best clean detector | +0.000074 | +0.001419 | +0.001400 | +0.000900 | +0.000000 |

The best M1163 detector is row-floor clean: all min dataset deltas are `0.0`.

## Interpretation

This is a useful correction to the earlier M1144/M1145 stop signal.

The individual models failed:

- M1144 alone was too low precision.
- M1145 alone over-acted and could not abstain.

Their consensus/support combination is materially better:

- M1144 supplies abstain discipline.
- M1145 supplies candidate-boundary directional support.
- The combination recovers a clean, non-trivial fraction of the M1143 oracle
  surface without new training.

However, M1163 is not the same object as M1161:

- M1163 selects protected-tail actions directly.
- M1161 selects generated atom movements inside the M1155/M1147 event surface.

So M1163 does not automatically solve unified atom proposal coverage.  It does
show that a deployable event detector may be recoverable from existing signals.

## Decision

Continue, but with a structural change:

1. Keep M1161 as the best event-query atom recovery policy.
2. Keep M1163 as the best current deployable event detector.
3. Next step should join them by rebuilding atom proposal/training rows on the
   M1163-selected event set, not by more M1161 threshold tuning.

If that join fails, then the atom branch remains event-local and the protected
tail detector becomes a separate native route rather than a unified posting
compiler improvement.
