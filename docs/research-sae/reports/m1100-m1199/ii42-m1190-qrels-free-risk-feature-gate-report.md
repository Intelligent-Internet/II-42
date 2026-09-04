# M1190 Qrels-Free Risk Feature Gate

## Purpose

M1189 showed that a single top-k overlap gate is too shallow.  M1190 tests a
richer qrels-free gate using query-time native features:

- baseline/aggressive/conservative top-k overlaps
- changed top100 entrant statistics
- proposal document BM25/P1/fused ranks and scores
- source-count and dual-source shares
- query atom and perturbation atom statistics

Labels use qrels only during training.  Evaluation is leave-one-dataset-out:
train on 14 datasets, choose threshold on the train fold, evaluate on the
held-out dataset.

## Artifacts

- Script: `scripts/audit_m1190_qrels_free_risk_feature_gate.py`
- Full JSON:
  `runs/m1190_qrels_free_risk_feature_gate_v1/m1190_qrels_free_risk_feature_gate.json`
- Full Markdown:
  `runs/m1190_qrels_free_risk_feature_gate_v1/m1190_qrels_free_risk_feature_gate.md`
- Smoke:
  `runs/m1190_qrels_free_risk_feature_gate_smoke_v1/`

## Full Shared15 Macro

| Variant | Take | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_best3` | 0.221 | +0.003013 | +0.006581 | +0.006700 | +0.005578 | +0.000708 |
| `oracle_pair` | 0.852 | +0.002599 | +0.005643 | +0.005712 | +0.004721 | +0.000419 |
| `aggressive` | 1.000 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `cv_hgb` | 0.966 | +0.001647 | +0.003533 | +0.003105 | +0.002142 | -0.000070 |
| `cv_logistic` | 0.857 | +0.002078 | +0.003114 | +0.002229 | +0.001554 | -0.000015 |
| `conservative` | 1.000 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |

## Interpretation

M1190 gives a precise mixed result.

Positive:

- The oracle gap is large.  Pairwise oracle (`aggressive` vs `conservative`)
  improves all metrics and makes CUB positive.
- The best3 oracle is stronger still, showing that a query-level policy has a
  real ceiling above both fixed policies.
- Runtime qrels-free features are sufficient to train stable models and replay
  through leave-one-dataset-out.

Negative:

- The learned accept gates do not beat fixed aggressive.  HGB and logistic both
  remain close to aggressive and keep small negative CUB.
- The label distribution is strongly imbalanced toward aggressive.  Many
  datasets have 80%-100% aggressive-favorable queries, so an accept classifier
  naturally over-admits.
- This does not solve cqadupstack-style row harm.

## Diagnosis

The blocker is not whether context-conditioned posting deltas have value.
They do.  The blocker is the control formulation.

Training a model to answer "is aggressive better than conservative?" is too
easy and biased toward accepting aggressive.  The real need is narrower:
identify the minority of high-risk queries where aggressive should be vetoed.

That suggests a risk-first objective, not an accept-first objective.

## Decision

Do not scale the M1190 accept gate.

Keep the following as retained evidence:

1. Oracle policy ceiling is meaningfully above fixed aggressive/conservative.
2. Conservative remains a valid safe fallback.
3. The next gate should be a damage-veto model with high precision on harmful
   aggressive moves, not a generic accept classifier.

## Next Step

M1191 should train a damage-veto policy from the M1190 cached examples without
rerunning native DB queries.

Proposed label:

- harmful if aggressive utility is below conservative utility, or if aggressive
  causes CUB/MAP/NDCG/MRR harm relative to conservative.

Proposed policy:

- default to aggressive
- veto to conservative only when harm probability is high
- threshold selected on training fold with explicit CUB and row-harm penalty

Acceptance:

- Keep at least 70% of aggressive MAP/NDCG/MRR gains.
- Make macro CUB non-negative.
- Reduce heldout row harm compared with fixed aggressive.

If damage-veto also fails, the issue is likely missing observability in current
features/proposals, not training depth.
