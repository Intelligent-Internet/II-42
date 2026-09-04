# SAE M48 Selector Holdout Robustness Plan

Status: closed. M46 canonical selector is robust enough to remain the current
research profile.

## Goal

M46 promoted a runtime-safe profile selector:

```text
fanout_to_m44
if M45-high predicted SAE postings >= 3000:
    use M44 low-cost
else:
    use M45 high-quality
```

M48 tests whether this is a full15 aggregate artifact. The concern is
overfitting: selector thresholds were swept on the same 15 BEIR datasets used
for reporting. M48 therefore does not train a new model. It runs holdout
selection over the same selector grid and asks whether the canonical M46 rule
is stable.

## Method

Add:

```text
scripts/research_sae_m48_selector_holdout.py
```

The script reuses the M46 profile artifacts and selector grid, but stores
per-dataset metrics and physical cost for each policy. It then runs:

- full15 selector ranking;
- leave-one-dataset-out selection;
- dataset-family holdout selection.

The selection objective is the same M46 quality-cost score:

```text
NDCG@10 + MAP@100 - 0.02 * (SAE postings / M45_high_quality postings)
```

This keeps the holdout test aligned with M46, while verifying that the
canonical threshold is not a brittle single-dataset choice.

## Families

```text
biomedical:
    nfcorpus, scifact, trec-covid

web_qa:
    cqadupstack, hotpotqa, msmarco, nq, quora

fact_entity:
    climate-fever, dbpedia-entity, fever

argument_finance_science:
    arguana, fiqa, scidocs, webis-touche2020
```

## Acceptance Rule

Keep M46 canonical if:

- full15 quality-cost still selects `fanout_to_m44@3000`;
- LODO mostly selects the same policy;
- when LODO selects a different policy, heldout quality does not consistently
  beat canonical;
- family holdout does not reveal a clear family-specific better policy.

If these fail, the selector should be simplified or parked before engineering.
