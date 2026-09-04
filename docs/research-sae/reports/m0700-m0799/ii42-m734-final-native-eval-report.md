# M734C Constrained Delta Smoke

M734C applies the fixed M734B selector to the M734A native replay
surface.  If no deployable selector exists, this report is a formal
stop artifact rather than a skipped hidden step.

- Status: `skipped`

## Reason

M734B did not clear the separability gate.  The best-AUC model was `source_atoms_s0.01`/`productive_safe` with AUC=0.806154 but TP=0; the best-TP model was `combined`/`hard_safe` with TP=48 but AUC=0.551469.  No single deployable surface clears both the AUC and actual holdout selection gates.  Do not start deep compiler training from this teacher/interface surface.

## Decision

M734C was skipped because M734B did not find a deployable safe-delta selector.  Stop this M734 interface before deep compiler training.
