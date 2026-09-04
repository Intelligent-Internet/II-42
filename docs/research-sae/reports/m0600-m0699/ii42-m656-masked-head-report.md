# M656 / Retrieval-Constrained Masked Head Report

Status: `masked_head_failed`

M656 is the first learned follow-up to M654/M655.  It trains a
query-conditioned output head constrained to the coordinate family discovered
by the feasibility audits.  There is no BM25 signal and no qrels-time
acceptance gate during evaluation.

## Question

M654 showed that coordinate-level safe movement exists.  M655 showed that a
global fixed mask cannot turn that movement into Recall crossing.  M656 tests
whether a learned per-query masked head can activate the same coordinate family
well enough to cross boundary positives into top100.

## Runs

| Run | Key config | Status | all dO@100 | all dNDCG@10 | all dMAP@100 | all dR@100 | boundary dO@100 | boundary dR@100 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m656_masked_head_seed6561` | `MAX_DELTA=0.03` | failed | -0.033867 | +0.001538 | +0.002484 | -0.003683 | -0.043000 | +0.001905 |
| `m656_masked_head_safe_seed6561` | `MAX_DELTA=0.005`, strong preservation | failed | -0.004258 | +0.000539 | +0.000597 | +0.000000 | -0.006000 | +0.000000 |

## Main Result

The aggressive model learned boundary crossing, but only by damaging support
geometry:

```json
{
  "all": {
    "dense_overlap_at_100": -0.03386718750000006,
    "map_at_100": 0.00248447700592197,
    "mrr_at_20": 0.003348216685248706,
    "ndcg_at_10": 0.0015380306113060849,
    "recall_at_100": -0.003683035714285743
  },
  "boundary": {
    "dense_overlap_at_100": -0.04299999999999993,
    "map_at_100": -0.0029693569840989753,
    "mrr_at_20": 0.0,
    "ndcg_at_10": -0.0012380990409499582,
    "recall_at_100": 0.001904761904761909
  }
}
```

The strong-preservation model reduced damage, but test Recall crossing
disappeared:

```json
{
  "all": {
    "dense_overlap_at_100": -0.004258,
    "map_at_100": 0.000597,
    "mrr_at_20": 0.000391,
    "ndcg_at_10": 0.000539,
    "recall_at_100": 0.0
  },
  "boundary": {
    "dense_overlap_at_100": -0.006000,
    "map_at_100": -0.000004,
    "mrr_at_20": 0.0,
    "ndcg_at_10": -0.000151,
    "recall_at_100": 0.0
  }
}
```

## What Carries Forward

M656 is not a deployment candidate, but it is not a dead result.

1. It proves a learned query-conditioned masked head can create boundary
   Recall movement.  M655 fixed masks could not do this.
2. It also proves the current preservation loss is insufficient.  The model
   crosses by disturbing top100 support too much.
3. Stronger scalar preservation reduces damage but removes held-out crossing.
   The solution is not simply smaller deltas or larger MSE weights.

## Core Block

The current loss preserves protected scores, but it does not explicitly
preserve protected ranks against outside intruders.  A document can keep its
absolute score while another non-protected document rises above it, causing
dense-overlap and top100 support loss.

This matches the data:

- Aggressive M656 improves boundary Recall but loses `0.043` boundary O@100.
- Safe M656 reduces all O@100 loss to `0.0043`, but loses test Recall gain.
- Both variants still fail overlap guards.

## Next Breakthrough Point

M657 should keep the same masked-head structure but change the preservation
objective:

1. Add a support-rank barrier: protected top100 docs must stay above outside
   risk docs by a margin.
2. Use outside-risk docs from M654/M655 coordinate analysis as explicit
   negatives, not just replaceable tail docs.
3. Keep boundary crossing loss, but only accept movement that preserves
   protected-vs-intruder ordering.
4. Track crossing, overlap, and intruder rate separately in the conclusion
   report.

Acceptance for M657 should require boundary Recall@100 improvement with
all-query and boundary dense overlap no worse than `-0.001`.  If that fails,
the current coordinate-mask family should stop and the next route should return
to first-stage output-head design rather than deeper tuning of this mask.

## Artifacts

- Aggressive JSON:
  `runs/ii42-m656-masked-head-v1/m656_masked_head_seed6561/m656_masked_head_seed6561.json`
- Safe JSON:
  `runs/ii42-m656-masked-head-v1/m656_masked_head_safe_seed6561/m656_masked_head_safe_seed6561.json`
- Aggressive checkpoint:
  `runs/ii42-m656-masked-head-v1/m656_masked_head_seed6561/m656_masked_head_seed6561.masked_head.pt`
- Safe checkpoint:
  `runs/ii42-m656-masked-head-v1/m656_masked_head_safe_seed6561/m656_masked_head_safe_seed6561.masked_head.pt`
