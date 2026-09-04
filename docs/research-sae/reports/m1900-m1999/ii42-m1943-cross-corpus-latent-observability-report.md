# M1943 Cross-Corpus Latent Observability

Decision: **stop_bounded_query_residual_family**

## Dataset Summary

| Dataset | Queries | Residual | TargetQueries | TargetEntropy | TargetMaxShare | ProposalRecall | AnyHit |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| fiqa | 648 | 250 | 250 | 0.9952 | 0.0120 | 0.0191 | 0.1120 |
| scifact | 300 | 14 | 14 | 0.9959 | 0.1429 | 0.0000 | 0.0000 |

## Cross-Corpus Gate

- target queries: `264`
- target entropy: `0.995003`
- target max query share: `0.011364`
- proposal target recall: `0.018078`
- proposal any-hit rate: `0.106061`
- pooled LODO AUC: `None`
- minimum fold AUC: `None`

| Check | Passed |
| --- | --- |
| `dataset_count` | `false` |
| `lodo_fold_count` | `false` |
| `lodo_min_fold_auc` | `false` |
| `lodo_pooled_auc` | `false` |
| `proposal_any_target_hit_rate` | `false` |
| `proposal_any_target_hit_rate_per_dataset` | `false` |
| `proposal_target_recall` | `false` |
| `proposal_target_recall_per_dataset` | `false` |
| `residual_queries_per_dataset` | `true` |
| `target_entropy` | `true` |
| `target_max_query_share` | `true` |
| `target_query_count` | `true` |

## Interpretation

M1943 separates the M1942 failure into two parts.

First, the useful oracle target is not universal. Across 264 target-bearing
queries, M1943 finds 1,894 distinct target latents and normalized entropy
`0.9950`; the most common latent appears in only `1.14%` of target queries.
FiQA target document DF has median `0.287%` and p95 `2.34%`. This rules out the
hypothesis that the only available residual signal is the universal-latent
shape learned by M1942.

Second, the fixed qrels-free proposal cannot rank that target into a deployable
budget. FiQA and SciFact expose `60.35%` and `82.14%` of targets in the union of
BM25 top-32, semantic top-32 and native ranks 96-256. Full ranks 96-1000 expose
`100%`. However, only `44.87%` and `58.04%` have positive source-versus-head
contrast, with median positive proposal ranks `884` and `1,253`. A 64-posting
proposal consequently recalls only `1.91%` on FiQA and `0%` on SciFact.

The missed relevant documents are not confined to the immediate boundary:
their median ranks are `301.5` on FiQA and `190.5` on SciFact, with p95 ranks
`863.0` and `682.3`. A local top-256 heuristic therefore cannot recover most
of the residual opportunity.

## Decision

Close M1943 at the structural canary. Do not run the four-corpus gate, unseen
transfer, or another bounded query-head training run. The predeclared proposal
coverage floors fail by a wide margin, including a complete SciFact row
failure.

This does not prove that the broad retrieval-conditioned source is
unobservable. It proves that the current hand-ranked top-64 interface is the
wrong compiler input. One independent diagnostic remains scientifically
justified: measure target-versus-harm separability inside the broad qrels-free
source pool using LODO validation. That experiment must have a new contract
and must not retroactively alter the M1943 gate. If broad-pool separability also
fails, the bounded residual family is closed rather than tuned again.

This is a train-free observability audit. Qrels define labels only; proposal features are qrels-free.
