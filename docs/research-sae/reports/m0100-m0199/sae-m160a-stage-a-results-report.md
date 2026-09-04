# M160A Broad Neutral Stage-A Results Report

Date: 2026-06-02

## Scope

M160A is a corrective Stage-A dense-distillation run. It starts from random
initialization and uses a broad source-balanced PPLX manifest instead of the
BEIR-only M150 Stage-A execution surface.

This report records the completed primary run. It is a Stage-A milestone, not
a BM25+SAE product promotion result.

## Primary Run

```text
run_name: bm25sae-m160a-pplx16384k96-stagea-v1
host: spark / huoju@100.123.2.95
output: /home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-v1
manifest: /home/huoju/leask/runs/m160a-broad-neutral-manifest
clearml_project: ii42_sae/M160
```

Final training state:

| Metric | Value |
| --- | ---: |
| Steps | `269,040` |
| Epochs | `2` |
| Examples | `103,311,067` |
| Final mode | `straight_through` |
| Final tau | `0.35` |
| Final loss | `0.06669` |
| Final neighbor overlap | `0.8534` |
| Elapsed | `29,119 s` |
| Throughput | `9.24 steps/s` |

Artifacts:

| Artifact | Path |
| --- | --- |
| latest checkpoint | `/home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-v1/bm25sae_stagea_latest.pt` |
| objective-selected checkpoint | `/home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-v1/bm25sae_stagea_best.pt` |
| summary | `/home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-v1/bm25sae_stagea_summary.json` |
| events | `/home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-v1/events.jsonl` |

## Data Actually Used

The final summary confirms the intended broad neutral surface was used. Total
source-count examples include repeated exposure from the source-weighted
manifest.

| Source group | Examples |
| --- | ---: |
| BEIR corpus/query examples | `75,057,816` |
| Neutral Wikipedia examples | `9,600,000` |
| Neutral arXiv examples | `9,560,512` |
| Neutral PubMed examples | `9,531,648` |
| Other neutral/proxy examples | `2,820,078` |

The important correction versus M150 is that Wikipedia, arXiv, and PubMed were
not just planned; they were actually consumed by the training loop.

## Candidate Eval Readout

The candidate eval is a small continuity surface. It is useful for trend
diagnosis but is not the final Stage-A gate.

Final checkpoint:

| Row | Hit@10 | Hit@20 | MRR@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | `0.5384` | `0.6061` | `0.3903` | `0.3951` |
| Dense | `0.6298` | `0.6817` | `0.4639` | `0.4677` |
| M160A latest | `0.5926` | `0.6648` | `0.4354` | `0.4400` |

Best observed candidate-ranking point during the run:

| Step | Hit@20 | MRR@10 | MRR@20 | Mode |
| ---: | ---: | ---: | ---: | --- |
| `257250` | `0.6749` | `0.4437` | `0.4483` | `straight_through` |

Best observed candidate-admission point:

| Step | Hit@20 | MRR@10 | MRR@20 | Mode |
| ---: | ---: | ---: | ---: | --- |
| `250` | `0.7201` | `0.4252` | `0.4314` | `soft` |

## Interpretation

M160A clearly beats BM25 on candidate admission and ranking, and it narrows the
gap to dense. The final checkpoint is still not dense-equivalent:

| Metric | Dense | M160A latest | Gap |
| --- | ---: | ---: | ---: |
| Hit@20 | `0.6817` | `0.6648` | `-0.0169` |
| MRR@10 | `0.4639` | `0.4354` | `-0.0285` |
| MRR@20 | `0.4677` | `0.4400` | `-0.0277` |

The late `straight_through` phase improved top-rank metrics. The last 50k
steps still showed ranking gains, so the run does not look fully saturated.
However, candidate admission and top-rank ranking are not optimized by the same
checkpoint: early soft checkpoints had stronger Hit@20, while late
straight-through checkpoints had stronger MRR.

## Decision

Primary M160A is saved as a milestone, but Stage A is not closed.

Next action:

1. Start an explicit continuation from `bm25sae_stagea_latest.pt`.
2. Run a bounded `30k-50k` step continuation under a new run name.
3. Stop if MRR@10/MRR@20 plateau for `20k-30k` steps or if Hit@20 degrades
   materially below `0.66`.
4. After continuation, run the formal Stage-A gate over latest/objective-best
   checkpoints and compare against M130/M150.

The first continuation launch was stopped after `750` steps because the default
schedule restarted from `soft/tau ~= 1.0`. A valid continuation must preserve
the late-stage shape:

```text
lr: 5e-5
mode: straight_through from step 1
tau: 0.35 -> 0.25
max_steps: 50000
```

## Corrected Continuation

The corrected continuation is running under a separate run name:

```text
run_name: bm25sae-m160a-pplx16384k96-stagea-cont-e3-v1
host: spark / huoju@100.123.2.95
output: /home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-cont-e3-v1
clearml_task: http://100.116.110.26:8080/projects/9cd355b16f5c48e49c9aa5aff5f013e0/experiments/61f0d0d3050349d8980d9d1ebf755a97/output/log
```

The first event confirms that the corrected run resumes in the intended
late-stage shape instead of restarting the soft schedule:

```text
step: 1
mode: straight_through
tau: 0.349998
lr: 5e-5
```

The continuation completed at `50000` steps. The strongest candidate-surface
points were:

| Point | Step | Hit@20 | MRR@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| Best Hit@20 | `40750` | `0.6783` | `0.4370` | `0.4417` |
| Best MRR | `46750` | `0.6772` | `0.4443` | `0.4487` |
| Final | `50000` | `0.6716` | `0.4367` | `0.4410` |

The continuation slightly improved the primary best MRR point, but the final
checkpoint regressed from the best intermediate point. This confirms that
Stage A should select by an explicit gate and not simply take the last epoch.

## Continuity Full-Corpus Gate

The first formal gate uses the existing M130 all-data full-corpus continuity
surface, with the same `doc64/query80` active clipping used in previous M150A2
Stage-A comparisons:

```text
corpus: /home/huoju/leask/runs/m130-pplx-all-data-eval-corpus
profile: doc_active_k=64, query_active_k=80
queries: 886
documents: 993,336
```

Results:

| Checkpoint | Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| Baseline | BM25 | `0.2439` | `0.2211` | `0.1542` | `0.0965` |
| Baseline | Dense | `0.3132` | `0.2992` | `0.2251` | `0.1504` |
| Baseline | BM25+dense score fusion | `0.3152` | `0.2815` | `0.2086` | `0.1375` |
| Continuation best | SAE | `0.2870` | `0.2614` | `0.1946` | `0.1336` |
| Continuation best | BM25+SAE score fusion | `0.2983` | `0.2726` | `0.2003` | `0.1337` |
| Continuation latest | SAE | `0.2866` | `0.2670` | `0.1970` | `0.1342` |
| Continuation latest | BM25+SAE score fusion | `0.3020` | `0.2739` | `0.2002` | `0.1343` |

Physical diagnostics:

| Checkpoint | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| Continuation best | `1,888,436` | `775,293` | `1.071 s` |
| Continuation latest | `1,872,487` | `771,015` | `1.093 s` |

Interpretation:

- `latest.pt` is the better M160A Stage-A candidate on this full-corpus gate.
- M160A improves standalone SAE quality versus the earlier M150A2 Stage-A
  result, but equal-weight BM25+SAE fusion still does not capture the full
  benefit.
- The blocker is now score/fusion shape and BM25-complement ranking, not only
  sparse semantic coverage.

## Post-Hoc Fusion Diagnostic

A rankings-producing run for `latest.pt` was used for a post-hoc sweep over
simple BM25 weights and BM25 admission limits:

```text
rankings: /home/huoju/leask/runs/bm25sae-m160a-cont_latest-full-corpus-eval-doc64-q80-rankings/m110_full_corpus_rankings.jsonl
diagnostic: /home/huoju/leask/runs/bm25sae-m160a-cont_latest-full-corpus-eval-doc64-q80-rankings/posthoc-fusion
```

Best sweep results:

| Metric | Best source | Value |
| --- | --- | ---: |
| Recall@100 | BM25+dense score fusion | `0.3152` |
| MRR@20 | Dense | `0.2992` |
| NDCG@10 | Dense | `0.2251` |
| MAP@100 | Dense | `0.1504` |

Best BM25+SAE sweep points:

| Objective | Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| Recall@100 | `w0.35_bm25top100` | `0.3050` | `0.2674` | `0.2017` | `0.1392` |
| MRR@20 | `w0.75_bm25top50` | `0.3024` | `0.2713` | `0.2037` | `0.1382` |
| MAP@100 | `w0.5_bm25top100` | `0.3041` | `0.2692` | `0.2023` | `0.1393` |

Conclusion: fixed post-hoc BM25/SAE score fusion is not enough. M160A has a
stronger sparse representation than M150A2, but it still needs BM25-aware
Stage-B training to convert that representation into the final retrieval
surface.

## Stage-B Follow-Up

A direct Stage-B canary was trained from `latest.pt`:

```text
run_name: bm25sae-m160a-latest-stageb-direct-v1
session: bm25sae_m160a_latest_stageb_direct_v1
init_checkpoint: /home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-cont-e3-v1/bm25sae_stagea_latest.pt
train_rows: /home/huoju/leask/runs/m130-pplx-all-data-candidate-rows
eval_rows: /home/huoju/leask/runs/bm25sae-pplx-all-data-eval-candidate-rows
```

This canary uses normalized SAE fusion, a BM25 scale anchor near the post-hoc
sweep range, and hard-pairwise losses. The early candidate-surface evals
already showed a positive signal:

| Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | Dense Hit@20 | Dense MRR@20 |
| ---: | ---: | ---: | ---: | ---: |
| `1` | `0.6772` | `0.4494` | `0.6817` | `0.4677` |
| `250` | `0.6862` | `0.4556` | `0.6817` | `0.4677` |
| `500` | `0.6896` | `0.4658` | `0.6817` | `0.4677` |

The candidate-surface trend was not used as promotion evidence. The deciding
evidence is the full-corpus eval below.

### Stage-B Direct Full-Corpus Result

The direct Stage-B canary completed and produced a clear continuity-surface
pass:

```text
run_name: bm25sae-m160a-latest-stageb-direct-v1
checkpoint: /home/huoju/leask/runs/bm25sae-m160a-latest-stageb-direct-v1/bm25sae_stageb_best.pt
full_corpus_eval: /home/huoju/leask/runs/bm25sae-m160a-latest-stageb-direct-v1-full-corpus-eval-doc64-q80
selection_step: 5000
scale_sae: 1.0972
scale_bm25: 0.3337
```

Candidate-surface best event:

| Row | Hit@20 | MRR@20 |
| --- | ---: | ---: |
| BM25 | `0.6061` | `0.3951` |
| Dense | `0.6817` | `0.4677` |
| SAE model | `0.8205` | `0.5794` |
| BM25+SAE model | `0.8070` | `0.5532` |

Continuity full-corpus gate:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | `0.2439` | `0.2211` | `0.1542` | `0.0965` |
| Dense | `0.3132` | `0.2992` | `0.2251` | `0.1504` |
| BM25+dense score fusion | `0.3173` | `0.2994` | `0.2262` | `0.1493` |
| SAE | `0.3369` | `0.3542` | `0.2487` | `0.1553` |
| BM25+SAE score fusion | `0.3421` | `0.3581` | `0.2528` | `0.1557` |
| BM25+SAE RRF | `0.3328` | `0.3393` | `0.2299` | `0.1398` |

Physical diagnostics:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| M160A Stage A latest | `1,872,487` | `771,015` | `1.093 s` |
| M160A latest + Stage-B direct | `2,574,658` | `847,743` | `1.420 s` |

Interpretation:

- This is the first M160 result that beats both dense and BM25+dense on the
  continuity full-corpus surface.
- The result also explains the earlier blocker: Stage A had enough semantic
  capacity, but the untrained score geometry did not transfer into final
  BM25+SAE ranking.
- The cost increased materially versus Stage A. The next optimization cannot
  ignore sparse fanout/postings.
- This is still not product promotion. The next gate must be an official BEIR
  representative full-corpus run to rule out continuity-surface overfitting.

## Official Representative Gate

The first official representative gate has been started with the same active
profile as the continuity breakthrough:

```text
run_name: bm25sae-m160a-stageb-direct-official-gate-doc64q80-df1-v1
checkpoint: /home/huoju/leask/runs/bm25sae-m160a-latest-stageb-direct-v1/bm25sae_stageb_best.pt
profile: doc_active_k=64, query_active_k=80, max_df_ratio=1.0
datasets: fiqa, scidocs, cqadupstack, trec-covid, msmarco
log: /home/huoju/leask/logs/bm25sae-m160a-stageb-direct-official-gate-doc64q80-df1-v1.log
```

This gate intentionally checks quality transfer before cost compression. If it
passes, the next step is an active-budget and DF-cap sweep to reduce sparse
fanout.
