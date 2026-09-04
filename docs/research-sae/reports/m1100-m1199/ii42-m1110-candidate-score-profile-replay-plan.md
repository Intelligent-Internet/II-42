# ii42 M1110 Candidate Score Profile Replay Plan

Date: 2026-07-08

## Why

M1100 and M1101 stopped the semantic-neighbor pair-construction branch:

- M1100 improved atom-only quality and unified Recall, but lost M1050
  MRR/NDCG/MAP.
- M1101 reduced semantic pressure and became safer versus lexical BM25, but
  lost all four unified metrics versus M1050.

This means another semantic-pair count / LR swap is not justified.

The useful old evidence is B12/M150/M160:

- fixed scoring profiles can recover meaningful aggregate quality;
- learned selectors did not generalize under LODO;
- global reranker features are weak inside the same query, so blindly training
  another scorer is also not justified.

M1110 therefore adds the missing observability: export per-query candidate
scores from the M1050-style evaluator and replay fixed scoring profiles before
training anything else.

## Implementation

New/changed files:

- `scripts/research_sae_m1040_atom_posting_utility.py`
  - optional `--ranking-export-dir`;
  - optional `--ranking-export-k`;
  - writes `*_candidate_scores.jsonl` and `*_queries.jsonl`.
- `scripts/research_sae_m1050_broad_atom_posting_terms.py`
  - exposes export flags.
- `scripts/research_sae_m1100_semantic_neighbor_posting.py`
  - exposes export flags.
- `scripts/run_m1100_semantic_neighbor_posting_spark.sh`
  - optional `RANKING_EXPORT_DIR`;
  - optional `RANKING_EXPORT_K`.
- `scripts/audit_m1110_candidate_score_profile_replay.py`
  - fixed-profile replay over exported lexical/atom candidate score rows.

Default behavior is unchanged. Export only happens when explicitly enabled.

## Next Run

Run a M1050-compatible replay export on the current three-dataset heldout
surface. Recommended first export:

```text
RUN=/home/huoju/leask/runs/ii42-m1110-m1050-profile-export-v1
OUTPUT_NAME=m1110_m1050_profile_export_s1050.json
SEED=1050
SEMANTIC_MAX_PAIRS_PER_DATASET=0
SEMANTIC_HARD_NEGATIVES_PER_QUERY=0
RANKING_EXPORT_DIR=/home/huoju/leask/runs/ii42-m1110-m1050-profile-export-v1/rankings
RANKING_EXPORT_K=1000
```

This uses the M1100 runner with semantic pairs disabled so the candidate export
matches the M1050-style baseline as closely as possible.

## Replay Profiles

M1110 tests deterministic, globally fixed profiles:

- lexical;
- atom-only;
- additive lexical + atom weights;
- lexical base with atom residual;
- atom base with lexical residual.

It does not train a selector and does not tune per dataset.

## Pass Gate

Pass only if the best fixed profile on heldout:

- improves MAP@100 and Recall@100 versus lexical BM25;
- does not materially regress MRR@20 or NDCG@10;
- gives a clearer path than M1050 `unified_scale_0.5`.

If replay cannot beat M1050-style fixed scale, stop profile replay and move to
query-local score calibration with explicit dense/BM25 boundary supervision.

## Result

M1110 completed on the three-dataset heldout surface.

Report:

```text
docs/research-sae/reports/m1100-m1199/ii42-m1110-profile-replay-result-report.md
```

Best heldout MAP profile:

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical` | 0.486992 | 0.392967 | 0.325332 | 0.258925 |
| `additive_atom_0.5` | 0.512545 | 0.401204 | 0.335614 | 0.266129 |
| `lex_residual_atom_0.75` | 0.520115 | 0.402480 | 0.333091 | 0.265431 |

Best delta versus lexical:

| Profile | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `additive_atom_0.5` | +0.025554 | +0.008237 | +0.010283 | +0.007204 |
| `lex_residual_atom_0.75` | +0.033123 | +0.009512 | +0.007760 | +0.006506 |

Interpretation:

- Fixed profile replay is real: lexical + atom improves all four heldout
  metrics.
- The best MAP profile is `additive_atom_0.5`, which matches the current
  M1050-style unified profile. So simple fixed-profile search is not a new
  breakthrough beyond validating the existing profile.
- `lex_residual_atom_0.75` is a useful alternate tradeoff: more Recall/MRR,
  slightly lower NDCG/MAP. This creates a concrete M1111 question: is
  query-level profile choice learnable without repeating the old M160 selector
  failure?

## Stop Rule

Do not run more atom-vocabulary training until this replay answers whether the
current scoring profile is the bottleneck. If the exported candidate rows show
low same-query separability, the next route must change the first-stage score
geometry rather than add another reranker.

M1110 answers that the fixed profile is not the main missing piece. The next
step is a query-level oracle/separability audit over a very small profile set:
`additive_atom_0.5` versus `lex_residual_atom_0.75`.
