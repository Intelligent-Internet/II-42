# SAE M112-M115 Fanout-Aware Training Results Report

Date: 2026-05-23

Superseded by:

```text
sae-m116-m122-sota-push-results-report.md
```

M112-M115 identify the fanout blocker and promote M114 as the first useful
background-score checkpoint. M116-M122 continue from that result and promote
M121/M122 as the first overlap-constrained quality/cost improvement.

## Decision

M112-M115 validate the next constraint for the DiffSAE-aligned route:
candidate-set quality is no longer sufficient. A checkpoint must also control
full-corpus posting fanout.

The useful result is M114:

- M112 proves background-margin training can nearly match dense on the refreshed
  candidate surface, but it opens almost the whole full corpus.
- M114 proves a hard background-score penalty can reduce full-corpus postings
  below the M109 baseline while keeping BM25+SAE fusion competitive.
- M113 and M115 are not promoted. M113 was stopped early because the weak
  penalty increased background score mass. M115 was stopped early because the
  middle penalty was less stable than M114 after straight-through training.

The next step should not be another candidate-only fine-tune. It should keep
M114's absolute background score control and recover ranking quality with a
better two-phase or multi-objective schedule.

## Code Changes

Updated:

```text
scripts/research_sae_m111_full_corpus_hard_negative_refresh.py
scripts/research_sae_m112_background_fanout_train.py
```

Changes:

- M111 refresh now accepts `--split`, so eval roots are no longer mislabeled as
  train rows.
- M112 introduces background negatives sampled from the root document corpus.
- M112/M114 training records background diagnostics: background-above-positive
  rate, background top-k intrusion count, and background score mass.
- The training objective now supports:
  - `loss_background_margin`
  - `loss_background_score`
  - `loss_background_overlap`

The important correction is `loss_background_score`: margin-only training lets
background documents stay below positives while still carrying high absolute
scores. Those scores open too many postings in full-corpus retrieval.

## Runs

All runs used the M111 refreshed candidate roots:

```text
/home/huoju/leask/runs/m111-full-corpus-hard-negatives/m111_m107_train_refresh
/home/huoju/leask/runs/m111-full-corpus-hard-negatives/m111_m97_eval_refresh_splitfix
```

The eval refresh was regenerated with:

```text
--split eval
```

### Candidate-Surface Results

| Run | Best Step | Main Change | hit@10 | MRR@10 | Background Top20 | Background Score Mean |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| M112 | 1600 | margin-only background negatives | 0.8603 | 0.6758 | 2.1504 | not logged |
| M114 | 700 | hard background score penalty | 0.8032 | 0.6179 | 1.5742 | 0.7522 |
| M115 partial | 100 | softer background score penalty | 0.7849 | 0.6096 | 1.2227 | 1.1757 |

Interpretation:

- M112 is the best candidate-set model, but it is not index-safe.
- M114 sacrifices candidate-set ranking to control background score mass.
- M115 did not become a useful midpoint. After straight-through training its
  selection metric degraded, so it was stopped before full-corpus eval.

## Full-Corpus Index Results

Evaluation corpus:

```text
/home/huoju/leask/runs/m97-stage-a-validation-v2/m21_payload_root/m97_stage_a_eval
```

The full-corpus evaluator is:

```text
scripts/research_sae_m110_full_corpus_index_eval.py
```

| Checkpoint | SAE Postings / Query | SAE Accumulators / Query | SAE Recall@100 | SAE MRR@10 | Fusion Recall@100 | Fusion MRR@10 | Fusion NDCG@10 | Fusion MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M109 8192/k64 | 60,370 | 23,052 | 0.3647 | 0.4103 | 0.4241 | 0.4907 | 0.3634 | 0.2344 |
| M111 refresh | 78,156 | 25,456 | 0.3247 | 0.3462 | 0.4109 | 0.4656 | 0.3432 | 0.2160 |
| M112 margin-only | 208,302 | 25,863 | 0.3065 | 0.3551 | 0.4022 | 0.4756 | 0.3427 | 0.2177 |
| M114 hard-bgscore | 45,097 | 19,296 | 0.3647 | 0.3983 | 0.4258 | 0.5004 | 0.3625 | 0.2311 |

M114 is the first fanout-aware point that improves both:

- cost versus M109: postings drop from about `60k` to `45k`, and accumulators
  drop from about `23k` to `19k`;
- BM25+SAE fusion recall/MRR versus M109: Recall@100 `0.4258` vs `0.4241`,
  MRR@10 `0.5004` vs `0.4907`.

It does not fully replace M109 because NDCG@10 and MAP@100 are still slightly
lower. This means the route is promising but not finished.

## Interpretation

The experiments separate three effects:

1. Candidate-set ranking can be made strong.
2. Candidate-set ranking can silently create high-DF sparse atoms.
3. Absolute background score control is an effective proxy for full-corpus
   posting fanout.

M112 is the failure case: it has strong candidate-set metrics but opens nearly
all documents in the M97 eval corpus. M114 is the useful counterexample: it has
weaker candidate metrics but produces a lower-cost full-corpus index and a
slightly better BM25+SAE fusion MRR.

The current blocker is therefore no longer "can DiffSAE train." It can. The
blocker is ranking recovery under a fanout-constrained objective.

## Next Step

The next run should keep the M114 hard background-score signal, but change the
training schedule:

1. Start with the M112/M109 candidate-rank objective to preserve ranking.
2. Introduce the M114 background-score loss gradually instead of from step 1.
3. Select checkpoints on full-corpus M110 metrics or a cheaper proxy that
   explicitly includes background score mass.
4. Keep `background_k=160` or higher for diagnostics, but consider a smaller
   training `background_k` with a larger eval background sample for speed.

Promotion gate for the next checkpoint:

- postings/query no higher than M109 `8192/k64`;
- fusion NDCG@10 and MAP@100 no lower than M109;
- fusion Recall@100 and MRR@10 at least matching M114;
- SAE alone should not regress below M114.
