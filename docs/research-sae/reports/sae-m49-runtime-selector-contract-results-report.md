# SAE M49 Runtime Selector Contract Results Report

Status: closed. M49 produced the first runtime selector research contract.

## Summary

M49 converted the M46/M48 canonical selector into a product-shaped but
non-public contract:

```text
contract_id = sae_runtime_selector_m49_v0
status = research_contract_not_public_api
```

The runtime shape remains:

```text
query text
-> one text-to-atoms encoder
-> support/value logits
-> runtime-safe fanout selector
-> selected export profile
-> unified evidence-atom sparse search
```

## Contract Output

```text
results/sae/m49/runtime-selector-contract/sae_runtime_selector_contract.json
results/sae/m49/runtime-selector-contract/sae_runtime_selector_contract.md
```

## Model Contract

```text
checkpoint = results/sae/m40/lexical-df-gate-train-eval-current/m31_joint_final_ranking_student.pt
encoder_count = 1
latent_dims = 8192
feature_mode = token_char
max_tokens = 512
```

This explicitly answers the multi-encoder question: M49 does not require
multiple encoders. Runtime selection happens after the single encoder produces
support/value logits.

## Selector Contract

```text
selector = fanout_to_m44
signal_profile = m45_high_quality
signal = predicted_sae_postings
threshold = 3000
if predicted_sae_postings >= 3000:
    use m44_low_cost
else:
    use m45_high_quality
```

The selector does not require:

- dataset id;
- qrels;
- query-time dense embedding;
- multiple encoders.

## Profiles

| Profile | Pool | Export | Fanout power | Low SAE | High SAE | Role |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `m44_low_cost` | 96 | 48 | 0.25 | 0.40 | 1.00 | high-fanout fallback |
| `m45_balanced` | 128 | 48 | 0.10 | 0.45 | 1.00 | available inactive fallback |
| `m45_high_quality` | 96 | 48 | 0.15 | 0.45 | 1.00 | default profile |

## Evidence

| Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.871949 | 0.917221 | 0.801862 | 0.775242 | 0.463246 | 2844.088 | 1312.060 |

Holdout summary:

- LODO selected canonical in 13 of 15 splits.
- The 2 non-canonical LODO selections were worse on heldout NDCG/MAP.
- Family holdout found no clear replacement that dominates canonical.

## Non-Goals

M49 intentionally does not freeze:

- SQL/API;
- mutable index behavior;
- maintenance design;
- dense-removal product claims;
- real-workload quality claims without qrels or proxy-qrels.

## Decision

M49 is the first engineering handoff point. The next phase can implement a
read-only runtime-query prototype against this contract, but productization
still requires native/runtime parity and real-corpus efficiency evidence.
