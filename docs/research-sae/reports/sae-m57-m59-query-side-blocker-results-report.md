# SAE M57-M59 Query-Side Blocker Results Report

Date: 2026-05-21

## Summary

M57-M59 attempted to push the current M54/M56 text-to-atoms line toward the
teacher by widening the Stage-B training surface and then isolating query-side
training with fixed teacher document atoms.

The result is clear:

- Wider shared-encoder Stage-B training does not preserve teacher atom shape
  well enough.
- Query-side fixed-doc training is the more useful diagnostic route, but the
  current small token-level encoder still leaves a large teacher gap.
- Full15 qrel supervision alone does not close the gap. More labels on the
  same objective and encoder family are not enough.

The product model therefore remains blocked. The next useful push is a
stronger query encoder plus direct teacher-shape distillation, not another
weight-only sweep over the current shared encoder.

## M57 Shared-Encoder Widening

M57 scaled the M56 safe Stage-B balance from two datasets to four datasets:

```text
datasets = scifact, nfcorpus, fiqa, scidocs
checkpoint = m54-e2-clean
teacher = shared_sae_8192_64
```

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Doc Recall | Query Recall | Neighborhood Recall |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M54 baseline | 0.654611 | 0.621174 | 0.501748 | 0.406679 | 0.336422 | 0.188750 | 0.308500 |
| M57 safe_anchor | 0.663032 | 0.626758 | 0.500917 | 0.406291 | 0.297375 | 0.156875 | 0.293700 |
| M57 shape_lock | 0.660884 | 0.624097 | 0.503866 | 0.407531 | 0.316781 | 0.170000 | 0.298400 |

M57 `shape_lock` is the best shared-encoder point, but it still loses teacher
shape versus M54 baseline. `rank_mid` was stopped after the first two variants
showed the failure mode: stronger ranking pressure is unlikely to preserve
semantic atom geometry.

Decision: do not continue shared-encoder widening until the training path can
cache the expensive auxiliary examples and isolate document/query failure
modes.

## M58 Fixed-Doc Query-Side Training

M58 keeps document atoms fixed to teacher document atoms and trains only query
text to query atoms. This isolates the query-side blocker.

Four-dataset development matrix:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Gap R | Gap MRR | Gap N | Gap MAP | Candidate Docs | SAE Postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.636854 | 0.622651 | 0.495268 | 0.400674 | - | - | - | - | - | - |
| Teacher fixed-doc | 0.721790 | 0.696360 | 0.575091 | 0.484613 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | - | - |
| plain-e2 | 0.668898 | 0.647048 | 0.515504 | 0.419050 | -0.052892 | -0.049312 | -0.059587 | -0.065563 | 1879.5 | 3229.5 |
| style-e2 | 0.673422 | 0.641930 | 0.518337 | 0.419551 | -0.048368 | -0.054429 | -0.056754 | -0.065062 | 1869.0 | 3156.6 |
| query-prefix-e2 | 0.670250 | 0.640562 | 0.515850 | 0.420349 | -0.051540 | -0.055798 | -0.059241 | -0.064264 | 1868.1 | 3022.6 |
| style+query-e2 | 0.663483 | 0.643442 | 0.515750 | 0.419954 | -0.058307 | -0.052917 | -0.059341 | -0.064659 | 1862.5 | 2974.3 |
| style-e4 | 0.668022 | 0.646145 | 0.519723 | 0.421391 | -0.053768 | -0.050215 | -0.055368 | -0.063222 | 1881.6 | 3324.1 |
| style-e8 | 0.673600 | 0.642923 | 0.518106 | 0.422643 | -0.048191 | -0.053437 | -0.056985 | -0.061970 | 1882.7 | 3413.2 |
| teacher-heavy-e4 | 0.666590 | 0.640499 | 0.515603 | 0.418264 | -0.055200 | -0.055861 | -0.059488 | -0.066349 | 1889.7 | 3404.5 |
| qrel-residual-e4 | 0.669853 | 0.640854 | 0.517786 | 0.421633 | -0.051937 | -0.055506 | -0.057305 | -0.062980 | 1843.6 | 3063.6 |

The best development signal is split:

- `style-e8` gives the best Recall@100 and MAP@100.
- `style-e4` gives the best NDCG@10.
- qrel residual improves cost and MAP slightly, but does not solve the teacher
  gap.
- teacher-heavy weighting regresses, so the current bottleneck is not solved
  by simply increasing teacher KL/support/value weights.

Decision: query-side fixed-doc training is the right diagnostic harness, but
the current encoder/objective still cannot approach teacher quality.

## M58 Full15 Generalization Check

The best M58 configuration was retrained on the four development datasets and
evaluated on all 15 BEIR datasets.

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.783775 | 0.786255 | 0.667518 | 0.625401 |
| M58 style-e8 fixed-doc | 0.802138 | 0.800114 | 0.685109 | 0.647001 |
| Teacher fixed-doc | 0.844716 | 0.839950 | 0.749006 | 0.724736 |

Full15 teacher gap:

| Metric | Gap |
| --- | ---: |
| Recall@100 | -0.042579 |
| MRR@20 | -0.039836 |
| NDCG@10 | -0.063898 |
| MAP@100 | -0.077735 |

Worst full15 collapses versus teacher:

| Dataset | Metric | Delta |
| --- | --- | ---: |
| `arguana` | NDCG@10 | -0.076419 |
| `cqadupstack` | NDCG@10 | -0.140129 |
| `cqadupstack` | MAP@100 | -0.160280 |
| `dbpedia-entity` | MAP@100 | -0.103881 |
| `fiqa` | NDCG@10 | -0.169178 |

Decision: four-dataset query-side training improves over BM25 but does not
generalize near the teacher.

## M59 Full15 Supervised Query-Side Check

M59 tested whether the problem is simply too little supervised training data.
The same fixed-doc query-side path was trained and evaluated on all 15 BEIR
datasets.

```text
train examples = 1342
epochs = 4
query_style_prefixes = true
```

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.783775 | 0.786255 | 0.667518 | 0.625401 |
| M59 full15 supervised | 0.798010 | 0.801798 | 0.683840 | 0.645672 |
| Teacher fixed-doc | 0.844716 | 0.839950 | 0.749006 | 0.724736 |

M59 full15 teacher gap:

| Metric | Gap |
| --- | ---: |
| Recall@100 | -0.046707 |
| MRR@20 | -0.038152 |
| NDCG@10 | -0.065167 |
| MAP@100 | -0.079064 |

M59 does not beat the M58 full15 generalization row. It slightly improves
MRR@20 but worsens Recall@100, NDCG@10, and MAP@100.

Decision: more BEIR qrel supervision on the same model family is not enough.

## Current Blocker

The current student can improve BM25, but it cannot preserve enough of the
teacher semantic neighborhood. The failure is not just:

- missing query prefix;
- scalar SAE weight selection;
- too few epochs;
- too little BEIR supervised data;
- teacher KL/support/value weights.

The likely blocker is representation capacity and teacher-shape retention on
the query side. Query-side fixed-doc training shows the cleanest gap because
document atoms are already teacher atoms.

## Next Direction

The next attempt should be M60:

1. Keep the fixed-doc query-side harness.
2. Train a stronger query encoder, not a shared doc/query encoder.
3. Add a teacher-shape stage before ranking:
   - support top-k recall;
   - value calibration on active atoms;
   - teacher-neighborhood KL;
   - fanout-aware active atom budget.
4. Then add qrel/listwise ranking as a second stage, not as the first dominant
   signal.
5. Evaluate with full15 from the start, and use development subsets only for
   smoke tests.

Promotion requires closing the full15 fixed-doc teacher gap before returning to
document-side text-to-atoms or SQL/runtime engineering.
