# II-42 M405 Overlap-Anchored Query Correction Plan

Date: 2026-06-27

## Motivation

M403-A produced a repeated learned-only NDCG gain over deterministic structural
tail, but Dense O@100 regressed.  M404 tried a broader shared dense-space
adapter, but the 4-task canary was below the deterministic teacher:

- learned-only macro: `0.43071`
- deterministic structural macro: `0.43313`
- learned + BM25 a0.10 macro: `0.50223`
- deterministic structural + BM25 a0.10 macro: `0.50269`

The failure mode suggests that adapting original dense coordinates before each
task-specific structural projection is too blunt.  It disrupts the structural
geometry that makes M392-M396 work.

## Hypothesis

The better next test is to keep the correction inside each task's structural
coordinate space:

```text
query_dense + base query structural coordinates -> corrected structural query
```

Document postings remain deterministic and frozen.  The model is still
qrels-free; qrels are evaluation only.

## Loss

M405 keeps the M403-A dense score/listwise/pairwise distillation terms and adds
two preservation terms:

- dense-pool score anchor: on exact dense top-k candidates, corrected scores
  should not drift too far from deterministic structural scores;
- structural query anchor: corrected query coordinates and sketches should stay
  close to deterministic query coordinates.

This is meant to reduce Dense O@100 damage without returning to the fully
deterministic teacher.

## Gate

Run FiQA first against the M403 seed404 surface:

- target: learned-only NDCG must beat deterministic structural tail;
- dense overlap: Dense O@100 should regress less than M403-A seed404;
- if FiQA passes, run the same canary on `FiQA2018,ArguAna,SCIDOCS,TRECCOVID`.
