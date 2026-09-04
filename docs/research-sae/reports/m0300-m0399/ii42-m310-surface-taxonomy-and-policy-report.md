# ii42 M310 Surface Taxonomy And Policy Report

Date: 2026-06-18

## Status

M310 is still below the full15 dense baseline, but the failure mode is now
clearer. The primary blocker is not raw candidate admission. On the available
M310 posting surfaces, relevant documents usually enter the candidate pool, but
the final scorer fails to keep them high enough.

The current full15 dense gap snapshot is:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.7092 | 0.6670 | 0.5874 | 0.4318 |
| Best M310 | 0.6831 | 0.6276 | 0.5477 | 0.3973 |
| Gap | -0.0260 | -0.0394 | -0.0397 | -0.0346 |
| Raw SAE | 0.6436 | 0.5979 | 0.5122 | 0.3656 |
| Raw posting score | 0.6335 | 0.5576 | 0.4773 | 0.3374 |

Best M310 is meaningfully better than raw SAE and raw posting score, but still
does not close the dense ranking gap.

## Evidence Surfaces

The root-cause pass used preserved M310B posting surfaces:

```text
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1/fiqa_all_official_clean/cache_hit
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1/scidocs_all_official/cache_hit
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1/scifact_all_official_clean/cache_hit
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1/nfcorpus_all_official_clean/cache_hit
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1/trec_covid_all_official_clean/cache_hit
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1/nq_q200_official/cache_hit
```

Local copies of generated reports are under:

```text
/Volumes/Betty/Tmp/ii42-m310-surface-taxonomy-v1
/Volumes/Betty/Tmp/ii42-m310-policy-sweep-v1
```

Scripts:

```text
scripts/research_sae_m310_surface_taxonomy.py
scripts/research_sae_m310_policy_sweep.py
scripts/research_sae_m310_latent_bm25_canary.py
```

## Root Cause

Across the six checked surfaces:

| Scope | Rows | Candidate-hit rows | Candidate-hit rate |
| --- | ---: | ---: | ---: |
| Six-surface taxonomy | 2,521 | 2,437 | 0.9667 |

The focus-policy failure buckets were:

| Cause | Count | Interpretation |
| --- | ---: | --- |
| `hit_top_rank` | 1,776 | Already solved by current policy. |
| `hit_but_low_rank` | 483 | Candidate exists, but scorer ranks it too low. |
| `relevant_has_both_evidence_but_ranked_out` | 97 | Evidence exists in multiple sources, but final score loses it. |
| `candidate_miss` | 84 | True candidate-generation miss. |
| `sae_hit_suppressed` | 41 | SAE has the hit, final policy suppresses it. |
| `bm25_hit_suppressed` | 20 | BM25 has the hit, final policy suppresses it. |
| `sae_evidence_ranked_out` | 10 | SAE evidence exists but not in top rank. |
| `latent_hit_suppressed` | 9 | Latent score has the hit, final policy suppresses it. |
| `bm25_evidence_ranked_out` | 1 | BM25 evidence exists but not in top rank. |

The exact conclusion is:

- Candidate generation is not the dominant blocker on these surfaces.
- The dominant loss is top-rank scoring/admission.
- Raw `posting_score` is not strong enough as a final score.
- Latent BM25-style atom scoring is useful, but it needs BM25 and raw SAE source
  scores kept as separate normalized evidence channels.

## Per-Surface Snapshot

| Surface | Rows | Candidate-hit rate | Focus policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: |
| `fiqa` | 648 | 0.9614 | `bm25 + latent w0.35` | 0.7623 | 0.5250 | 0.4465 | 0.3850 |
| `scidocs` | 1,000 | 0.9670 | `bm25 + latent w0.50` | 0.4565 | 0.3601 | 0.2054 | 0.1442 |
| `scifact` | 300 | 1.0000 | `bm25 + latent w0.35` | 0.9767 | 0.7608 | 0.7910 | 0.7485 |
| `nfcorpus` | 323 | 0.9288 | `bm25 + latent w0.50` | 0.3197 | 0.5398 | 0.3419 | 0.1604 |
| `trec-covid` | 50 | 1.0000 | `bm25 + latent w0.35` | 0.1558 | 0.9500 | 0.8500 | 0.1235 |
| `nq_q200` | 200 | 0.9850 | `bm25 + latent w0.35` | 0.9425 | 0.4753 | 0.5113 | 0.4595 |

`trec-covid` has low Recall@100 because each query has many relevant documents;
top-rank metrics are already strong. It should not drive representation changes.

## Policy Sweep

The targeted adjustment tested runtime-safe fixed policies over the same
candidate surfaces. Each policy combines normalized source scores:

```text
score = latent_weight * latent_binary_bm25
      + bm25_weight * bm25
      + sae_weight * sae
```

### Train FIQA/SciDocs/SciFact/NFCorpus, Validate NQ/TREC

Best train policy:

```text
lin_latent1_bm250p75_sae1
```

| Split | Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| Train baseline `sae` | `sae` | 0.5785 | 0.4722 | 0.3551 | 0.2829 |
| Train baseline `latent` | `latent_binary_bm25` | 0.5735 | 0.4618 | 0.3475 | 0.2760 |
| Train best | `latent1 + bm25.75 + sae1` | 0.5963 | 0.4900 | 0.3748 | 0.2995 |
| Validation baseline `sae` | `sae` | 0.7578 | 0.5516 | 0.5607 | 0.3728 |
| Validation best-train | `latent1 + bm25.75 + sae1` | 0.7857 | 0.5720 | 0.5826 | 0.3946 |

### Train NQ/TREC, Validate FIQA/SciDocs/SciFact/NFCorpus

Best train policy:

```text
lin_latent1p25_bm250p75_sae0p5
```

| Split | Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| Train baseline `sae` | `sae` | 0.7578 | 0.5516 | 0.5607 | 0.3728 |
| Train baseline `latent` | `latent_binary_bm25` | 0.7722 | 0.5421 | 0.5529 | 0.3699 |
| Train best | `latent1.25 + bm25.75 + sae.5` | 0.7857 | 0.5716 | 0.5844 | 0.3937 |
| Validation baseline `sae` | `sae` | 0.5785 | 0.4722 | 0.3551 | 0.2829 |
| Validation best-validation | `latent1 + bm25.75 + sae1` | 0.5963 | 0.4900 | 0.3748 | 0.2995 |

Both directions converge to the same policy family:

```text
latent_binary_bm25 weight: 1.0 to 1.25
BM25 weight: 0.75
SAE source weight: 0.5 to 1.0
```

This is stronger evidence than a single-dataset tuned profile, because the
policy survives a reverse holdout split.

### Seven-Surface Candidate Selfcheck

After narrowing the grid to the cross-holdout policy family, the available full
surfaces were evaluated together. The first pass used six surfaces. A follow-up
run added `webis-touche2020`.

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Objective |
| --- | ---: | ---: | ---: | ---: | ---: |
| `lin_latent1_bm250p75_sae1` | 0.6134 | 0.4984 | 0.3936 | 0.3065 | 0.8248 |
| `lin_latent1p25_bm250p75_sae0p5` | 0.6133 | 0.4974 | 0.3922 | 0.3051 | 0.8224 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.6098 | 0.4927 | 0.3887 | 0.3009 | 0.8148 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.6095 | 0.4916 | 0.3884 | 0.3017 | 0.8145 |
| `sae` | 0.5931 | 0.4793 | 0.3728 | 0.2888 | 0.7853 |
| `latent_binary_bm25` | 0.5905 | 0.4690 | 0.3653 | 0.2824 | 0.7714 |
| `bm25` | 0.4660 | 0.3640 | 0.2723 | 0.1981 | 0.5788 |

The best fixed policy improves over raw SAE by `+0.0203` Recall@100,
`+0.0191` MRR@20, `+0.0208` NDCG@10, and `+0.0177` MAP@100 on this
seven-surface selfcheck.

### Per-Surface Delta

Compared with the best old M310 policy on the same surface:

| Surface | Best old policy | Best new policy | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| `fiqa` | `bm25+latent w0.35` | `latent1 + bm25.75 + sae1` | +0.0022 | +0.0065 | +0.0069 | +0.0074 |
| `nfcorpus` | `bm25+latent w0.50` | `latent1.25 + bm25.75 + sae.5` | +0.0049 | -0.0019 | -0.0007 | -0.0003 |
| `nq_q200` | `bm25+latent w0.35` | `latent1.25 + bm25.75 + sae.5` | +0.0000 | +0.0013 | +0.0053 | +0.0008 |
| `scidocs` | `bm25+latent w0.50` | `latent1 + bm25.75 + sae1` | +0.0022 | +0.0016 | +0.0021 | +0.0019 |
| `scifact` | `bm25+latent w0.35` | `latent1 + bm25.75 + sae1` | +0.0093 | +0.0133 | +0.0099 | +0.0136 |
| `trec-covid` | `bm25+latent w0.35` | `latent1 + bm25.75 + sae1` | +0.0025 | +0.0017 | +0.0113 | +0.0036 |
| `webis-touche2020` | `bm25+latent w0.50` | `latent1.25 + bm25.75 + sae.5` | -0.0132 | -0.0052 | -0.0074 | -0.0060 |
| `dbpedia shard_0000` | `bm25+latent w0.50` | `latent1.25 + bm25.75 + sae.5` | -0.0070 | +0.0040 | -0.0027 | -0.0029 |

The new fixed policy family is useful but not universally dominant. Webis and
the DBpedia shard still prefer the old BM25+latent profile.

## Policy-Selection Headroom

A query-level oracle over the policy set gives the upper bound for an adaptive
selector. This uses qrels only for analysis; it is not a deployable policy.

Seven full surfaces:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Best fixed policy | 0.6134 | 0.4984 | 0.3936 | 0.3065 |
| Query-level oracle | 0.6330 | 0.5920 | 0.4537 | 0.3533 |
| Oracle gain | +0.0196 | +0.0936 | +0.0602 | +0.0468 |

Oracle winner counts:

| Policy | Query count |
| --- | ---: |
| `bm25` | 804 |
| `sae` | 758 |
| `latent_binary_bm25` | 307 |
| `bm25+latent w0.50` | 224 |
| `latent1 + bm25.75 + sae1` | 224 |
| `bm25+latent w0.35` | 186 |
| `latent1.25 + bm25.75 + sae.5` | 67 |

DBpedia shard:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Best fixed policy | 0.4738 | 0.6769 | 0.4148 | 0.2389 |
| Query-level oracle | 0.5161 | 0.7941 | 0.4909 | 0.2870 |
| Oracle gain | +0.0422 | +0.1171 | +0.0761 | +0.0481 |

This is enough headroom to justify a learned runtime-safe selector/scorer.
Further manual fixed-weight tuning is lower value.

## Learned Selector / Scorer Canary

Two low-capacity learned canaries were tested:

- `scripts/research_sae_m310_policy_selector.py`
  - query-level softmax selector over fixed policies;
  - features are runtime-safe query/candidate source statistics;
  - no dataset id.
- `scripts/research_sae_m310_candidate_scorer.py`
  - candidate-level linear logistic scorer;
  - features are normalized BM25/SAE/latent/posting scores, ranks, and
    interaction terms.

Cross-holdout result:

| Run | Best fixed objective | Learned objective | Verdict |
| --- | ---: | ---: | --- |
| selector `train4_val3` | 1.0456 | 0.9868 | worse than fixed |
| selector `train3_val4` | 0.7961 | 0.7475 | worse than fixed |
| candidate scorer `train4_val3` | 1.0456 | 0.9868 | worse than fixed |
| candidate scorer `train3_val4` | 0.7961 | 0.7237 | worse than fixed |

The simple learned routes do not capture the oracle headroom. The likely reason
is that the decision is not just query-level source selection or a linear
candidate score. The useful headroom probably requires listwise supervision or
stronger cross-candidate features.

Current decision:

- keep `lin_latent1_bm250p75_sae1` and
  `lin_latent1p25_bm250p75_sae0p5` in the evaluator;
- do not promote the current learned selector/scorer;
- do not spend more time on low-capacity query-level selection;
- if learned scoring continues, use a listwise/ranking objective over fuller
  official/sharded surfaces.

## Current Decision

Do not restart Stage A or train a new atom model based on this evidence.

The first targeted adjustment has been landed in the M310 evaluator:

```text
lin_latent1_bm250p75_sae1
lin_latent1p25_bm250p75_sae0p5
```

Smoke validation on the existing NFCorpus surface passed. The new evaluator
metrics were:

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `sae` | 0.3045 | 0.5253 | 0.3167 | 0.1404 |
| `latent_binary_bm25` | 0.3009 | 0.5035 | 0.3045 | 0.1364 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.3197 | 0.5398 | 0.3419 | 0.1604 |
| `lin_latent1_bm250p75_sae1` | 0.3228 | 0.5398 | 0.3389 | 0.1581 |
| `lin_latent1p25_bm250p75_sae0p5` | 0.3246 | 0.5379 | 0.3413 | 0.1601 |

This is a controlled scoring-profile adjustment, not a new model claim.

The next targeted adjustment sequence should be:

1. Promote the `latent + BM25 + SAE` fixed policy family into the M310 evaluator
   as an explicit candidate policy. This is now implemented.
2. Run official full-matrix evaluation where preserved surfaces exist.
3. Regenerate missing posting surfaces with `KEEP_SURFACES=1` or shard-level
   policy evaluation for large datasets.
4. If the fixed policy closes a meaningful part of the dense gap, freeze it as
   the M310B default scoring profile.
5. If the fixed policy improves but remains short, train a small runtime-safe
   scorer using the same three normalized evidence channels plus rank/fanout
   features. That should be the next training surface, not another atom encoder
   restart.

## Open Items

- Complete preserved full surfaces currently exist for `nfcorpus`, `scifact`,
  `scidocs`, `fiqa`, `trec-covid`, and `webis-touche2020`.
- Partial surfaces currently exist for `nq` (`q200`) and `dbpedia-entity`
  (`shard_0000`).
- Large preserved full surfaces are still missing for `arguana`, `cqadupstack`,
  `quora`, `hotpotqa`, `fever`, `climate-fever`, and `msmarco`.
- The highest-priority missing gap datasets are `climate-fever`,
  `cqadupstack`, `hotpotqa`, `fever`, and `msmarco`.
- Full15 promotion must wait for those surfaces or for an equivalent
  shard-level evaluator that does not require materializing every surface in one
  file.
- The local BEIR15 PostgreSQL database is now available for exact examples and
  BM25/dense verification, but the M310 SAE policy still depends on M310 posting
  surfaces until the SAE payload is queryable inside the database.
