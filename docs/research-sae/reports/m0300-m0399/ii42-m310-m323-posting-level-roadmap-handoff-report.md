# ii42 M310-M323 Posting-Level Retrieval Handoff Report

Date: 2026-06-18

## Purpose

This report summarizes the M310 and later posting-level retrieval line so it
can be handed to the parallel research branch without losing the concrete
evidence. The line starts from SAE atoms as latent posting terms, then moves
toward BM25 token postings plus SAE atom postings as one searchable sparse
surface.

The key conclusion is not that this route is solved. The key conclusion is
that the blocker has moved from raw representation to final candidate
admission and top-rank scoring. Static scalar fusion is not enough, but learned
posting-level scoring has clear signal.

## Evidence Sources

Primary local reports:

- `docs/research-sae/reports/m0300-m0399/ii42-m310-dense-gap-full15-current.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m310-surface-taxonomy-and-policy-report.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m310-latent-terms-bm25-canary-report.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m320-dense-assisted-posting-native-plan.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m320-dense-assisted-posting-native-report.md`

Primary scripts:

- `scripts/research_sae_m310_latent_bm25_canary.py`
- `scripts/research_sae_m310_dense_gap_matrix.py`
- `scripts/research_sae_m310_surface_taxonomy.py`
- `scripts/research_sae_m310_policy_sweep.py`
- `scripts/research_sae_m310_policy_oracle.py`
- `scripts/research_sae_m320_dense_assisted_posting_atoms.py`
- `scripts/research_sae_m321_atom_pruning_diagnostic.py`
- `scripts/research_sae_m322_candidate_pool_scorer.py`

Important caveat: the M320-M322 files are currently active research files in
the working tree. They should be treated as evidence for the research line, not
as product code.

## M310: Latent Terms And Fixed Fusion

M310 tested whether SAE atoms can behave like latent BM25 terms. The route was:

```text
text -> SAE atoms -> latent term postings -> IDF/BM25-style score
```

The best M310 full15 result improves over raw SAE and raw posting score, but it
does not close the gap to dense retrieval.

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.7092 | 0.6670 | 0.5874 | 0.4318 |
| SAE | 0.6436 | 0.5979 | 0.5122 | 0.3656 |
| Posting score | 0.6335 | 0.5576 | 0.4773 | 0.3374 |
| Best M310 | 0.6831 | 0.6276 | 0.5477 | 0.3973 |
| Best M310 minus dense | -0.0260 | -0.0394 | -0.0397 | -0.0346 |

M310 won against dense on two datasets:

| Dataset | Delta R@100 | Delta NDCG@10 | Meaning |
| --- | ---: | ---: | --- |
| `scifact` | +0.0133 | +0.0431 | Strong scientific fact-checking win. |
| `webis-touche2020` | +0.0624 | +0.0256 | Argument retrieval benefits from latent terms. |

The worst gaps were concentrated in datasets where semantic coverage and
top-rank calibration remain hard:

| Dataset | Delta R@100 | Delta NDCG@10 |
| --- | ---: | ---: |
| `climate-fever` | -0.0844 | -0.0792 |
| `cqadupstack` | -0.0685 | -0.0522 |
| `dbpedia-entity` | -0.0677 | -0.0638 |
| `fiqa` | -0.0666 | -0.0700 |
| `hotpotqa` | -0.0446 | -0.0667 |

### M310 Root Cause

M310's most important result is diagnostic. On the checked posting surfaces,
candidate generation is not the dominant blocker.

| Scope | Rows | Candidate-hit rows | Candidate-hit rate |
| --- | ---: | ---: | ---: |
| Six-surface taxonomy | 2,521 | 2,437 | 0.9667 |

Dominant failure buckets:

| Cause | Count | Interpretation |
| --- | ---: | --- |
| `hit_but_low_rank` | 483 | Candidate exists, scorer ranks it too low. |
| `relevant_has_both_evidence_but_ranked_out` | 97 | Multiple evidence sources exist, final score loses it. |
| `candidate_miss` | 84 | True candidate-generation miss. |
| `sae_hit_suppressed` | 41 | SAE has the hit, final policy suppresses it. |
| `bm25_hit_suppressed` | 20 | BM25 has the hit, final policy suppresses it. |

So the M310 lesson is:

- latent atom BM25 is useful;
- raw atom score is not enough as final ranking;
- BM25, SAE, and latent-BM25 need to remain separate evidence channels;
- static fixed-weight fusion improves results but cannot fully recover top
  rank quality.

### M310 Fixed Policy Family

The stable policy family from cross-holdout sweeps was:

```text
latent_binary_bm25 weight: 1.0 to 1.25
BM25 weight: 0.75
SAE source weight: 0.5 to 1.0
```

Seven-surface selfcheck:

| Policy | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Best fixed policy | 0.6134 | 0.4984 | 0.3936 | 0.3065 |
| Raw SAE | 0.5931 | 0.4793 | 0.3728 | 0.2888 |
| Raw latent BM25 | 0.5905 | 0.4690 | 0.3653 | 0.2824 |
| BM25 | 0.4660 | 0.3640 | 0.2723 | 0.1981 |

The fixed policy is useful, but the query-level oracle shows there is much more
headroom:

| Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Best fixed policy | 0.6134 | 0.4984 | 0.3936 | 0.3065 |
| Query-level oracle | 0.6330 | 0.5920 | 0.4537 | 0.3533 |
| Oracle gain | +0.0196 | +0.0936 | +0.0602 | +0.0468 |

Low-capacity query selectors and linear candidate scorers did not capture this
oracle headroom. That points toward a richer listwise candidate-pool scorer,
not more manual scalar fusion.

## M320: Posting-Native Atom Encoder

M320 moved from posthoc latent scoring to direct posting-native atom training:

```text
text -> encoder -> atom postings -> BM25 + atom unified retrieval
```

Dense/PPLX is used as a teacher, but the target is retrieval through atom
postings. This is closer to the desired unified sparse engine.

M320 admission training improved atom-BM25 quality sharply, especially on
`scifact`, but created a physical-cost problem.

### M320 Heldout Quality

NFCorpus:

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M1050 atom-BM25 | 0.0705 | 0.0989 | 0.0633 | 0.0277 |
| M320 admission atom-BM25 | 0.1598 | 0.1809 | 0.0960 | 0.0452 |
| M1050 unified scale 0.5 | 0.2611 | 0.4573 | 0.2872 | 0.1301 |
| M320 admission unified scale 0.5 | 0.2698 | 0.4705 | 0.3010 | 0.1403 |

SciFact:

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M1050 atom-BM25 | 0.4689 | 0.2723 | 0.2837 | 0.2748 |
| M320 admission atom-BM25 | 0.6496 | 0.2922 | 0.3177 | 0.2899 |
| M1050 unified scale 0.5 | 0.8552 | 0.6011 | 0.6240 | 0.5938 |
| M320 admission unified scale 0.5 | 0.8463 | 0.5621 | 0.5993 | 0.5516 |

### M320 Physical Cost

| Dataset | Model | Touched Docs | Postings |
| --- | --- | ---: | ---: |
| `nfcorpus` | M1050 | 160.9 | 164.5 |
| `nfcorpus` | M320 admission | 3633.0 | 39532.5 |
| `nfcorpus` | M320 clipped | 3171.4 | 11323.6 |
| `nfcorpus` | M320 df-aware | 2299.2 | 5017.4 |
| `scifact` | M1050 | 246.8 | 270.3 |
| `scifact` | M320 admission | 5183.0 | 87242.1 |
| `scifact` | M320 clipped | 5012.9 | 32977.6 |
| `scifact` | M320 df-aware | 4740.7 | 18940.1 |

Interpretation:

- M320 proves the training signal can create stronger atom postings.
- The strongest atom postings rely on high-DF semantic hubs.
- Hard clipping and generic DF penalties reduce fanout but lose useful signal.
- The next problem is not only representation. It is utility-aware admission
  and final ranking.

## M321: DF And Utility Pruning

M321 froze the M320 admission checkpoint and tested pruning. This changed the
interpretation: high-DF atoms are neither uniformly good nor uniformly bad.

Example:

| Dataset | Variant | Atom R@100 | Atom MRR@20 | Atom Postings | Unified MRR@20 |
| --- | --- | ---: | ---: | ---: | ---: |
| `nfcorpus` | `df<=1.0` | 0.1598 | 0.1809 | 39532.5 | 0.4705 |
| `nfcorpus` | `df<=0.25` | 0.1009 | 0.1128 | 1377.3 | 0.4753 |
| `scifact` | `df<=1.0` | 0.6496 | 0.2922 | 87242.1 | 0.5621 |
| `scifact` | `df<=0.75` | 0.6663 | 0.3206 | 47318.4 | 0.5583 |
| `scifact` | `df<=0.25` | 0.6244 | 0.2675 | 13022.3 | 0.5537 |

Manual utility formulas were not robust:

- v1 selected useful signals but still kept bad hubs.
- v2 punished high DF too strongly and collapsed atom-only retrieval on
  `scifact`.

M321 decision:

- do not scale hand-written utility formulas;
- use pruning diagnostics only as evidence;
- train a learned candidate-pool scorer over BM25 and atom features.

## M322-M323: Learned Candidate-Pool Scoring

M322 froze the M320 admission checkpoint and trained a scorer over the union of
BM25 and atom candidates. Runtime-safe features include normalized BM25 score,
atom score, source flags, rank features, query/doc atom counts, overlap mass,
overlap IDF, and overlap DF bands.

M322 valid heldout aggregate on `nfcorpus + scifact`:

| Variant | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `df<=0.25` | fixed unified | 0.5425 | 0.5130 | 0.4358 | 0.3332 |
| `df<=0.25` | M322 scorer | 0.6090 | 0.6125 | 0.5083 | 0.3934 |
| `df<=0.25` | M322 best | 0.6247 | 0.6145 | 0.5167 | 0.4054 |

M323 expanded the no-leakage scorer canary to `nfcorpus + scifact + fiqa` and
compared against exact dense on the same split.

| Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense exact | 0.7433 | 0.6152 | 0.5330 | 0.4464 |
| `df<=0.25` fixed unified | 0.5374 | 0.4262 | 0.3540 | 0.2815 |
| `df<=0.25` M323 scorer | 0.6588 | 0.5440 | 0.4463 | 0.3587 |
| `df<=0.25` M323 best | 0.6642 | 0.5485 | 0.4536 | 0.3663 |
| `df<=0.5` M323 scorer | 0.6584 | 0.5497 | 0.4513 | 0.3579 |
| `df<=0.5` M323 best | 0.6605 | 0.5517 | 0.4557 | 0.3614 |

M323 clearly beats static additive fusion. It still trails exact dense, so it
is not a product-ready dense replacement.

The candidate upper bound is high:

| Variant | Candidate upper-bound R@100 | Candidate upper-bound NDCG@10 |
| --- | ---: | ---: |
| `df<=0.25` | 0.7225 | 0.8426 |
| `df<=0.5` | not separately promoted | not separately promoted |

This means both candidate admission and final scoring still matter. The scorer
learns real signal, but the admitted candidate pool and scoring objective are
not yet strong enough.

## M324-M325 Notes

The M320 report records two follow-ups:

- M324 tested larger `candidate_k=300`, `hidden_dim=96`, and longer training.
- M325 tested source-balanced hard negatives over BM25-only, atom-only, and
  BM25+atom candidates.

M324 increased candidate upper bound but regressed scorer quality:

| Variant | Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `df<=0.25` | candidate upper bound | 0.7542 | 1.0000 | 0.8698 | 0.7542 |
| `df<=0.25` | M324 scorer | 0.6259 | 0.5206 | 0.4227 | 0.3454 |
| `df<=0.5` | candidate upper bound | 0.7634 | 1.0000 | 0.8761 | 0.7634 |
| `df<=0.5` | M324 scorer | 0.6533 | 0.5144 | 0.4233 | 0.3370 |

The current interpretation is that larger topK alone adds useful positives and
many noisy negatives. It does not solve the ranking problem. Source-balanced
training is the right next diagnostic, but it should be judged against M323
MRR/NDCG/MAP, not just candidate upper bound.

## Comparison With Earlier M1000/M1050 Route

The M1000/M1050 route came from the visual-word paper analogy:

```text
SAE atoms -> posting terms -> BM25/IDF first-stage retrieval
```

It was strong as an indexing concept but weak in atom-only quality.

| Dataset | Model | Atom R@100 | Atom MRR@20 | Atom NDCG@10 | Atom MAP@100 | Postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | M1050 atom-BM25 | 0.0705 | 0.0989 | 0.0633 | 0.0277 | 164.5 |
| `scifact` | M1050 atom-BM25 | 0.4689 | 0.2723 | 0.2837 | 0.2748 | 270.3 |
| `nfcorpus` | M320 admission atom-BM25 | 0.1598 | 0.1809 | 0.0960 | 0.0452 | 39532.5 |
| `scifact` | M320 admission atom-BM25 | 0.6496 | 0.2922 | 0.3177 | 0.2899 | 87242.1 |

Tradeoff:

- M1050 has low fanout and is index-friendly, but quality is too weak.
- M320 has much better atom quality, but fanout is too high.
- M322/M323 show that learned posting-level scoring can recover significant
  top-rank quality from the same evidence.

This suggests the target architecture should combine:

1. M1000/M1050's low-cost posting discipline;
2. M320's stronger admission-oriented atom training;
3. M322/M323's learned final candidate-pool scoring.

## Strategic Interpretation

The most important trend is consistent across M310-M323:

- Static scalar fusion is not enough.
- Candidate generation is often good enough to expose relevant documents.
- High-DF atoms are dangerous but sometimes carry essential semantic signal.
- A learned posting-level scorer improves substantially over fixed fusion.
- The system is not yet dense-equivalent because admission and scoring are both
  still imperfect.

The route is therefore viable as a research direction, but not as a solved
product path.

## Recommended Direction For The Parallel Line

### Keep

- Keep latent atom terms as first-class posting terms.
- Keep BM25 token scores, SAE/atom scores, and latent-BM25 scores as separate
  evidence channels.
- Keep candidate upper bound reporting. It is necessary to separate admission
  failure from scoring failure.
- Keep exact dense same-split baselines for every promotion comparison.
- Keep physical-cost metrics: touched docs, postings/query, candidate count,
  and estimated latency.

### Stop

- Stop manual scalar fusion sweeps as the main route.
- Stop single global DF threshold as a final pruning strategy.
- Stop simple query-level policy selection unless it is only diagnostic.
- Stop increasing candidate_k without a source-aware/listwise scorer.

### Continue

The next high-value version should be a stricter M326-style experiment:

1. Start from the best current M320/M323 evidence surface.
2. Use no qrel leakage on heldout candidates.
3. Build source-balanced candidate pools:
   - BM25-only;
   - atom-only;
   - BM25+atom;
   - dense-hit/BM25-miss;
   - high-BM25 false positives.
4. Train a listwise candidate-pool scorer, not just pairwise or linear scoring.
5. Include atom fanout and DF-band features as inputs, not hard global rules.
6. Evaluate on at least five datasets before full15:
   - `nfcorpus`
   - `scifact`
   - `fiqa`
   - `scidocs`
   - one large QA dataset such as `nq` or `hotpotqa`
7. Promote only if it beats M323 on MRR/NDCG/MAP while keeping postings within
   a defined cost budget.

### Promotion Gate

A successor should not be promoted unless it satisfies all of the following:

- Beats M323 best on same-split `nfcorpus + scifact + fiqa`.
- Beats fixed M310 policy on the same full-corpus surfaces.
- Narrows the dense gap on both ranking and recall metrics.
- Does not rely on full-corpus high-DF atom fanout.
- Reports candidate upper bound and final scorer quality separately.
- Preserves split hygiene and avoids forced heldout qrel positives.

### Product Boundary

This line should not yet claim dense replacement. It can claim:

- SAE atoms can function as posting terms.
- Atom-BM25 and lexical BM25 can share a unified sparse abstraction.
- Learned posting-level scoring is materially better than fixed fusion.

It cannot yet claim:

- full15 dense-equivalent quality;
- stable product-ready fanout;
- a finalized SQL/API contract;
- safe mutable index behavior.

## Short Handoff Summary

M310 established that latent-BM25 atom scoring is useful but below dense. M310
also showed that candidate admission is usually not the main blocker; top-rank
scoring is. M320 proved direct posting-native atom training can make atom-BM25
much stronger, but the useful atoms become too high-DF and expensive. M321
showed hand pruning is too blunt. M322/M323 proved learned candidate-pool
scoring over BM25 and atom posting features works and beats static fusion, but
still trails dense.

The next line should focus on source-balanced listwise admission/ranking over
posting-level features, with explicit fanout constraints. That is the most
promising bridge between the low-cost atom-posting idea and the quality gains
needed to challenge dense retrieval.
