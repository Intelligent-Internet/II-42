# M1233 Event Source Observability

## Purpose

M1230-M1232 showed that the current atom-candidate interface cannot safely
separate target atoms from harm atoms. M1233 moved one level up and asked
whether query-local action events (`high`, `mid`, `low`) are separable before
attaching atoms or running native replay.

## Important Correction

The first full run appeared positive because `native_action` outperformed
`event_delta_only`. That was not enough evidence: native rank-context features
are identical for the three actions of the same query, so a model can look
positive by learning a fixed action prior.

The script was corrected to add an `action_only` baseline. After this
correction, the apparent native-action gain disappears.

## Corrected Smoke Run

- Surface: `cqadupstack`, `scidocs`, `webis-touche2020`
- Output JSON: `runs/m1233_event_source_observability_smoke_v2/m1233_event_source_observability.json`
- Output Markdown: `runs/m1233_event_source_observability_smoke_v2/m1233_event_source_observability.md`

| Variant | TargetRecall | Precision | HarmPrecision | Gap |
| --- | ---: | ---: | ---: | ---: |
| action_only top1 | 0.678261 | 0.313253 | 0.008032 | 0.305221 |
| native_action top1 | 0.678261 | 0.313253 | 0.008032 | 0.305221 |
| event_delta_only top1 | 0.426087 | 0.196787 | 0.012048 | 0.184739 |
| combined top1 | 0.434783 | 0.200803 | 0.012048 | 0.188755 |

`native_action` is exactly equal to `action_only`. The event-delta and combined
feature groups are worse than the fixed action prior.

## Decision

Stop M1233 at corrected smoke. Do not run full shared15 v2, do not attach atoms
to events, and do not run native replay from this event shape.

Reason:

- The only apparent positive signal was explained by action prior leakage.
- Event-delta features reduce target coverage and worsen harm separation.
- Native query context remains useful at query-level guard scale, but this
  event construction does not expose an actionable event-level selector.

## Implication

The recent sequence now has a clearer conclusion:

- M1228: native query-level rank context is real.
- M1230: static atom-doc context does not transfer it to atom candidates.
- M1231: directional atom-boundary movement does not transfer it.
- M1232: simple harm-separated atom proposal filtering loses target coverage.
- M1233: action-event selection collapses to a fixed action prior.

The deployable bottleneck is still the proposal/interface, not classifier
depth. The next step should not be another threshold, atom filter, or action
selector on these same sources.

## Next Direction

The next useful branch must create a new observable source, not re-rank the old
one. A reasonable next audit is a boundary-document witness source:

1. Identify query-local boundary documents whose movement would improve recall
   or CUB in the teacher surface.
2. Extract atom witnesses from those documents, not from action delta rows.
3. Compare witness atoms against harm-boundary atoms before training.
4. Only if witness atoms improve target coverage and precision-minus-harm gap,
   test bounded replay.

Stop condition: if boundary-document witnesses cannot beat the current source
model on target recall and gap at smoke, stop before training.
