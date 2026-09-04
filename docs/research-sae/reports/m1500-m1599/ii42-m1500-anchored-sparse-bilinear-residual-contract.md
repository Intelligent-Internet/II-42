# M1500 Anchored Sparse Bilinear Residual Contract

Date: 2026-07-09
Status: teacher audit in progress

## Decision

M1500 does not continue query gates, raw-token MaxSim, source routers, or
post-hoc rerankers. It tests whether the useful joint movement observed in the
M1182/M1225/M1251 lineage can be represented by a compact sparse bilinear
residual on top of the canonical M549U/PPLX-1024 native score.

The only model anchor is the frozen M549U compiler on the official PPLX
sentence root. Historical M1129/M1137 and P1.3/Snowflake artifacts are evidence
sources, not interchangeable scoring surfaces.

## Score Contract

```text
penalty(q, d) = sparse_zq(q) dot sparse_zd(d)
score(q, d) = frozen_M549U_score(q, d) - lambda * penalty(q, d)
pair_gain(q, p, n) = penalty(q, n) - penalty(q, p)
```

The residual dimensions use signed values. A positive penalty lowers a
document score, while a negative penalty boosts it. This preserves one native
UnifiedPosting score path and does not introduce a separate reranker.

## Teacher Reconstruction

M1225 and M1251 do not persist q-doc pair labels. Treating their summary JSON
as a pair teacher would be incorrect. M1500 reconstructs the supervision as
follows:

1. M1182 supplies `(query, relevant document, less-relevant document)` pair
   proposals and the historical `rank_teacher_positive` versus `harm_penalty`
   role.
2. Every pair score is recomputed through the canonical M549U native atom
   scorer. Historical raw/joint score magnitudes are recorded only for audit
   and are never regression targets.
3. M1224 CUB action evidence is rebuilt from the native PPLX product surface
   and attached at query level. This establishes whether the old pair proposal
   is supported by the retained CUB-specific/action-signed signal.
4. M1225 scale-1 and M1251 signed-sum results remain external native outcome
   evidence. They do not supply per-pair values.

## Pair Objectives

`target_gain` rows come from M1182 `rank_teacher_positive` pairs. Their
objective requests positive residual pair gain.

`harm_nondegrade` rows come from M1182 `harm_penalty` pairs. Their objective is
a hard non-negative pair-gain constraint plus a zero residual-energy prior.
They are not inverted into a second positive teacher. This avoids teaching the
model to copy a movement already known to damage qrel ordering.

Pair weights are normalized inside `(dataset, query, role)` groups. This keeps
large pair enumerations from a few queries from dominating the capacity test.

## M1500 Gate

M1501 may start only when all conditions hold:

- query and document lineage is `m549u_active_locked`, 1024 dimensions, with
  `m549u_signed_coordinate_v1` query namespace;
- pair, query, and document coverage are each at least 95%;
- both target and harm roles are present;
- at least one target pair is still misordered by the frozen PPLX anchor;
- rebuilt CUB/action evidence covers at least 90% of retained pair rows.

Failure means the old movement cannot be promoted into a PPLX teacher. It does
not justify another selector or loss sweep.

## M1501 Capacity Test

M1501 receives only the audited pair file. It first constructs a sparse
query-document penalty field:

- target pairs contribute positive pressure to `penalty(negative) -
  penalty(positive)`;
- harm pairs impose non-degradation and low-energy constraints;
- query-only and document-only controls use the same pair objective;
- dense low-rank and TopK sparse bilinear factors are compared at ranks
  8/16/32/64 and active counts 4/8/16.

No text encoder is trained in M1501. The route stops unless rank at most 32 and
active count at most 16 retain 75% of the free-factor oracle gain while meeting
the macro, touch, and touched-document gates from the goal.

## Stop Rules

1. Do not continue when lineage or coverage fails.
2. Do not fit old Snowflake score magnitudes on PPLX.
3. Do not use a query gate, dataset threshold, BM25 inference feature, or
   post-hoc reranker to pass M1501.
4. Do not train text heads before free-factor capacity passes.
5. A repeated failure at the same gate closes the route rather than spawning a
   parameter sweep.
