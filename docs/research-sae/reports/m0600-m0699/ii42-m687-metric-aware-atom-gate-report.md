# M687 Metric-Aware Atom Gate

## Status

M687 is complete and rejected as a promotable route.

It keeps the M686 atom-delta compiler fixed and tests whether a learned
metric-aware query gate can admit only safe generated-posting deltas. The run
confirms that a safe admission function exists in oracle form, but the current
feature set and thresholding cannot predict it well enough on holdout.

This result supports the current route direction:

- do not return to traditional SAE reconstruction;
- keep dense-root unified posting as the engineering representation;
- keep dense/head support as a hard guard;
- train generated-posting deltas only under retrieval constraints;
- treat admission prediction as the next bottleneck.

## Setup

- Surface: native shared15 PostgreSQL path.
- Queries: `1342`.
- Baseline: P1 native shared15 baseline.
- Strong control: M674 top95-preserving BM25 tail reorder.
- Compiler: M686 `s002_a8` atom delta shape.
- Gate label: apply the atom delta only if native counterfactual metrics pass
  non-regression constraints.

Fixed delta configuration:

- selected atoms: `8`;
- expansion scale: `0.02`;
- shared boost: `1.0`;
- top95 floor: `0.997`;
- NDCG floor: `-0.00005`;
- MRR floor: `-0.00005`.

Artifacts:

- `runs/m687_metric_aware_atom_gate_v1/m687_summary.json`
- `runs/m687_metric_aware_atom_gate_v1/m687_report.md`
- `runs/m687_metric_aware_atom_gate_v1/m687_counterfactual_rows.jsonl`

## Label Diagnostics

| Split | Rows | Positive rows | Positive share |
| --- | ---: | ---: | ---: |
| train | 1098 | 471 | 0.428962 |
| holdout | 244 | 96 | 0.393443 |
| full | 1342 | 567 | 0.422504 |

Gate quality:

| Metric | Value |
| --- | ---: |
| train AUC | 0.598841 |
| holdout AUC | 0.572072 |
| holdout precision | 0.459854 |
| holdout recall | 0.656250 |

The rejection reasons are dominated by head/support risk:

| Reason | Train | Holdout | Full |
| --- | ---: | ---: | ---: |
| top95 drop | 557 | 132 | 689 |
| MAP drop | 133 | 31 | 164 |
| CUB drop | 13 | 4 | 17 |
| NDCG drop | 22 | 5 | 27 |
| Recall drop | 9 | 0 | 9 |
| MRR drop | 5 | 1 | 6 |

The current gate is therefore not limited by label scarcity alone. Roughly 40%
of rows are safe by counterfactual labels, but the learned gate only reaches
holdout AUC `0.572072` and over-applies relative to the oracle.

## Native Results

Full shared15 delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 | 1.000000 | 1.000000 |
| atom-delta all | +0.000302 | -0.000078 | -0.000138 | +0.000024 | -0.000164 | 0.993678 | 1.000000 |
| metric gate | +0.000008 | +0.000002 | +0.000048 | +0.000008 | +0.000011 | 0.996666 | 0.593145 |
| metric oracle | +0.000200 | +0.000078 | +0.000050 | +0.000037 | +0.000024 | 1.000000 | 0.422504 |

Holdout delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.003747 | +0.000190 | +0.000000 | +0.000000 | +0.000000 | 1.000000 | 1.000000 |
| atom-delta all | +0.000956 | -0.000360 | -0.000336 | -0.000137 | -0.000079 | 0.993356 | 1.000000 |
| metric gate | +0.000000 | -0.000063 | -0.000031 | -0.000137 | -0.000007 | 0.996678 | 0.561475 |
| metric oracle | +0.000820 | +0.000075 | +0.000030 | +0.000000 | +0.000000 | 1.000000 | 0.393443 |

## Interpretation

M687 does not validate the learned metric gate. The holdout gate loses MAP,
NDCG, MRR, and CUB, and it fails the configured top95 floor.

The important positive signal is the oracle:

- holdout oracle improves Recall, MAP, and NDCG without hurting MRR/CUB;
- full oracle improves Recall, MAP, NDCG, MRR, and CUB;
- oracle apply rate is about `0.39` on holdout and `0.42` full.

This means the M686 atom-delta shape is not the immediate blocker. A useful
admission boundary exists. The blocker is predicting that boundary from
allowed inference-time features, especially the top95/head-risk component.

This also sharpens the critique of the broader route. The project should not
return to raw SAE reconstruction, because reconstruction/sparsity is still not
aligned with top100 membership. It should also not keep arbitrary direct
posting fine-tuning as the mainline. The useful objective is narrower:

> dense-faithful unified posting plus retrieval-constrained generated-posting
> deltas, admitted only when support/head risk is predictable.

## Decision

Reject M687 as a promotion candidate.

Keep it as a diagnostic milestone. It supplies the per-query counterfactual
surface needed for the next gate and proves that safe atom-delta admission has
oracle room.

## Next Step

Proceed to M688 only if it improves admission prediction, not if it merely
enlarges the compiler.

Recommended M688:

1. Reuse `m687_counterfactual_rows.jsonl` as the training surface.
2. Add explicit head-risk features:
   - top95 displacement margin;
   - top100 boundary score margin;
   - boosted atom fanout and IDF risk;
   - overlap between added atoms and protected top95 docs;
   - per-query baseline MAP/Recall slack.
3. Use a precision-biased admission rule rather than the current over-applying
   threshold.
4. Preserve the M686 atom-delta shape until gate prediction is solved.
5. Promotion requires holdout Recall/MAP/CUB non-regression, no material
   NDCG/MRR drop, top95 above floor, and full-surface direction match.

If M688 cannot predict the safe rows better than M687, the next blocker is
atom visibility and proposal-pool coverage, not loss depth or model size.
