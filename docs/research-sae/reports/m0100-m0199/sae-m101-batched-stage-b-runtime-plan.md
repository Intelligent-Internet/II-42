# SAE M101 Batched Stage-B Runtime Plan

Date: 2026-05-22

## Summary

M99/M100 prove that candidate-level residual ranking is the current Stage-B
mainline. The remaining issue is engineering shape: M99/M100 train and score
through row-level Python loops. That is acceptable for research evidence, but
not for a runtime scorer or fast iteration.

M101 converts the same ranker into a batched tensor path and validates export
parity. It does not change the model objective or claim full-corpus product
readiness.

## Goals

- Preserve the M99/M100 scoring contract:
  BM25/SAE scores, score-derived rank/margin features, and query distribution
  features only.
- Train the ranker through padded tensors instead of row loops.
- Export model weights and feature normalization metadata.
- Verify parity between row-loop scoring and batched/exported scoring.
- Compare quality against M98, fixed BM25+SAE, and M100.

## Acceptance Gate

M101 passes if:

- batched training remains clearly above M98 on eval and holdout;
- batched/export parity has strict top-10 parity or documented tie-only
  differences;
- score max absolute delta is within float tolerance;
- training/inference throughput is better than the row-loop path;
- dense remains training/control only, not runtime input.

## Non-Goals

- No full-corpus native index changes.
- No mutable SQL/API productization.
- No dense-removal product claim.

