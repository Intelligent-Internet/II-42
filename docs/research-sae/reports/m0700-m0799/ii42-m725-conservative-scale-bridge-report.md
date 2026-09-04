# M725 Conservative Scale Bridge

## Question

M724 showed that M721b pair-interaction movement can change P1.3 top-100
boundary membership, but the M722 shared15 bridge lost small amounts of
NDCG/MAP/MRR.  M725 tests whether this is simply a global movement-scale
problem before spending more training budget.

This is still a first-stage dense-equivalence probe:

- frozen baseline: `P1.3 / M549U native signed-dot`
- no BM25
- no reranker
- no learned gate
- no qrels-driven training or selection
- frozen native P1.3 document posting surface

## Inputs

| Scale | Output root |
| ---: | --- |
| 0.020 | `runs/m722_p1p3_pair_interaction_retrieval_bridge_shared15_v1` |
| 0.010 | `runs/m725_p1p3_pair_interaction_retrieval_bridge_scale001_v1` |
| 0.005 | `runs/m725_p1p3_pair_interaction_retrieval_bridge_scale0005_v1` |

Each run evaluates all shared15 datasets through the same native P1.3 posting
surface and compares the moved query atoms against the frozen P1.3 baseline.

## Macro Scale Curve

| Scale | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 | dO@10 | dO@50 | dO@100 | dO@256 | dCUB | dSupport cos |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.020 | -0.000221 | -0.000422 | +0.000191 | -0.000723 | +0.000245 | -0.000129 | +0.005918 | +0.000206 | +0.000054 | -0.000044 |
| 0.010 | -0.000158 | -0.000356 | -0.000119 | -0.000702 | +0.000267 | -0.000072 | +0.003249 | +0.000173 | +0.000003 | -0.000011 |
| 0.005 | +0.000055 | -0.000079 | +0.000007 | -0.000336 | +0.000067 | +0.000000 | +0.001729 | +0.000123 | +0.000005 | -0.000003 |

## Gate Result

| Scale | Dense-equivalence guard | Promotion gate | Reason |
| ---: | --- | --- | --- |
| 0.020 | Fail / borderline | Fail | O@100 and Recall improve, but MAP/MRR/NDCG regress and support cosine drops. |
| 0.010 | Fail | Fail | O@100 still improves, but Recall becomes negative and rank metrics still regress. |
| 0.005 | Pass | Fail | O@100/CUB/Recall/support are preserved, but MAP and MRR still regress. |

## Interpretation

M725 does not support blind longer training on the current global movement
shape.  Lowering the scale monotonically reduces damage, but it also reduces
the useful boundary gain, and even at `scale=0.005` the moved representation
does not clear the promotion gate.

This means the main issue is not just under-training.  The current movement can
cross some useful dense top-100 boundaries, but it does not know which
movements are rank-safe.  A single global scale cannot separate safe movement
from top-rank disturbance.

The line is therefore useful as a diagnostic, not yet as a deployable
first-stage compiler.

## Decision

Do not promote M721b/M725 as a new P1 first-stage candidate.

Do not scale this exact global movement training further until selection is
changed.  More epochs would likely keep improving local pair success while
still spending ranking geometry, because the current objective does not bind
top-rank preservation tightly enough.

Retain these artifacts because they isolate the failure mode:

- pair-level movement is real
- dense overlap@100 can be improved without changing doc postings
- safe movement requires query/movement confidence, not only global scale

## Next Probe

The next useful first-stage probe should be M726:

1. Use M724/M725 evidence to build a confidence-gated movement replay.
2. Gate movement using only dense-equivalence signals, not qrels:
   - top-rank margin safety
   - support cosine delta
   - dense top-10/top-50 stability
   - candidate movement magnitude
   - pair model confidence
3. Accept movement only when it preserves:
   - dense overlap@10/@50/@100
   - support cosine floor
   - candidate upper bound
   - Recall@100
4. Reject the line if confidence gating either removes nearly all O@100 gain or
   still regresses MAP/MRR.

If M726 fails, the pair-interaction route should be stopped as a first-stage
optimization route and kept only as evidence for designing a direct
dense-topK/rank-margin preserving compiler.
