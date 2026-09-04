# ii42 M332 Full-Corpus Natural Admission/Ranking Plan

## Summary

M332 is the clean follow-up to M326-M331. It tests the missing experiment:

```text
full-corpus natural candidate union + final admission/ranking objective
```

The goal is not another scalar fusion sweep or a post-hoc preservation gate.
M332 keeps the current posting encoder/checkpoint fixed and changes the scorer
training surface so the final scorer sees the same natural evidence families
that the runtime path can use.

## What Was Missing

M323-M331 trained on a BM25 plus atom candidate pool. Dense teacher scores were
used as supervision, but dense-teacher candidates were not part of the training
candidate surface. Also, the candidate pool did not explicitly include the
full-score BM25+atom unified top-k documents; it only included independent BM25
and atom top-k lists.

That means the previous scorer line was close to the final objective, but not a
complete full-corpus natural admission/ranking objective.

## M332 Candidate Contract

For each query, the candidate surface is:

- BM25 top-k documents.
- Atom/SAE top-k documents.
- Full-score BM25 + `scale * atom` unified top-k documents.
- Train-only dense-teacher top-k documents.
- Train-only qrel positives, preserving the existing M323 no-leakage rule:
  heldout never forces qrel positives.

The scorer input remains runtime-safe: BM25 and atom features only. Dense
teacher candidates and scores are used as training supervision, not as runtime
features.

Source IDs are now bitmasks:

- `1`: BM25 evidence.
- `2`: atom evidence.
- `4`: teacher evidence.
- `0`: forced training-only source.

This lets diagnostics distinguish BM25/atom/teacher overlap without inventing
one-off source categories.

## First Run

Run name:

```text
ii42-m332-natural-union-final-objective-v1
```

Comparable surface:

```text
nfcorpus, scifact, fiqa, arguana, scidocs
```

Base checkpoint:

```text
/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt
```

Teacher rankings:

```text
/home/huoju/leask/runs/ii42-m326-baselines/dense_teacher_seed1050_expansion5_top300.json
```

Candidate settings:

- `candidate_k = 160`
- `unified_candidate_scale = 0.5, 1.0`
- `teacher_candidate_split = train`
- `teacher_candidate_k = 120`
- `max_atom_df_ratio = 0.25`

Training objective:

- listwise qrel target;
- pairwise hard-negative loss;
- dense-teacher KL;
- dense false-positive suppression;
- BM25-supported qrel rescue;
- boundary top-k ranking loss.

The first run uses the M330/M331 scorer knobs plus the natural-union surface.
If it does not beat M326B/M331 on the same five-dataset heldout surface, do not
expand.

## Promotion Criteria

M332 must satisfy all of:

- Beats M326B expansion winner on at least MRR@20, NDCG@10, and MAP@100.
- Does not materially lose Recall@100 versus M326B.
- Improves or holds M331 diagnostics:
  BM25-supported qrel suppression, qrel-low, semantic false positives, and
  teacher top10 qrel loss.
- Candidate cost remains explainable through source/cost statistics.

If it only improves candidate upper bound but not learned ranking, the next
step is not a wider candidate pool. It is a richer listwise/cross-candidate
model.

