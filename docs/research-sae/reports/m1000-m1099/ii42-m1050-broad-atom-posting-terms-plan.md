# M1050 Broad Atom Posting Terms Plan

Date: 2026-06-16

## Goal

M1040.2 showed the important new signal:

```text
direct-from-scratch atom posting training can create low-fanout atoms,
but nfcorpus-only supervision makes those atoms query-specific.
```

M1050 tests the next necessary condition: train one atom vocabulary on a
broader cross-dataset surface and evaluate heldout queries per dataset.

The purpose is still not a learned fusion head. The target is:

```text
SAE atoms become reusable posting terms.
```

This follows the visual-words paper insight more directly: an atom should be a
term with useful df/idf behavior, not a latent vector feature that requires a
post-hoc dense-style scorer to rescue it.

## Data

Initial canary datasets:

- `nfcorpus`;
- `scifact`;
- `fiqa`.

These are chosen because:

- they are available in the current M1000 official surface;
- they cover different query/doc styles;
- `fiqa` is large enough to expose fanout problems without requiring full
  BEIR15 runtime.

Each dataset uses:

- official `documents.input.jsonl`;
- official `queries.input.jsonl`;
- official `quality_qrels.json`;
- deterministic 70/30 train/heldout query split.

## Training Objective

M1050 reuses the M1040 posting utility objective, with two key changes:

1. one SAE atom vocabulary is trained over multiple datasets;
2. background documents are sampled from all training datasets, so batch IDF
   and background-overlap pressure are less dataset-specific.

No reconstruction pretraining is used in the primary canary. M1040 showed that
reconstruction pretraining tends to create high-df head atoms that the posting
loss cannot easily undo.

## Evaluation

For each dataset:

- lexical BM25;
- M1050 atom-BM25;
- unified lexical + atom-BM25 at fixed scales.

Report:

- all/train/heldout metrics;
- atom df diagnostics;
- avg touched postings/docs.

Also report a query-weighted aggregate across datasets.

## M1050.0 Pass/Fail

Pass signal:

- heldout atom-BM25 improves materially over M1040.2 heldout collapse;
- atom df/fanout remains much healthier than M1030/M1040.0;
- unified heldout improves without simply reverting to lexical BM25.

Fail signal:

- atom-BM25 train is strong but heldout remains weak across datasets;
- df/fanout returns to whole-corpus behavior;
- one dataset dominates and others collapse.

If M1050.0 fails, the next useful route is not scalar tuning. It should change
the representation side:

- query/document asymmetric atom vocabularies;
- stronger pooling from multiple text spans;
- curriculum from broad atom stability to ranking utility.

## Current Status

Implementation:

- `scripts/research_sae_m1050_broad_atom_posting_terms.py`
- `scripts/run_m1050_broad_atom_posting_terms_spark.sh`

Execution target:

- `spark-1`
- remote output root:
  `/home/huoju/leask/runs/ii42-m1050-broad-atom-posting-terms-v1`

## M1050.0 Partial Result

Status: training completed; `nfcorpus` and `scifact` partial eval completed;
`fiqa` full-corpus eval is still pending because full PPLX token-hidden pooling
over `57,638` documents is slow. The model checkpoint is already saved, so
the next step can run cached or sharded eval without retraining.

Artifacts:

- Remote checkpoint:
  `/home/huoju/leask/runs/ii42-m1050-broad-atom-posting-terms-v1/m1050_broad_atom_posting_terms.pt`
- Local checkpoint:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1050_broad_atom_posting_terms.pt`
- Remote partials:
  `/home/huoju/leask/runs/ii42-m1050-broad-atom-posting-terms-v1/m1050_broad_atom_posting_terms_nfcorpus.partial.json`
  `/home/huoju/leask/runs/ii42-m1050-broad-atom-posting-terms-v1/m1050_broad_atom_posting_terms_scifact.partial.json`
- Local partials:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1050_broad_atom_posting_terms_nfcorpus.partial.json`
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1050_broad_atom_posting_terms_scifact.partial.json`

Training surface:

| Dataset | Docs | Queries | Used train queries | Pairs | Needed docs | Background docs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 3,633 | 323 | 226 | 1,740 | 1,039 | 512 |
| `scifact` | 5,183 | 300 | 210 | 904 | 874 | 512 |
| `fiqa` | 57,638 | 648 | 300 | 2,000 | 1,642 | 512 |

Training signal:

- total pairs: `4,644`;
- query hidden rows: `736`;
- doc hidden rows: `3,555`;
- background hidden rows: `1,536`;
- rank loss: `0.6868 -> 0.1796`;
- fanout loss: `27.7565 -> 0.1661`;
- background overlap: `0.0900 -> 0.0106`.

This confirms that cross-dataset direct atom posting training can still learn
a low-fanout posting vocabulary. It does not yet prove heldout generalization.

### Partial Evaluation

`nfcorpus`:

| Row | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| lexical BM25 | heldout | 0.2411 | 0.4625 | 0.2901 | 0.1303 | 1,938.5 |
| M1050 atom-BM25 | heldout | 0.0705 | 0.0989 | 0.0633 | 0.0277 | 164.5 |
| unified scale 0.5 | heldout | 0.2611 | 0.4573 | 0.2872 | 0.1301 | n/a |
| unified scale 1 | heldout | 0.2594 | 0.4052 | 0.2398 | 0.1054 | n/a |

Atom df:

- active atoms: `3296`;
- max df ratio: `0.1519`;
- head 1% df ratio: `0.0696`;
- avg touched docs: `160.9`.

`scifact`:

| Row | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| lexical BM25 | heldout | 0.8241 | 0.5526 | 0.5793 | 0.5460 | 14,469.0 |
| M1050 atom-BM25 | heldout | 0.4689 | 0.2723 | 0.2837 | 0.2748 | 270.3 |
| unified scale 0.5 | heldout | 0.8552 | 0.6011 | 0.6240 | 0.5938 | n/a |
| unified scale 1 | heldout | 0.8552 | 0.6007 | 0.6202 | 0.5940 | n/a |

Atom df:

- active atoms: `3538`;
- max df ratio: `0.2126`;
- head 1% df ratio: `0.0777`;
- avg touched docs: `246.8`.

## M1050.0 Interpretation

What improved compared with M1040:

- fanout is still healthy under a broader train surface;
- heldout unified results can improve over lexical BM25 on `scifact`;
- `nfcorpus` recall also improves slightly at low SAE scale without increasing
  top-rank quality.

What still fails:

- atom-only heldout is still weak, so the atoms are not yet a standalone
  semantic posting vocabulary;
- `nfcorpus` ranking quality drops when atom weight is too high;
- `fiqa` full eval is too slow without cached doc atom vectors.

Decision:

- M1050 validates the direction more than M1040, but it is not a model
  candidate yet.
- The immediate next step should be engineering the evaluator/training loop:
  save checkpoints, cache doc atom vectors, and run dataset eval independently.
- After that, M1060 should train with a broader surface and a more explicit
  generalization constraint, not another single-dataset penalty sweep.

