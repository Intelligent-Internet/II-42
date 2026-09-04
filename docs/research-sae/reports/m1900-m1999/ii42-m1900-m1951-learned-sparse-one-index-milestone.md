# M1900-M1951 Learned-Sparse One-Index Milestone

Date: 2026-07-13

## Status

This document indexes the complete M1900-M1951 development batch. It records
the reproducible code surface, the evidence retained from failed routes, and
the currently validated product candidate without promoting partial results
to a full BEIR15 conclusion.

The current learned-sparse one-index frontier is M1934 `b1.125`:

```text
exact lexical postings + bounded Granite semantic postings
    -> disjoint namespaces in one sparse vector
    -> one physical inverted index
    -> one additive sparse dot product
```

It is validated on four selection corpora and three frozen unseen corpora,
with exact native replay on SciDocs. It is not yet a full 15-dataset result or
the product default.

## Evidence Map

| Stage | Question | Retained conclusion | Primary report |
| --- | --- | --- | --- |
| M1900-M1905 | Can a paper-shaped sparse route be reproduced locally? | More training repairs ranking, but the pretrained output basis is the decisive local advantage. The M1904 SAE branch is closed; M1905 is a mechanism control, not a product checkpoint. | `docs/research-sae/reports/m1900-m1999/ii42-m1900-m1920-learned-sparse-success-failure-factor-report.md` |
| M1910-M1912 | Can latent terms or external SAE/SPLARE artifacts provide a stronger basis? | M1911 proves that a broadly trained latent vocabulary can approach mature sparse quality in one exact index, but its posting cost is too high. External artifacts remain provenance- and compatibility-bound. | `docs/research-sae/reports/m1900-m1999/ii42-m1911-nomic-latent-terms-reproduction-report.md` |
| M1913-M1917 | Which mature parent should anchor further work? | M1914 Granite is the compact learned-sparse control, OpenSearch sparse-v2 is the strongest frozen head-ranking parent, and P1 remains the dense-faithfulness control. | `docs/research-sae/reports/m1900-m1999/ii42-m1917-tri-parent-native-report.md` |
| M1918-M1920 | Does local post-training or thresholding improve the mature parent? | Candidate-surface gains do not survive full-corpus native replay. Thresholding repairs cost but not frozen quality. The continuation route is closed. | `docs/research-sae/reports/m1900-m1999/ii42-m1917-m1920-parent-and-pplx-closure-report.md` |
| M1930-M1934 | Can lexical and semantic evidence share one physical index? | Yes. The additive one-index construction closes exactly. M1934 `b1.125` transfers to three unseen corpora and reproduces through the native path. | `docs/research-sae/reports/m1900-m1999/ii42-m1934-fixed-budget-unseen-transfer-report.md` |
| M1935-M1938 | Can the extra semantic budget be made cost-neutral by provenance or channel changes? | The additional tail contains real value, but DF-only, provenance-only, and co-keyed channel rules do not retain the gain at `b1` cost. | `docs/research-sae/reports/m1900-m1999/ii42-m1934-m1938-lexical-semantic-frontier-report.md` |
| M1939-M1942 | Can a small learned residual improve the fixed parent? | Local residual signals exist, but fixed-source and query-expansion training do not generalize safely. M1934 remains the frontier. | `docs/research-sae/reports/m1900-m1999/ii42-m1939-m1942-residual-generalization-report.md` |
| M1943-M1945 | Is residual utility observable over the complete action space? | Targets are low-DF and sampled target/harm separation is strong, but complete-source ranking fails. The selector family is closed before native promotion. | `docs/research-sae/reports/m1900-m1999/ii42-m1943-m1945-residual-observability-closure-report.md` |
| M1950-M1951 | Does swapping in OpenSearch improve the same one-index construction? | It gives a better four-row macro quality/cost point, but FiQA remains row-unsafe under both global and transferred calibration. Retain as mechanism evidence only. | `docs/research-sae/reports/m1900-m1999/ii42-m1950-m1951-opensearch-one-index-closure-report.md` |

## Current Product Frontier

M1934 `b1.125` freezes all model and scoring choices:

- Granite 30M sparse revision
  `ad82b1fd09541c998c8d45045d601c51fdb8a9b7`;
- M1914 top-192 document and top-50 query support;
- M1931 qrels-free `rms_m4` query calibration;
- exact II42 lexical postings in a disjoint namespace;
- semantic postings capped at `1.125x` lexical posting count;
- one additive sparse dot-product index at candidate depth 1,000;
- no dataset-specific selector, reranker, or learned gate.

The validated scope is:

- selection: FiQA, ArguAna, NFCorpus, and SciFact;
- frozen transfer: SciDocs, Quora, and TREC-COVID;
- native exact replay: SciDocs;
- four-fold leave-one-dataset-out budget selection: passed.

The principal unresolved risk is corpus-wide posting traversal. Relative to
`b1`, `b1.125` adds 12.5% semantic entries and 6.25% total entries. On the
three unseen rows it increases mean posting touches by 7.61%; TREC-COVID is
the highest-DF engineering risk.

## Code Organization

The committed code is grouped by purpose rather than by run output:

- `scripts/train_m1901_*` through `scripts/train_m1905_*` reproduce the paired
  SAE and standard-SPLADE training controls.
- `scripts/prepare_m1910_*` through `scripts/prepare_m1920_*` build pinned
  latent-term, Granite, OpenSearch, P1, and checkpoint comparison surfaces.
- `scripts/summarize_m1911_*` through `scripts/summarize_m1920_*` generate
  auditable matrices from those surfaces.
- `scripts/audit_m1930_*` through `scripts/audit_m1938_*` implement the
  lexical-semantic one-index construction and fixed-budget frontier.
- `scripts/audit_m1939_*` through `scripts/audit_m1945_*` isolate residual
  observability and complete-action-space failure mechanics.
- `scripts/run_m1950_*` and `scripts/run_m1951_*` reproduce the mature-parent
  swap and frozen calibration transfer.
- matching `tests/test_*m19*.py` files cover loaders, manifests, sparse
  transformations, metric summaries, contracts, and stop gates.

Small JSON matrices, manifests, source identities, and environment freezes are
committed because they make the reports auditable. Checkpoints, model weights,
activation caches, corpus matrices, query-row dumps, and `runs/` contents are
not part of this milestone.

## Promotion Boundaries

The following claims are supported:

1. Mature learned-sparse semantic evidence and exact lexical evidence can be
   accumulated in one physical inverted index.
2. A fixed 1.125x semantic budget improves the seven measured rows without
   dataset-specific tuning.
3. The gain survives an exact native replay on the measured SciDocs surface.
4. M1911 proves that latent vocabularies can be competitive when trained at
   sufficient scale, but it does not meet the current cost target.

The following claims are not supported yet:

1. M1934 is not a complete official BEIR15 result.
2. M1934 is not proven to beat dense retrieval globally.
3. M1951 is not row-safe and must not replace M1934.
4. The M1939-M1945 learned residual family is not deployable.
5. Candidate-surface or sampled-AUC gains must not be presented as native
   product gains without complete-source and native replay gates.

## Reproduction Entry Points

- Learned-sparse reset: `scripts/run_m1900_paper_native_sparse_audit_spark.sh`
- Granite parent: `scripts/run_m1913_granite_sparse_native_spark.sh`
- M1914 calibration: `scripts/run_m1914_granite_fixed_support_calibration_spark.sh`
- Common native parent matrix: `scripts/run_m1917_tri_parent_common_native.sh`
- M1934 transfer preparation: `scripts/run_m1934_granite_transfer_surface_spark.sh`
- M1934 fixed-budget audit: `scripts/audit_m1934_fixed_budget_transfer.py`
- OpenSearch parent swap: `scripts/run_m1950_opensearch_one_index_audit_spark.sh`

## Milestone Decision

Freeze M1934 `b1.125` as the current validated learned-sparse one-index
candidate. Keep M1914 as the compact semantic parent, frozen OpenSearch as the
mature ranking control, and P1 as the dense-faithfulness control. Close the
current residual-selector and scalar-calibration families.

The next promotion gate is a frozen, same-run broader regression and native
cost audit. It must not be replaced by another local training or scalar sweep.
