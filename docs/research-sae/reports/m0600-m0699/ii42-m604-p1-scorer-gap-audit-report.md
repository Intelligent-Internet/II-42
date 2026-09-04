# M604 / P1 Scorer Gap Audit

Status: audit completed for the M605 scorer-recovery decision.

This report justifies trying M605 because multiple native rows contain
under-ranked positives that are already inside the P1/BM25 candidate pool. The
subsequent M605 report supersedes the early "proceed" decision: tested global
rerankers did not pass the guarded native matrix, so the current action is to
stop scorer micro-tuning and return to the upstream P1/native score surface
rather than continue M605 variants.

M603 is frozen at official native coverage `12/15`. The remaining fixed-alpha
matrix expansion is paused because the observed gap is now scorer/reranker
dominated: P1-a0125 exposes a stronger candidate upper bound than BM25 on the
completed official rows, but loses Recall@100 and MAP@100 when the fixed alpha
scorer fails to promote relevant documents into the final top100.

This report records the first M604 evidence used to decide whether M605 should
proceed.

## Method

The audit uses the native PostgreSQL candidate path, not an offline full-corpus
scan. For each query it exports the candidate union produced by:

- P1 semantic candidates from the frozen P1-a0125 atom surface.
- BM25 candidates from the native II-42 BM25 index.
- The same `ii42_hybrid_fuse_candidates` fixed-alpha scorer used by M603.

The exporter writes per-candidate JSONL rows with:

- `candidate_present`; qrel-only misses are emitted as rows with
  `candidate_present=false` so M605 can keep the correct recall denominator.
- `label` and `rel_score` from qrels.
- `fused_rank`, `fused_score`, and per-query fused z-score.
- `p1_rank`, `p1_score`, reciprocal rank, and per-query z-score.
- `bm25_rank`, `bm25_score`, reciprocal rank, and per-query z-score.
- source arrays emitted by the native hybrid fusion function.

Positive qrels are classified as:

- `top100_hit`: positive appears in fused top100.
- `candidate_present_under_ranked`: positive appears in P1/BM25/fused
  candidate evidence but not fused top100.
- `candidate_miss`: positive is absent from the audited candidate union.

The M604 stop condition is conservative: if candidate misses are at least as
large as under-ranked positives, do not start M605 for that surface.

## Initial Native Audit Results

| Dataset | Queries | Positives | Top100 hit | Under-ranked | Candidate miss | BM25 rescue only | Semantic only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `webis-touche2020` | 49 | 932 | 410 (0.4399) | 434 (0.4657) | 88 (0.0944) | 97 | 67 |
| `nfcorpus` | 323 | 12334 | 2094 (0.1698) | 5385 (0.4366) | 4855 (0.3936) | 1565 | 2946 |

Artifacts:

- `runs/m604_p1_scorer_gap_audit_v1/webis_touche2020_full/webis-touche2020_m604_p1_scorer_gap.json`
- `runs/m604_p1_scorer_gap_audit_v1/webis_touche2020_full/webis-touche2020_m604_p1_scorer_gap.jsonl`
- `runs/m604_p1_scorer_gap_audit_v1/nfcorpus_full/nfcorpus_m604_p1_scorer_gap.json`
- `runs/m604_p1_scorer_gap_audit_v1/nfcorpus_full/nfcorpus_m604_p1_scorer_gap.jsonl`

## Interpretation

`webis-touche2020` is the stronger decision row for this audit because it was
one of the known P1 fixed-alpha loss rows. Its candidate miss rate is low
relative to under-ranking:

```text
under-ranked positives: 434 / 932 = 0.4657
candidate misses:        88 / 932 = 0.0944
```

That means most recoverable loss is already inside the native P1/BM25 candidate
pool. A global reranker has a plausible target and M605 is justified for this
row.

`nfcorpus` is less clean because candidate misses are high, but under-ranked
positives are still the largest single category:

```text
under-ranked positives: 5385 / 12334 = 0.4366
candidate misses:       4855 / 12334 = 0.3936
```

This does not reject M605. It says M605 should not be judged only on nfcorpus,
and candidate-generation limitations still exist for qrels-heavy datasets.

## Decision

Proceed to M605-A with a global interpretable reranker over native candidate
features. The first M605 pass should use exported candidate rows, query-level
splits, and no dataset id feature.

M605-A acceptance remains:

- Improve P1-a0125 macro Recall@100 and MAP@100 on native-path evaluation.
- Do not materially regress NDCG@10 or MRR@20.
- Reduce losses on at least two of `quora`, `cqadupstack`, and
  `webis-touche2020`.
- Reject any gain that requires dataset-specific thresholds or disappears when
  evaluated through the native DB path.

## Backend Scalability Note

The initial `webis-touche2020` and `nfcorpus` runs used the table backend,
which is acceptable for small/medium audits but too slow for bounded
`cqadupstack`/`quora` probes because it scans unnested atom arrays per query.
The M604 exporter now also supports a `postings` backend using the existing P1
atom postings builder. A `nfcorpus` 3-query postings smoke succeeded and
matched the expected classification shape:

```text
candidate_miss=58
candidate_present_under_ranked=59
top100_hit=27
```

Large target rows should therefore use `--semantic-backend postings`, with the
postings table in the `ii42_m603_betty` tablespace where needed.

## Target-Row Postings Audits

The exporter now supports deterministic query sampling and explicit query-id
filters:

- `--sample-queries N --sample-salt SALT` for stable bounded shards.
- `--query-ids-file PATH` for targeted query lists, such as worst-query probes
  from an existing matrix.

New postings-backend probes:

| Dataset / shard | Queries | Positives | Top100 hit | Under-ranked | Candidate miss | Semantic only | BM25 rescue only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` stable q40 | 40 | 80 | 47 (0.5875) | 21 (0.2625) | 12 (0.1500) | 23 | 2 |
| `quora` stable q40 | 40 | 52 | 52 (1.0000) | 0 (0.0000) | 0 (0.0000) | 0 | 0 |
| `quora` worst40 by P1-a0125 recall | 40 | 66 | 64 (0.9697) | 2 (0.0303) | 0 (0.0000) | 2 | 0 |

Artifacts:

- `runs/m604_p1_scorer_gap_audit_v1/cqadupstack_q40_postings/cqadupstack_m604_p1_scorer_gap.json`
- `runs/m604_p1_scorer_gap_audit_v1/cqadupstack_q40_postings/cqadupstack_m604_p1_scorer_gap.jsonl`
- `runs/m604_p1_scorer_gap_audit_v1/quora_q40_postings/quora_m604_p1_scorer_gap.json`
- `runs/m604_p1_scorer_gap_audit_v1/quora_worst40_postings/quora_m604_p1_scorer_gap.json`
- `runs/m604_p1_scorer_gap_audit_v1/query_sets/quora_worst40_p1_a0125.txt`

`cqadupstack` remains a useful M605 training row: under-ranked positives are
present and candidate misses do not dominate.

`quora` is not yet explained by this positive-level top1000 audit. The official
native matrix shows a large P1-a0125 gap despite a high candidate upper bound
(`Recall@100=0.60185`, `Candidate UB=0.98709`), but both random and worst-query
bounded audits are almost entirely top100 hits. Treat this as an audit
coverage/definition warning, not as a clean M605 training signal. Before using
`quora` to train or judge M605, reconcile the matrix denominator/query set with
the M604 qrels and candidate extraction path.

A direct current table-backed rerun for
`beir15:quora:q:100149` produced `Recall@100=1.0`, `MRR@20=0.5`,
and `Candidate UB=1.0`, while the older M603 matrix artifact recorded
`Recall@100=0.0` for the same query. This points to a stale or inconsistent
M603 quora row rather than a confirmed P1 scoring failure. Re-run quora through
the current native path before using it in M605 acceptance or training.

## Final Routing

The original next steps were executed in M605:

1. `cqadupstack_q40`, `webis-touche2020`, and `nfcorpus` were used for guarded
   target3 M605-A/M605-B runs.
2. `quora` remained excluded from training because the bounded audit did not
   reproduce the old matrix failure mode.
3. Native replay and matrix integration were added through
   `scripts/apply_m605_global_reranker.py`,
   `scripts/compile_m603_native_surface_matrix.py`, and
   `scripts/check_m605_guarded_matrix.py`.
4. Logistic, alpha-blended logistic, GBDT, and pairwise-linear rerankers were
   tested.

Final decision: stop this M605 reranker route for now. The M604 audit remains
useful as evidence that under-ranking exists, but the tested scorer families
failed the guarded native-path acceptance rule. The next useful work is to
change the upstream P1 score/posting surface so fixed-union ranking is more
stable before rerunning a new M604/M605 audit cycle.
