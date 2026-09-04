# M1232 Harm-Separated Proposal Source

## Purpose

M1231 showed that adding directional movement features to the existing
retrieval-conditioned candidate rows does not improve target/harm separation.
M1232 tested whether the proposal pool itself can be made safer before any
training or native replay.

The audit deliberately keeps the CUB-specific target/harm label universe fixed
to the base train-fold allowed atoms. Strict proposal filters therefore cannot
look better merely by removing target or harm atoms from the denominator.

## Run

- Smoke surface: `cqadupstack`, `scidocs`, `webis-touche2020`
- Output JSON: `runs/m1232_harm_separated_proposal_source_smoke_v1/m1232_harm_separated_proposal_source.json`
- Output Markdown: `runs/m1232_harm_separated_proposal_source_smoke_v1/m1232_harm_separated_proposal_source.md`

## Key Result

| Variant | TargetRecall | Precision | HarmPrecision | Gap |
| --- | ---: | ---: | ---: | ---: |
| base model top8 | 0.976959 | 0.184991 | 0.006108 | 0.178883 |
| ratio3 model top8 | 0.940092 | 0.183288 | 0.006289 | 0.176999 |
| zero_harm model top8 | 0.912442 | 0.189837 | 0.006711 | 0.183126 |
| zero_harm_pos3 model top8 | 0.442396 | 0.179104 | 0.005597 | 0.173507 |

The strict filters reduce or slightly reshape harm exposure, but they do not
create a useful target/harm separation improvement. The best gap gain
(`zero_harm`) is only about `+0.0042` while losing about `-0.0645` target
recall. The stricter `zero_harm_pos3` source collapses target coverage.

## Decision

Stop this proposal-source shape at smoke. Do not run full shared15 and do not
run native replay from M1232.

Reason:

- The branch was required to improve proposal separability before replay.
- The smoke result shows coverage loss dominates harm reduction.
- This is not a training-depth problem because the proposal pool itself fails
  before any selector or generated-posting policy is trained.

## Implication

The reusable conclusion is now sharper:

- M1228 showed native rank context makes query-level safe/harm visibility real.
- M1230 showed static atom-doc context does not transfer that signal to atom
  candidates.
- M1231 showed directional atom-boundary movement also does not transfer it.
- M1232 showed simple train-fold harm-separated proposal filtering loses too
  much target coverage.

The next branch should not be another threshold on the same atom source. It
must change the proposal interface so target visibility and harm separation are
created together.

## Next Candidate Direction

The next useful experiment is to move from atom filtering to event/source
construction:

1. Build query-local proposal events from native row context first.
2. Attach atoms to events only when they explain the same boundary movement.
3. Audit event-level target/harm separability before atom selection.
4. Only if event-level separation is positive, derive atom deltas from selected
   events and test bounded replay.

Stop condition: if event/source construction cannot improve target coverage
and precision-minus-harm gap simultaneously on smoke, do not train and do not
run replay.
