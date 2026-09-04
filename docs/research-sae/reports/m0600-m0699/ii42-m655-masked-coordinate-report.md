# M655 / Masked Coordinate Compiler Report

Status: `masked_coordinate_failed`

M655 follows the M654 positive feasibility audit.  It freezes a coordinate mask
from train/dev and evaluates whether that mask transfers to held-out test rows.
The goal is to separate three questions:

1. Does train/dev discover reusable coordinate families?
2. Can a deployable global masked update improve ranking without qrels?
3. Does a fixed-mask oracle still have boundary-crossing headroom?

## Variant Matrix

| Variant | Mask source | Global scale | Status | all dO@100 | all dNDCG@10 | all dMAP@100 | all dR@100 | all dMRR@20 |
| --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| default | accepted one-off | 0.050 | failed | -0.051836 | -0.003661 | -0.004027 | -0.001628 | -0.003461 |
| min2 | stable candidate | 0.005-0.010 | failed | -0.009687 | +0.001794 | +0.003089 | -0.001302 | +0.003257 |
| min2 tiny | stable candidate | 0.001-0.002 | failed | -0.000391 | +0.000210 | +0.001082 | +0.000000 | +0.001017 |

The final/default M655 configuration is `min2 tiny`: `MIN_MASK_COUNT=2`,
`MASK_DIMS=24`, `GLOBAL_SCALE=0.001`, `GLOBAL_SCALE_CAP=0.002`.

## Final Run

Run:
`runs/ii42-m655-masked-coordinate-v1/m655_masked_coordinate_min2_tiny_seed6551/m655_masked_coordinate_min2_tiny_seed6551.json`

Mask profile:

| Field | Value |
| --- | ---: |
| mask entries | 24 |
| accepted-source entries | 0 |
| safe-source entries | 1 |
| candidate-source entries | 23 |
| train/dev accepted audits | 38 / 94 |
| train/dev safe-coordinate queries | 8 / 94 |

Final test deltas versus `m549_frozen`:

| Surface | dO@100 | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| global all | -0.000391 | +0.000210 | +0.001082 | +0.000000 | +0.001017 |
| global boundary | -0.001471 | +0.000000 | +0.000085 | +0.000000 | +0.000000 |
| fixed-mask oracle boundary | +0.000000 | +0.001871 | +0.000137 | +0.000000 | +0.000000 |

Final decision:

```json
{
  "failed_checks": [
    "oracle_boundary_recall_positive",
    "global_boundary_overlap_safe"
  ],
  "status": "masked_coordinate_failed"
}
```

## Interpretation

M655 is a useful negative result with one limited positive signal.

The positive signal is that a stable tiny mask can preserve all-query dense
overlap while improving MAP and MRR.  This means the coordinate families found
by M654 are not random noise.  They can perform conservative rank polishing.

The negative result is more important: neither global masked update nor
fixed-mask oracle moves Recall@100 on held-out boundary rows.  The fixed-mask
oracle can improve NDCG/MAP without losing dense overlap, but it does not cross
additional positives into top100.  Therefore the current fixed-mask abstraction
does not solve the core bottleneck.

## Current Blocks

1. One-off accepted coordinates overfit.  The default accepted-only mask caused
   large dense-overlap loss and metric regression.
2. Stable candidate masks are safer, but too weak for boundary crossing.
3. Tiny global movement can polish ranking, but it is not a recall-recovery
   mechanism.
4. Fixed-mask oracle accepted held-out updates, but those accepted updates only
   improved rank quality among already-covered positives, not Recall@100.
5. The next model cannot be a global vector or a hand-tuned fixed mask.  It
   must be query-conditioned and trained against crossing pressure.

## What Carries Forward

M654 remains the stronger result: coordinate-level safe movement exists.

M655 adds a constraint: the movement is not captured by a global frozen mask.
The viable direction is a retrieval-constrained generated-posting objective
that learns when and how to activate the M654 coordinate families per query,
while preserving dense support.

## Next Step

M656 should be a retrieval-constrained masked output-head trainer:

1. Freeze the stable coordinate family from M654/M655 as the allowed movement
   region.
2. Train a query-conditioned selector/head on train boundary cases.
3. Use a direct crossing loss between boundary positives and replaceable
   top100 tail negatives.
4. Add dense-support preservation loss over baseline top100 and dense top100.
5. Evaluate on held-out dev/test with no qrels-time acceptance gate.

Acceptance for M656 should require held-out boundary Recall@100 improvement
with all-query dense overlap no worse than `-0.001`, and MAP/NDCG/MRR not
materially negative.  If M656 also fails to move Recall, this branch should
stop and return to first-stage output-head design rather than more mask tuning.

## Artifacts

- Final JSON:
  `runs/ii42-m655-masked-coordinate-v1/m655_masked_coordinate_min2_tiny_seed6551/m655_masked_coordinate_min2_tiny_seed6551.json`
- Final query rows:
  `runs/ii42-m655-masked-coordinate-v1/m655_masked_coordinate_min2_tiny_seed6551/m655_masked_coordinate_min2_tiny_seed6551_query_rows.jsonl`
- Variant JSONs:
  `runs/ii42-m655-masked-coordinate-v1/m655_masked_coordinate_seed6551/m655_masked_coordinate_seed6551.json`
  `runs/ii42-m655-masked-coordinate-v1/m655_masked_coordinate_min2_seed6551/m655_masked_coordinate_min2_seed6551.json`
