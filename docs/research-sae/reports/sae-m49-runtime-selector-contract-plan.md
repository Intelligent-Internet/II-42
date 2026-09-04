# SAE M49 Runtime Selector Contract Plan

Status: closed as an engineering pre-contract, not a product API freeze.

## Goal

M46 and M48 established a stable one-encoder runtime selector:

```text
one text-to-atoms encoder
+ multiple runtime export profiles
+ cheap fanout selector
```

M49 turns that research result into an explicit contract so the next
engineering phase has a single source of truth. The contract must avoid product
overclaiming: it is not a SQL/API freeze, not a mutable index design, and not a
dense-removal product claim.

## Deliverable

Add:

```text
scripts/research_sae_m49_runtime_contract.py
```

The script reads:

```text
results/sae/m48/selector-holdout-full15/m48_selector_holdout.json
results/sae/m40/lexical-df-gate-train-eval-current/m31_joint_final_ranking_student.pt
```

and emits:

```text
results/sae/m49/runtime-selector-contract/sae_runtime_selector_contract.json
results/sae/m49/runtime-selector-contract/sae_runtime_selector_contract.md
```

## Contract Scope

The contract must specify:

- one encoder checkpoint and encoder config;
- the runtime selector rule;
- the available export profiles;
- required runtime-safe signals;
- the current full15 and holdout evidence;
- explicit non-goals.

## Canonical Runtime Rule

```text
profile = m45_high_quality
if predicted_sae_postings(profile=m45_high_quality) >= 3000:
    profile = m44_low_cost
```

This requires no dataset id, no qrels, no query-time dense embedding, and no
second encoder.

## Acceptance Criteria

- Contract JSON is machine-readable and generated from current evidence.
- Contract Markdown is human-readable and concise.
- Contract preserves M48's non-goals.
- Validation passes `py_compile`, `git diff --check`, and JSON parse checks.
