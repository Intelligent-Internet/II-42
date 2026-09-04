# M748 Native-Context Shared8 Validation

## Purpose

M748 validates whether the M745 native-context accepted-region signal survives
on a broader shared8 surface.

The key change versus M741-M743 is the feature interface:

- M741/M742/M743 used coordinate identity, scale, and simple coordinate
  statistics.
- M745/M747 add native candidate-context features from replay attempts:
  threshold deltas, margin deltas, candidate overlap, and score distribution
  movement around the rank boundary.

## Shared4 Anchor

M745 shared4 held-out boundary result:

| Feature set | Applied | Accepted hits | Best-row hits | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| context | 25 | 12 | 2 | +0.001306 | +0.000940 | +0.000000 | +0.000222 | +0.000000 | 1 |

## Shared8 Validation

M747 shared8 held-out boundary result:

| Feature set | Applied | Accepted hits | Best-row hits | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| context | 26 | 12 | 1 | +0.000000 | +0.001046 | +0.000769 | +0.001020 | +0.000612 | +0.000000 | 1 |
| context_group | 46 | 22 | 0 | +0.000000 | +0.000720 | +0.000560 | +0.000000 | +0.000229 | +0.000000 | 1 |
| context_teacher | 39 | 21 | 0 | +0.000000 | +0.001009 | +0.000420 | +0.000000 | +0.000517 | +0.000000 | 1 |
| context_teacher_group | 17 | 8 | 0 | +0.000000 | +0.000763 | +0.000420 | +0.000000 | +0.000492 | +0.000000 | 1 |

M654 oracle on the same shared8 held-out boundary slice:

| dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| +0.000000 | +0.001603 | +0.003861 | +0.010204 | +0.000360 | +0.000000 |

## Interpretation

The route is now positive enough to continue.

What changed:

- Coordinate-only selection failed even with high row-level AUC.
- Native-context selection passes strict held-out gate on shared4 and shared8.
- The deployable `context` feature set does not require M654 teacher
  coordinate features and still improves MAP/NDCG/MRR/CUB without dense
  overlap loss.

What did not change:

- Recall@100 did not move on shared8.
- The selector is still replay-based.
- This is still boundary-slice validation, not full native matrix validation.

## Decision

Promote the route from "oracle only" to "candidate route".

Next required gate:

1. Run shared15/native context export and selector.
2. Keep `context` as the primary deployable feature set.
3. Treat `context_teacher*` as diagnostic only.
4. Accept only if shared15 has positive MAP or NDCG with no O@100, CUB, MRR,
   or Recall regression.
5. After shared15, measure replay cost and decide whether to compile the
   selector into a cheaper query-time policy.

Stop condition:

If shared15 loses the strict gate or the gain collapses to near zero, do not
promote to P1.  Keep M745/M747 as evidence that the missing interface was
native boundary context, but not enough for deployment.
