# ii42 M1124 Listwise Atom Utility Smoke Plan

## Rationale

M1121-M1123 showed that fixed alpha and deployable alpha gating are not the
right next step. The failure is upstream: atom pressure produces useful
evidence, but it is not calibrated against top-rank preservation.

M1124 adds a small listwise batch loss to the existing pairwise objective. For
each query batch, the model must select its own positive from positives,
hard negatives, and background documents in the same batch. This directly
penalizes high-scoring non-positive documents, which is the observed
`pure_top_damage` failure mode.

## Smoke Configuration

- datasets: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`
- root: spark-1 shared15 root
- semantic hard negatives: disabled
- rank epochs: `4`
- learning rate: `0.0007`
- listwise rank weight: `0.25`
- listwise temperature: `0.07`
- export rankings: top `1000`

## Acceptance

Promote only if replay shows:

1. heldout MAP/NDCG/MRR improve over M1121 at comparable Recall, or
2. fixed alpha 0.25/0.30 improves over M1118 top-rank without large Recall
   loss, and
3. M1122 query audit shows fewer pure top-damage transitions.

Reject if train-selected alpha still overfits and heldout best is not better
than M1118/M1121 on top-rank metrics.
