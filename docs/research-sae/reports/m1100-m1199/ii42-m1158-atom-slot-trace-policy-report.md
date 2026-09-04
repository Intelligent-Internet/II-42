# M1158 Atom Slot Trace Policy

## Objective

M1157 produced a row-floor-clean deployed atom policy, but it recovered only a
small part of the M1155 single-atom oracle.  The hypothesis for M1158 was that
the remaining gap is candidate-slot local: a candidate atom may look safe at
query level but still replace the wrong top100 boundary document.

M1158 tests this directly by adding qrels-free base/moved native rank-trace
features.

## Method

For each M1155 marginal atom action, M1158 replays:

- base native ranking
- moved native ranking after adding one atom/scale delta

It then extracts inference-time trace features:

- top10/top50/top100/top200 overlap
- top100 entrants and exits
- entrant/exit old and new ranks
- entrant/exit fused/P1/BM25/source score statistics
- rank 95/100/101 boundary margins
- base vs moved score shifts

These features are joined with M1157 proposal/proxy features and tested with
LODO policies.

Artifacts:

- `runs/m1158_atom_slot_trace_policy_v1/slot_trace_rows.json`
- `runs/m1158_atom_slot_trace_policy_v1/atom_slot_trace_policy.json`
- `runs/m1158_atom_slot_trace_policy_v1/summary.md`

## Results

Surface:

- query_count: 149
- atom_count: 2,370

The oracle is unchanged:

| Policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | Row floor |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `oracle_single` | +0.001118 | +0.019178 | +0.005896 | +0.008171 | +0.005705 | clean |

Best clean deployed policy remains the M1157 proposal/proxy policy:

| Policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | Row floor |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `proposal_proxy_guarded_g0.50_s0.50_r0.70` | +0.000051 | +0.000551 | +0.000370 | +0.001220 | +0.003356 | clean |

Slot-trace feature families did not improve the clean frontier:

| Feature family | Best score behavior | Clean non-base policy |
| --- | --- | --- |
| `slot_trace` | negative macro utility | none |
| `proxy_trace` | negative macro utility | none |
| `proposal_proxy_trace` | higher Recall but CUB/NDCG losses | none |
| `proposal_proxy_native_trace` | near flat/negative | none |

Example failure:

| Policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | Row floor |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `proposal_proxy_trace_guarded_g0.30_s0.50_r0.50` | -0.000516 | +0.002043 | +0.000470 | -0.000288 | +0.001902 | fail |

## Interpretation

M1158 is a useful negative result.

The base/moved rank trace is not the missing deployable signal in its current
form.  It can identify larger movements, but those movements spend CUB or
NDCG/MAP floor.  This means the trace is too coarse: seeing that an atom moves
top100 membership is not enough to know whether the displaced boundary document
is protected or whether the entrant is useful.

This also prevents a micro-tuning loop:

- adding more query-level native features failed in M1157 high-gain policies
- adding full slot-trace features fails here
- the clean policy remains the simpler proposal/proxy atom gate

So M1158 should not be scaled as-is.

## Decision

Keep M1157 as the current conservative atom-policy baseline.

Stop the current M1158 trace-feature direction.  The next useful step should be
narrower and more label-structured:

1. Build protected-boundary witness labels: identify which top100 exits are
   protected positives or high-confidence dense/BM25 anchors.
2. Train a pair/witness classifier over atom-to-boundary-slot replacement,
   rather than a query-level action classifier with aggregate trace stats.
3. Replay only replacements that pass protected-boundary risk, then compare
   against the M1157 clean policy.

This is closer to the M650 lesson: pair-local replacement risk is the object to
learn, not a broader query/action score.
