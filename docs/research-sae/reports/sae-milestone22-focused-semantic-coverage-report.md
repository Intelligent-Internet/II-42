# SAE Milestone 22 Focused Semantic Coverage Report

## Scope

The previous full15 taxonomy showed that the main remaining gap is
`student_encoder_loss`, not native traversal loss. This pass tests the smallest
possible intervention:

- keep the current `token_lse + token_char + asym96 + budget8` training shape;
- repeat auxiliary training examples for taxonomy `student_encoder_loss`
  queries;
- keep scoring and native traversal unchanged;
- compare against a same-teacher, same-config baseline.

This is a training-pressure test, not a new product candidate.

## Rebuilt Teacher Caveat

The original `shared_sae_8192_64` artifact was lost with the old `/tmp` working
root. The final reports/checkpoints were safe, but the reusable teacher latents
had to be rebuilt under:

```text
/Volumes/Betty/Tmp/ii42_sae_beir15_shared
```

No local copy of the old 10k commons unlabeled dump was found, so this rebuild
uses the 15 BEIR qrels-preserving document samples only:

| Field | Value |
| --- | ---: |
| Training records | 49,059 |
| Unlabeled records | 0 |
| Latent dims | 8,192 |
| Active dims | 64 |
| Build elapsed | 69.43 s |

Because of this, the meaningful comparison in this report is `focus6` versus
`beironly-baseline`, not either run versus the older `/tmp` checkpoint.

## Implementation

`scripts/research_sae_text_atom_miss_taxonomy.py` now writes complete
per-query `query_rows`, including category and slice labels.

`scripts/research_sae_text_atom_train.py` now accepts:

```text
--focus-taxonomy-json
--focus-categories
--focus-example-repeat
```

When enabled, matching query IDs are repeated in retrieval/listwise/coverage
auxiliary examples. The core teacher distillation rows are not duplicated. This
keeps the experiment scoped to retrieval pressure rather than changing the
teacher target distribution.

The focus set was:

| Dataset | Focused `student_encoder_loss` queries |
| --- | ---: |
| `arguana` | 1 |
| `climate-fever` | 2 |
| `cqadupstack` | 9 |
| `fiqa` | 8 |
| `nfcorpus` | 4 |
| `nq` | 2 |
| `scidocs` | 2 |
| `scifact` | 2 |
| **Total** | **30** |

Only 17 of these are in the five-dataset training subset; the remaining cases
are held-out full15 diagnostics.

## Training Runs

Both runs use five training datasets and full15 evaluation:

```text
train = scifact, scidocs, nfcorpus, arguana, fiqa
eval  = full15 BEIR matrix
```

The focused run uses `--focus-example-repeat 6`.

| Run | Retrieval pairs | Coverage examples | Train elapsed |
| --- | ---: | ---: | ---: |
| `beironly-baseline` | 1,475 | 497 | 989.62 s |
| `focus6` | 1,610 | 582 | 1,140.50 s |

Training loss shape:

| Run | Final retrieval loss | Final budget loss | Final asym loss |
| --- | ---: | ---: | ---: |
| `beironly-baseline` | 0.1684 | 2.7314 | 2.9124 |
| `focus6` | 0.1317 | 2.4443 | 2.4133 |

The focused repeat clearly increases useful retrieval pressure and reduces the
budget/asym auxiliary losses. The question is whether that pressure survives
as full15 quality and physical cost.

## Full15 Quality

Best student-weight results:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `beironly-baseline` | 0.7867 | 0.7899 | 0.6699 | 0.6282 |
| `focus6` | 0.7865 | 0.7940 | 0.6734 | 0.6299 |
| Delta | -0.0003 | +0.0041 | +0.0035 | +0.0017 |

Interpretation:

- Focused repeat gives a real but small ranking improvement.
- Recall is flat.
- The best weight remains low (`0.25` or `0.5`), which means the student atom
  signal is still too noisy to increase confidently.

## Miss Taxonomy

| Category | Baseline | Focus6 | Delta |
| --- | ---: | ---: | ---: |
| `covered` | 1,275 | 1,280 | +5 |
| `student_encoder_loss` | 37 | 30 | -7 |
| `semantic_recovered` | 8 | 7 | -1 |
| `native_traversal_loss` | 5 | 4 | -1 |
| `teacher_or_qrels_hard_miss` | 14 | 16 | +2 |
| `lexical_regression` | 0 | 1 | +1 |

Slice-specific signal:

| Slice | Baseline `student_encoder_loss` | Focus6 `student_encoder_loss` |
| --- | ---: | ---: |
| `semantic_heavy` | 31 | 26 |
| `long_query` | 26 | 20 |

This confirms the focus mechanism targets the intended failures. It does not
solve them completely, and it creates small regressions elsewhere.

## Physical Active8 Cost

Native C payload benchmark, `sae_active_dims=8`,
`sae_postings_per_dim=128`, `bm25_candidate_k=200`,
`bm25_postings_per_term=64`:

| Run | Recall@100 | NDCG@10 | MAP@100 | Candidates | SAE posts | BM25 posts | Payload MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `beironly-baseline` | 0.7731 | 0.6575 | 0.6052 | 670.3 | 767.3 | 677.5 | 9.19 |
| `focus6` | 0.7761 | 0.6589 | 0.6023 | 768.1 | 809.6 | 677.5 | 9.26 |

Physical interpretation:

- Focus6 opens about 98 more candidate docs per query.
- SAE postings rise by about 42 per query.
- Payload size is slightly larger.
- MAP drops in the physical active8 path.

This fails the promotion rule: ranking gains are too small and physical cost
moves in the wrong direction.

## Conclusion

`focus6` is a useful negative/diagnostic result:

- It proves the taxonomy is actionable: targeted repeat reduces
  `student_encoder_loss`.
- It is too blunt: the same repeat increases posting fanout and candidate
  volume.
- It should not become the next checkpoint.

The next training step should not be another raw repeat sweep. It should use
the focused queries inside a teacher-aware objective that explicitly rewards
covering dense-positive/qrel-positive neighborhoods under a fixed active-query
budget and penalizes extra non-safe exposure.

Recommended next run:

```text
focus taxonomy + listwise dense-teacher candidate-budget loss
+ teacher-aware safe exposure
+ fixed doc88/query96 validation
+ full15 active8 cost gate
```

Acceptance criteria stay unchanged:

- NDCG/MAP must not regress.
- `student_encoder_loss` must decrease.
- active8 candidate docs and SAE postings must not increase materially.
