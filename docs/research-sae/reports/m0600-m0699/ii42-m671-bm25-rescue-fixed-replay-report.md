# M671 BM25 Rescue Fixed Replay

- Source label: `P1-a0125+M670`
- Baseline label: `P1-a0125-fixed-union`
- preserve_top_k: `98`
- admission_feature: `bm25_score`
- guard status: `accepted`
- failed checks: `[]`
- harmed datasets: `[]`

## Macro

| Source | CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `P1-a0125-fixed-union` | 0.741534 | 0.413351 | 0.231289 | 0.364802 | 0.543658 |
| `P1-a0125+M670` | 0.741534 | 0.417400 | 0.231604 | 0.364802 | 0.543658 |

Macro deltas:

| Metric | Delta |
| --- | ---: |
| `candidate_upper_bound` | +0.000000 |
| `map_at_100` | +0.000315 |
| `mrr_at_20` | +0.000000 |
| `ndcg_at_10` | +0.000000 |
| `recall_at_100` | +0.004049 |

## Per Dataset

| Dataset | Queries | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 40 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `nfcorpus` | 323 | +0.003545 | +0.000272 | +0.000000 | +0.000000 |
| `quora` | 40 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `webis-touche2020` | 49 | +0.013982 | +0.001114 | +0.000000 | +0.000000 |

## Eval-Style Outputs

- `runs/m671_bm25_rescue_fixed_replay_v1/evals/m671_native_available4/cqadupstack/P1-a0125-fixed-union/eval.json`
- `runs/m671_bm25_rescue_fixed_replay_v1/evals/m671_native_available4/nfcorpus/P1-a0125-fixed-union/eval.json`
- `runs/m671_bm25_rescue_fixed_replay_v1/evals/m671_native_available4/quora/P1-a0125-fixed-union/eval.json`
- `runs/m671_bm25_rescue_fixed_replay_v1/evals/m671_native_available4/webis-touche2020/P1-a0125-fixed-union/eval.json`
- `runs/m671_bm25_rescue_fixed_replay_v1/evals/m671_native_available4/cqadupstack/P1-a0125+M670/eval.json`
- `runs/m671_bm25_rescue_fixed_replay_v1/evals/m671_native_available4/nfcorpus/P1-a0125+M670/eval.json`
- `runs/m671_bm25_rescue_fixed_replay_v1/evals/m671_native_available4/quora/P1-a0125+M670/eval.json`
- `runs/m671_bm25_rescue_fixed_replay_v1/evals/m671_native_available4/webis-touche2020/P1-a0125+M670/eval.json`

## Matrix Interface Check

M671 also compiles through the existing M603 native matrix compiler:

- JSON: `runs/m671_bm25_rescue_fixed_replay_v1/m671_native_available4_matrix.json`
- Markdown: `runs/m671_bm25_rescue_fixed_replay_v1/m671_native_available4_matrix.md`

This matrix is an interface check, not a final promotion surface.  The
`BM25` and `dense` rows come from full shared15 baseline files, while the
`P1-a0125-fixed-union` and `P1-a0125+M670` rows come from available M604 replay
queries.  Query counts therefore differ (`349` versus `452`), and the compiler
correctly flags the result as not apples-to-apples.

## Conclusion

The fixed M670 rule remains accepted when replayed as an eval-style native source.  This is the candidate rule to scale next through the native DB/plugin path; do not train a richer scorer until this deterministic rule is validated more broadly.
