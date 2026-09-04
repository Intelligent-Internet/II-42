# SAE M80-A0 Neutral Corpus Report

Date: 2026-05-21

Status: M80-A0 implemented and smoke-validated. This report records the first
neutral mixed candidate-set corpus artifact for the M80 two-stage reset. It is
not a model-quality result.

## Summary

M80-A0 builds the data surface needed before Stage-A model training:

```text
neutral / mixed text
  + qrel positives
  + dense-teacher hard negatives
  + BM25 hard negatives
  + random negatives
  -> candidate-set corpus for Stage-A representation training
```

This follows the `diffsae-codex` lesson that training should happen over
candidate sets instead of whole-corpus differentiable TopK. It also avoids the
M70-M76 failure mode where sparse/final-ranking training tries to learn
representation, BM25 complementarity, and query calibration all at once.

## Implemented Artifact Builder

Script:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m80_neutral_corpus.py \
  --output-dir /Volumes/Betty/Tmp/ii42_sae_m80/neutral-stage-a-v0
```

Outputs:

- `documents.jsonl`
- `queries.jsonl`
- `candidate_rows.jsonl`
- `manifest.json`
- `summary.md`

The output directory is outside the repo because it is a data artifact:

```text
/Volumes/Betty/Tmp/ii42_sae_m80/neutral-stage-a-v0
```

## V0 Counts

| Metric | Value |
| --- | ---: |
| Documents | 28,351 |
| Queries | 540 |
| Candidate rows | 440 |
| Artifact size | 506 MB |
| Train docs | 23,210 |
| Validation docs | 2,836 |
| Holdout docs | 2,305 |

## Source Mix

| Source | Family | Docs | Queries | Candidate rows |
| --- | --- | ---: | ---: | ---: |
| `beir15:arguana` | `beir15_current_eval_surface` | 434 | 40 | 40 |
| `beir15:fiqa` | `beir15_current_eval_surface` | 475 | 40 | 40 |
| `beir15:msmarco` | `beir15_current_eval_surface` | 3,967 | 40 | 40 |
| `beir15:nfcorpus` | `beir15_current_eval_surface` | 1,313 | 40 | 40 |
| `beir15:scifact` | `beir15_current_eval_surface` | 438 | 40 | 40 |
| `beir15:trec-covid` | `beir15_current_eval_surface` | 15,033 | 40 | 40 |
| `m39:fiqa` | `broad_generated_query_surface` | 421 | 40 | 40 |
| `m39:msmarco` | `broad_generated_query_surface` | 342 | 40 | 40 |
| `m39:m39-trec-covid-broad-neighborhood` | `broad_generated_query_surface` | 3,145 | 40 | 40 |
| `m39:nfcorpus` | `broad_generated_query_surface` | 1,247 | 40 | 40 |
| `m39:scifact` | `broad_generated_query_surface` | 336 | 40 | 40 |
| `m52:balanced-stage-a` | `neutral_representation_mix` | 400 | 100 | 0 |
| `real:arxiv` | `commons_proxy_representation` | 400 | 0 | 0 |
| `real:pubmed` | `commons_proxy_representation` | 400 | 0 | 0 |

Policy data is not included in the promotion artifact. It remains an optional
held-out probe via `--include-policy-probe`.

## Candidate-Set Shape

Every candidate row currently has at least one qrel-positive document retained.
The V0 artifact contains:

| Label | Count |
| --- | ---: |
| Positive candidate labels | 9,808 |
| Negative candidate labels | 15,744 |
| Max candidates per row | 64 |

Candidate sources are tracked per candidate:

- `qrel_positive`
- `dense_teacher`
- `bm25`
- `random`

This source tracking is important for M80-A1/A2 diagnostics. It lets the next
trainer separate semantic teacher misses from lexical BM25 misses instead of
collapsing everything into one scalar loss.

## Quality-Claim Policy

This artifact is a training and evaluation harness input. It does not prove
product relevance quality.

- BEIR/M39 qrels are supervised candidate-set rows.
- M52 rows are neutral representation examples.
- arXiv/PubMed rows are representation and distribution probes.
- Policy rows are excluded by default and must stay held out until neutral
  Stage A passes.

## Next Step

M80-A1 should train a dense student on this candidate-set corpus. The first
trainer should optimize representation quality and teacher-neighborhood
preservation, not BM25 final-ranking calibration.

Concrete next implementation:

```text
M80-A1 dense-student smoke
  input:  neutral-stage-a-v0 candidate rows
  train:  query/doc dense scorer over candidate sets
  losses: multi-positive candidate CE + small dense-teacher KL
  report: held-out candidate recall, teacher overlap, source-family deltas
```

Only after A1 produces a strong dense representation should M80-A2 train and
evaluate hard sparse SAE preservation.
