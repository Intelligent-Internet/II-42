# SAE M48 Selector Holdout Robustness Results Report

Status: closed. M46 `fanout_to_m44@3000` remains the canonical research
profile.

## Summary

M48 checked whether the M46 selector was overfit to the full15 aggregate. The
answer is: the risk exists in principle, but this selector is stable enough to
keep.

The full15 quality-cost objective again selects:

```text
fanout_to_m44
df_threshold = 0.12
fanout_threshold = 3000
bm25_concentration_threshold = 0.10
```

Canonical result:

| Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.871949 | 0.917221 | 0.801862 | 0.775242 | 0.463246 | 2844.088 | 1312.060 |

## LODO Result

LODO selection chose the exact canonical selector in 13 of 15 heldout datasets.

The two exceptions did not invalidate the canonical rule:

| Heldout | LODO-selected policy | Heldout impact vs canonical |
| --- | --- | --- |
| `msmarco` | `fanout_to_m44@2000` | worse Recall, NDCG, and MAP |
| `trec-covid` | `df_balanced_fanout_m44@df0.08/fanout3000` | slightly higher MRR, but worse NDCG and MAP |

This is a useful anti-overfitting signal: when training-side selection deviates
from canonical, the heldout result does not improve.

## Family Holdout Result

Family holdout also supports keeping canonical:

| Family | Selected policy | Heldout impact vs canonical |
| --- | --- | --- |
| `biomedical` | `df_balanced_fanout_m44@df0.08/fanout3000` | small Recall/MAP gain but lower NDCG |
| `web_qa` | `df_balanced_fanout_m44@df0.20/fanout1500` | worse Recall, MRR, NDCG, MAP |
| `fact_entity` | canonical | identical |
| `argument_finance_science` | canonical | identical |

There is no family-level replacement that clearly dominates M46 canonical.

## Evidence

Primary output:

```text
results/sae/m48/selector-holdout-full15/m48_selector_holdout.md
results/sae/m48/selector-holdout-full15/m48_selector_holdout.json
```

## Decision

M48 keeps M46 canonical:

```text
one encoder
+ three runtime export profiles
+ fanout selector
```

This still does not close the dense-removal product gate, but it answers the
immediate overfitting concern well enough to proceed with product-shaped
engineering design around a single encoder and runtime profile selector.

## Product Shape Implication

M48 does not require multiple encoders. The runtime shape remains:

```text
text
-> one text-to-atoms encoder
-> support/value logits
-> cheap selector over runtime-safe signals
-> one selected export profile
-> unified evidence-atom sparse search
```

The selector changes only export/scoring policy. It does not require separate
model weights.
