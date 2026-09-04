# SAE Block-Max Phase 4.7 Candidate-Budget Exact Rerank Report

Date: 2026-05-12

## Purpose

Phase 4.7 follows the negative exact-skip results from Phase 4.5 and Phase 4.6.
The question is no longer whether another exact traversal variant can avoid
opening most SAE postings. The new question is whether a deliberately
approximate candidate generator can shrink the native query path while a full
SAE exact rerank preserves retrieval quality.

This is the first phase that treats exact full-SAE top-k parity as a diagnostic,
not as the acceptance criterion. The acceptance criterion is qrels quality under
a fixed candidate budget.

## Prototype

The new script is:

```text
scripts/research_sae_candidate_budget_rerank.py
```

The tested query path is:

```text
full SAE query dimensions
  -> select a small candidate-generation subset of query dimensions
  -> collect top impact-ordered postings from those dimensions
  -> union candidate documents
  -> rerank candidates with the full original SAE query dimensions
  -> return top-k
```

The rerank step is exact for the candidate set. Loss can only come from missing
documents during candidate generation.

Three selector families are tested:

- `top_weight`: strongest query activations first;
- `df_penalty`: strongest query activations penalized by latent document
  frequency;
- `incremental`: greedy selection that penalizes dimensions which add many new
  candidate documents instead of sharpening the current pool.

## Full SAE Baseline

The baseline is full-query SAE exact ranking over all documents.

| Dataset | Full R@100 | Full MRR@20 |
| --- | ---: | ---: |
| `scifact` | `0.9500` | `0.7206` |
| `scidocs` | `0.7025` | `0.5973` |
| `nfcorpus` | `0.3590` | `0.6546` |
| `arguana` | `1.0000` | `0.4399` |
| `fiqa` | `0.9126` | `0.7218` |

## Dataset Best Rows

The smaller-budget sweep is more informative for physical design because it
targets the 100-400 candidate range.

| Dataset | Best config | R@100 | MRR@20 | Candidates | Postings touched | Exact@100 overlap |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `top_weight d=16 pdim=24` | `0.9550` | `0.7214` | `311.0` | `379.0` | `0.6825` |
| `scidocs` | `incremental d=16 pdim=32 p=0.25` | `0.6945` | `0.5962` | `348.0` | `460.6` | `0.7520` |
| `nfcorpus` | `incremental d=16 pdim=32 p=0.25` | `0.3683` | `0.6562` | `336.9` | `443.2` | `0.7290` |
| `arguana` | `df_penalty d=12 pdim=16` | `1.0000` | `0.4400` | `127.1` | `189.5` | `0.5325` |
| `fiqa` | `top_weight d=16 pdim=32` | `0.9133` | `0.7231` | `378.6` | `499.1` | `0.7882` |

## General-Purpose Config Candidates

The cross-dataset view is more important than per-dataset best rows because the
index should not require per-corpus tuning.

| Config | Mean R@100 delta | Mean MRR@20 delta | Mean candidates | Mean exact@100 overlap |
| --- | ---: | ---: | ---: | ---: |
| `top_weight d=16 pdim=32` | `-0.0050` | `+0.0008` | `377.8` | `0.7727` |
| `incremental d=16 pdim=32 p=0.25` | `-0.0063` | `+0.0005` | `335.3` | `0.7575` |
| `top_weight d=12 pdim=32` | `-0.0099` | `+0.0010` | `292.8` | `0.6873` |
| `incremental d=16 pdim=24 p=1.00` | `-0.0132` | `+0.0005` | `244.6` | `0.6595` |
| `incremental d=12 pdim=32 p=0.50` | `-0.0171` | `+0.0003` | `240.9` | `0.6485` |

`top_weight d=16 pdim=32` is the best conservative prototype default. It loses
about half a recall point on average while keeping MRR flat or slightly better.
`incremental d=16 pdim=32 p=0.25` is the best lower-cost alternative.

The low exact-overlap values are expected. This path is not trying to reproduce
the full SAE top-100 exactly; it is trying to preserve judged retrieval quality.
In several datasets, qrels metrics slightly improve because the candidate
truncation removes some high-scoring but irrelevant full-SAE documents.

## Decision

This direction is worth continuing. It is the first Phase 4 physical/query
design that changes the cost curve materially without immediately losing qrels
quality.

Do not replace the exact read-only path yet. Keep the current exact doc-bound
reader as the correctness reference, and add the candidate-budget path as an
approximate mode in the next prototype.

The next native design should have two stages:

```text
candidate generator
  -> dimension-local impact-ordered posting heads
  -> compact candidate document accumulator
  -> optional candidate budget / early stop

exact candidate rerank
  -> full query dimensions
  -> doc-bound exact score lookup
  -> visible top-k
```

The representation problem is still present. A pure physical truncation gets to
roughly 240-380 candidates on these 2k-document slices. A production-scale
index will need either better query-side latent sharpening or an adaptive
candidate budget tied to observed score concentration.

## Verification

Commands run:

```bash
python3 -m py_compile scripts/research_sae_candidate_budget_rerank.py

python3 scripts/research_sae_candidate_budget_rerank.py \
  --datasets scifact \
  --max-queries 10 \
  --active-dims 8,16 \
  --postings-per-dim 32,0 \
  --incremental-penalties 0.5 \
  --output-dir results/sae/phase47/candidate-budget-rerank-smoke

python3 scripts/research_sae_candidate_budget_rerank.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --active-dims 8,16,32,64 \
  --postings-per-dim 32,64,128,0 \
  --incremental-penalties 0.25,0.5,1.0 \
  --output-dir results/sae/phase47/candidate-budget-rerank

python3 scripts/research_sae_candidate_budget_rerank.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --active-dims 4,8,12,16 \
  --postings-per-dim 8,16,24,32 \
  --incremental-penalties 0.25,0.5,1.0 \
  --output-dir results/sae/phase47/candidate-budget-rerank-small-budget
```
