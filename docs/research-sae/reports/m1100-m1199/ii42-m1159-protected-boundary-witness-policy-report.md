# M1159 Protected-Boundary Witness Policy

## Objective

M1158 showed that aggregate slot-trace features do not improve the clean
frontier.  M1159 narrows the hypothesis: maybe the useful signal is not the
raw trace itself, but a structured protected-boundary label.

The training labels use qrels only for supervision/audit:

- `no_relevant_exit`: the atom action does not remove a qrel-positive document
  from top100.
- `has_relevant_entrant`: the atom action admits a qrel-positive document into
  top100.
- `relevant_balance_positive`: entrant relevance exceeds exit relevance.

The deployed policy features remain inference-time available: proposal atom
features, proxy score shape, and optional qrels-free witness stats.

## Artifacts

- `runs/m1159_protected_boundary_witness_policy_v1/protected_boundary_witness_rows.json`
- `runs/m1159_protected_boundary_witness_policy_v1/protected_boundary_witness_policy.json`
- `runs/m1159_protected_boundary_witness_policy_v1/summary.md`

## Label Surface

- query_count: 149
- atom_count: 2,370
- safe_gain_rate: 0.146414
- no_relevant_exit_rate: 0.961603
- has_relevant_entrant_rate: 0.043460
- relevant_balance_positive_rate: 0.029536

This confirms why the problem is hard: most actions do not eject a qrel
positive, but only a small fraction also admit useful positives.

## Main Result

M1159 improves the best row-floor-clean non-oracle policy over M1157.

| Policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | Row floor |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| M1157 `proposal_proxy_guarded_g0.50_s0.50_r0.70` | +0.000051 | +0.000551 | +0.000370 | +0.001220 | +0.003356 | clean |
| M1159 `proposal_proxy_guarded_g0.50_s0.50_r0.70_e0.50` | +0.000051 | +0.000551 | +0.000403 | +0.001546 | +0.003356 | clean |

The improvement is small but clean:

- MAP improves by +0.000033 over M1157.
- NDCG improves by +0.000326 over M1157.
- Recall/MRR/CUB are preserved.
- Per-dataset floors remain clean.

The best clean action mix:

```text
a1_s0.01:3, a1_s0.05:5, a2_s0.01:2, a2_s0.05:4,
a3_s0.01:12, a3_s0.05:18, a4_s0.01:8, a4_s0.05:13,
a5_s0.05:4, a6_s0.01:1, a6_s0.05:2, a7_s0.05:1,
base:76
```

Oracle remains far above:

| Policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | Row floor |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `oracle_single` | +0.001118 | +0.019178 | +0.005896 | +0.008171 | +0.005705 | clean |

## What Worked

The useful part is the protected-boundary supervision, not raw witness
features.

The winning M1159 policy is still a `proposal_proxy` feature policy, but it
adds the `no_relevant_exit` guard head (`e0.50`).  This suggests the qrels-based
protected-exit target can regularize the atom selector without requiring qrels
at inference.

## What Did Not Work

Feature families that directly include raw witness features did not become
row-floor clean:

| Feature family | Best clean non-base |
| --- | --- |
| `witness` | none |
| `proxy_witness` | none |
| `proposal_proxy_witness` | none |

The raw witness features can increase macro utility, but they still introduce
CUB, MAP, NDCG, or MRR row regressions.  This repeats the M1158 lesson: simply
adding larger trace vectors is not the right direction.

## Interpretation

M1159 gives a small but important signal:

1. The single-atom admission branch is not exhausted yet.
2. The next improvement should be multi-task label design, not more raw feature
   stuffing.
3. `no_relevant_exit` is a useful auxiliary safety target.
4. `has_relevant_entrant` is sparse and probably needs a different teacher or
   broader data before it can recover a large share of the oracle.

## Decision

Keep M1159 as the new conservative baseline for this branch.

Next step should be M1160:

- train a calibrated multi-task atom selector with heads for safe-gain,
  no-relevant-exit, rank-safe, and possibly relevant-entrant;
- keep features to the compact proposal/proxy family first;
- test whether the M1159 clean improvement survives stricter LODO variants or
  a larger native surface;
- do not expand raw witness trace dimensions unless the compact multi-task
  head stops improving.
