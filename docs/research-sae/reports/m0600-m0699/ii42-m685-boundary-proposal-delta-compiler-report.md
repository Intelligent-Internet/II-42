# M685 Boundary Proposal Delta Compiler

## Status

M685 is complete and should not be promoted.

It directly tests the M684 diagnosis. M684 failed because its query gate had
signal but its BM25-tail proposal documents did not reproduce the M683 oracle
delta. M685 therefore changes the proposal interface:

- wider proposal pool from native fused tail, P1 tail, and BM25 tail;
- candidate features relative to the top100 boundary;
- weighted atom aggregation from selected proposal documents;
- same train-only F0.5 query gate from M683 oracle labels;
- same native shared15 PostgreSQL evaluation path.

This is still a negative result. It improves proposal visibility and learns a
candidate boundary model, but raw selected-document atoms still do not form a
safe generated-posting delta.

## Default Run

Artifacts:

- `runs/m685_boundary_proposal_delta_compiler_v1/m685_summary.json`
- `runs/m685_boundary_proposal_delta_compiler_v1/m685_report.md`

Proposal stats:

| Item | Value |
| --- | ---: |
| Candidate rows | 26,286 |
| Positive rows | 529 |
| Positive share | 0.020125 |
| Visible target ids | 529 / 752 |
| Proposal train AUC | 0.800041 |

Gate stats:

| Split | Rows | Positives | AUC | Precision | Recall |
| --- | ---: | ---: | ---: | ---: | ---: |
| train | 1098 | 168 | 0.813018 | 0.647059 | 0.327381 |
| holdout | 244 | 41 | 0.781569 | 0.421053 | 0.195122 |

Full shared15 delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 | 1.000000 | 1.000000 |
| bm25_pool_all | +0.000073 | -0.000921 | -0.000300 | -0.001156 | +0.000163 | 0.978681 | 1.000000 |
| proposal_all | -0.000165 | -0.000031 | -0.000024 | -0.000960 | +0.000659 | 0.967754 | 1.000000 |
| proposal_gate | +0.000165 | +0.000042 | -0.000130 | -0.000408 | -0.000118 | 0.997804 | 0.077496 |

Holdout delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.003747 | +0.000190 | +0.000000 | +0.000000 | +0.000000 | 1.000000 | 1.000000 |
| bm25_pool_all | +0.000192 | -0.002704 | -0.002055 | -0.004674 | -0.000218 | 0.978085 | 1.000000 |
| proposal_all | -0.000064 | -0.001310 | -0.001317 | -0.005869 | +0.002219 | 0.967299 | 1.000000 |
| proposal_gate | -0.000119 | -0.000172 | -0.000229 | -0.002049 | -0.000003 | 0.997368 | 0.077869 |

## Conservative Variant

To test whether the default failed only because the delta was too strong, M685
also ran a conservative variant:

- selected count: `2`
- expansion scale: `0.03`
- shared boost: `1.0`

Artifacts:

- `runs/m685_boundary_proposal_delta_compiler_s003_c2_v1/m685_summary.json`
- `runs/m685_boundary_proposal_delta_compiler_s003_c2_v1/m685_report.md`

Full shared15 delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| proposal_all | +0.000172 | +0.000002 | -0.000232 | -0.000039 | +0.000028 | 0.989819 | 1.000000 |
| proposal_gate | +0.000014 | +0.000022 | +0.000006 | +0.000000 | -0.000013 | 0.999302 | 0.077496 |

Holdout delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| proposal_all | -0.000161 | -0.000283 | -0.000227 | -0.000011 | +0.000052 | 0.989474 | 1.000000 |
| proposal_gate | -0.000161 | -0.000040 | -0.000036 | +0.000000 | +0.000008 | 0.999051 | 0.077869 |

The conservative variant improves head preservation and reduces damage, but it
still fails holdout Recall/MAP. Therefore the failure is not explained by delta
magnitude alone.

## Interpretation

M685 gives a sharper answer than M684.

Positive signals:

1. The wider proposal pool exposes many more teacher targets than the M684
   BM25-tail selector. M685 sees `529 / 752` balanced teacher targets.
2. The proposal model has non-trivial train AUC `0.800041`.
3. The query gate remains learnable, with holdout AUC `0.781569`.
4. Ungated proposal expansion can increase candidate upper bound, which means
   the proposal surface can move candidate membership.

Negative signals:

1. The default gated result loses holdout Recall, MAP, NDCG, MRR, and a tiny
   amount of CUB.
2. The ungated result increases holdout CUB but hurts ranking and Recall.
3. The conservative variant preserves top95 better but still loses holdout
   Recall/MAP.
4. M674 remains substantially stronger and cleaner.

This means the route has moved past a simple candidate-document selection
problem. We can find plausible candidates, but copying or averaging their raw
doc atoms is not the right delta basis.

## Relation To The Current Critique

The critique remains directionally correct:

- do not return to traditional SAE reconstruction;
- keep dense-root unified posting as the engineering form;
- add retrieval-constrained generated postings only under dense/support floors.

M685 narrows the required change:

> Retrieval-constrained generated posting cannot be implemented as selected
> candidate-doc atom copying. The next compiler must learn atom-level deltas
> or a replacement posting basis directly.

In other words, the next target is not "which tail document should we copy
from?" but "which atoms should the query compiler emit to approximate the safe
M683 oracle delta?"

## Decision

Reject M685 as a promotable route.

Keep M685 as a diagnostic result. It proves that broader candidate visibility
and boundary-relative features are not sufficient when the delta basis remains
raw selected-document atoms.

## Next Breakthrough Point

Proceed only if M686 changes the output target from document selection to
atom-level delta prediction.

Recommended M686:

1. Build an atom-delta teacher from M683 accepted oracle rows:
   compare accepted generated queries against baseline query atoms and extract
   added/boosted atoms as labels.
2. Train a global atom-level proposal model using features available at
   inference:
   query atom shape, candidate pool aggregate atom votes, atom frequency,
   signed support, boundary displacement, BM25/P1/fused visibility, and
   same-sign/opposite-sign evidence.
3. Generate a sparse delta directly from top atom predictions, not from top
   selected documents.
4. Keep native shared15 as the gate:
   Recall/MAP must improve on holdout and full surface, CUB must not regress,
   NDCG/MRR cannot materially drop, and top95 must stay above floor.
5. Stop if atom-level prediction cannot beat M674 or cannot produce positive
   holdout Recall/MAP while preserving CUB.

This is still the P1 main line: dense-root unified posting plus support-safe
retrieval expansion. M685 shows the next implementation must be an atom-level
posting compiler, not another scorer, gate, or document-copy proposal.
