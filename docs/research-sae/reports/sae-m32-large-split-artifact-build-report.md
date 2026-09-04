# SAE M32 Large-Split Artifact Build Report

Status: M32.0 split artifact build completed.

M32 starts by fixing the data problem exposed by M31. The previous training
surface used only the prepared full15 artifact: 49,059 documents, 1,342 queries,
and 39,742 qrel pairs. That surface remains useful as a regression benchmark,
but it is too small and too close to evaluation to close the product model
gate.

This build creates clean official-BEIR train/test split artifacts under:

```text
/Volumes/Betty/Tmp/ii42_sae_m32_splits
```

The build writes text-only `documents.jsonl`, `queries.jsonl`,
`quality_qrels.json`, `qrels.jsonl`, and `manifest.json` files. Embeddings and
SAE teacher latents were intentionally not generated in this step; follow-up
materialization and training results are tracked in the M32 training report.

## Build Configuration

| Field | Value |
| --- | --- |
| Official root | `/Volumes/Betty/Tmp/ii42_dataset_cache/beir_official` |
| Current regression root | `/Volumes/Betty/Tmp/ii42_sae_beir15_shared` |
| Output root | `/Volumes/Betty/Tmp/ii42_sae_m32_splits` |
| Max docs per artifact/dataset | 20,000 |
| Max train queries per dataset | 300 |
| Max official-test queries per dataset | 300 |
| Seed | 53 |

## Artifact Totals

| Artifact | Datasets | Documents | Queries | Qrel pairs |
| --- | ---: | ---: | ---: | ---: |
| `m32-train-20k-q300` | 8 | 128,816 | 2,167 | 18,957 |
| `m32-test-official` | 15 | 257,490 | 3,741 | 59,123 |

The train artifact is built only from official train/dev qrels. The official
test artifact is built separately and is never eligible for training.

## Dataset Summary

| Dataset | Train queries | Train docs | Test queries | Test docs | Leakage |
| --- | ---: | ---: | ---: | ---: | --- |
| `arguana` | 0 | 0 | 300 | 8,674 | `False` |
| `climate-fever` | 0 | 0 | 300 | 20,000 | `False` |
| `cqadupstack` | 0 | 0 | 300 | 20,000 | `False` |
| `dbpedia-entity` | 67 | 20,000 | 300 | 20,000 | `False` |
| `fever` | 300 | 20,000 | 300 | 20,000 | `False` |
| `fiqa` | 300 | 20,000 | 300 | 20,000 | `False` |
| `hotpotqa` | 300 | 20,000 | 300 | 20,000 | `False` |
| `msmarco` | 300 | 20,000 | 43 | 20,000 | `False` |
| `nfcorpus` | 300 | 3,633 | 300 | 3,633 | `False` |
| `nq` | 0 | 0 | 300 | 20,000 | `False` |
| `quora` | 300 | 20,000 | 300 | 20,000 | `False` |
| `scidocs` | 0 | 0 | 300 | 20,000 | `False` |
| `scifact` | 300 | 5,183 | 300 | 5,183 | `False` |
| `trec-covid` | 0 | 0 | 50 | 20,000 | `False` |
| `webis-touche2020` | 0 | 0 | 49 | 20,000 | `False` |

## Leakage Check

The builder excludes all current regression query IDs and all official test
query IDs from the train artifact.

Result:

```text
leaks = []
```

This is the key difference from the previous 1,342-query loop: M32 can now
train on official train/dev queries and evaluate on both the previous
regression surface and official held-out test surfaces without direct query-id
contamination.

## Notes

- Datasets with no official train/dev split remain holdout-only for M32.
- `trec-covid` remains hard holdout-only. If it remains the main collapse after
  M32.1/M32.2, the next move should be a separate biomedical/claim-heavy data
  track, not hidden aggregate optimization.
- `nfcorpus` and `scifact` have fewer than 20k available documents, so their
  artifacts include the available corpus.
- `arguana` official test has only 8,674 docs, so the test artifact includes
  the available corpus.

## Commands

Smoke:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m32_build_split_artifacts.py \
  --output-root /Volumes/Betty/Tmp/ii42_sae_m32_splits_smoke \
  --datasets fiqa scifact trec-covid \
  --max-docs 500 \
  --max-train-queries 20 \
  --max-test-queries 20
```

Full split:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m32_build_split_artifacts.py \
  --output-root /Volumes/Betty/Tmp/ii42_sae_m32_splits \
  --max-docs 20000 \
  --max-train-queries 300 \
  --max-test-queries 300
```

Validation:

```bash
PYTHONPATH=scripts python3 -m py_compile \
  scripts/research_sae_m32_build_split_artifacts.py
git diff --check
```

## Follow-Up Status

M32.1 and M32.2 are now covered by:

```text
sae-m32-large-split-training-results-report.md
```

The train artifact was materialized into Snowflake/SAE teacher atoms, candidate
coverage was measured, and two final-ranking query-side runs were completed.
The result improved aggregate quality but did not pass the robustness gate, so
the model is not promoted.
