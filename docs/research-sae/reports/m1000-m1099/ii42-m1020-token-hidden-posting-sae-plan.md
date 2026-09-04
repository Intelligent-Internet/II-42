# M1020 Token-Hidden Posting SAE Plan

Date: 2026-06-16

## Goal

M1000 proved that SAE atoms can be treated as BM25-style posting terms, but the
existing route still uses one global text embedding per document or query. The
paper that motivated M1000 works over patch-level hidden states, then pools SAE
activations into document-level term frequencies. M1020 tests the closer text
analogue:

```text
transformer token hidden states
    -> train sparse token atoms
    -> sum-pool token atoms into document/query term frequencies
    -> BM25/IDF over atom postings
    -> unified lexical + SAE posting retrieval
```

This is intentionally a training-first route. Runtime traversal work from
M1015 stays frozen until this representation surface proves useful.

## Hypothesis

The global-embedding SAE path may be losing too much local evidence before the
posting layer sees it. A token-hidden SAE can expose more "patch-like" semantic
terms:

- local concepts can fire independently inside long documents;
- document-level term frequency becomes a real pooled count/mass;
- corpus `df/idf` can suppress high-fanout semantic atoms;
- post-pool top-k clipping can control cost before learned fusion.

If this hypothesis is right, token-hidden SAE-BM25 should beat or approach the
global M1000 SAE-BM25 rows on small official corpora before any learned scorer.

## M1020.0 Smoke

Datasets:

- `nfcorpus`
- `scifact`

Corpus policy:

- official BEIR full corpus for each dataset;
- official test qrels and queries;
- no forced positives;
- no dataset-specific scoring profile.

Model:

- default transformer: `perplexity-ai/pplx-embed-v1-0.6B`;
- fallback allowed only if the model cannot expose token hidden states through
  Hugging Face `transformers`;
- token hidden states use the last hidden layer;
- no query/document prefix unless an explicit M1020 follow-up tests it.

Sparse model:

- train a small top-k token SAE from scratch;
- train examples are token hidden states from documents plus qrel-backed
  queries;
- token-level active atoms are small (`token_active_k`, default `8`);
- document/query vectors are sum-pooled token atoms;
- post-pool active clipping is mandatory (`doc_post_active_k`,
  `query_post_active_k`).

Loss:

- reconstruction MSE on token hidden states;
- fixed top-k sparse bottleneck;
- light activation L1;
- corpus/posting friendliness proxy during training:
  - penalize head atom domination in each batch;
  - encourage broader atom usage without forcing uniformity.

This is not a final retrieval loss. The goal is first to create a healthy
posting vocabulary.

## M1020.1 BM25-Compatible Atom Training

If M1020.0 shows competitive admission:

1. Increase training tokens and latent dimensions.
2. Add explicit post-encoding corpus diagnostics:
   - atom `df` distribution;
   - head atom ratio;
   - postings touched per query;
   - positive coverage by SAE-only candidate pool.
3. Add utility-aware regularization only after diagnostics show the head/noise
   atoms.

Do not jump directly into a learned fusion scorer. If the atom vocabulary is not
healthy, a scorer will hide the failure rather than solve it.

## M1020.2 Official Matrix

Only after M1020.1 passes on `nfcorpus`, `scifact`, and one medium dataset
(`fiqa`) should we run a larger official BEIR matrix.

Metrics:

- Recall@100
- MRR@20
- NDCG@10
- MAP@100
- average touched docs
- average SAE postings touched
- atom df/head ratio

Rows:

- lexical BM25;
- token-hidden SAE-BM25;
- unified lexical + token-hidden SAE-BM25;
- best global M1000 unified row for the same dataset;
- dense baseline where available.

## Stop Rules

Stop or redesign if:

- token-hidden SAE-BM25 is worse than global M1000 SAE-BM25 on both recall and
  top-rank metrics;
- atom `df` is too flat or too head-dominated after post-pool clipping;
- touched docs explode without recall gains;
- learned/fusion improvements are needed before the raw posting vocabulary
  shows any retrieval value.

## Current Execution Plan

1. Add `scripts/research_sae_m1020_token_hidden_posting_sae.py`.
2. Add `scripts/run_m1020_token_hidden_smoke_spark.sh`.
3. Run `nfcorpus` first on `spark-1`.
4. If `nfcorpus` shows a positive signal, run `scifact`.
5. Record results in this document before deciding whether to expand to
   `fiqa` or larger corpora.

## M1020.0 Result

Status: completed for official `nfcorpus`.

Artifacts:

- Remote:
  `/home/huoju/leask/runs/ii42-m1020-token-hidden-posting-sae-v1/nfcorpus_m1020_token_hidden.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/nfcorpus_m1020_token_hidden.json`

Configuration:

- `pplx-embed-v1-0.6B` transformer token hidden states;
- `180,000` sampled token states;
- latent dims `4096`;
- token active `8`;
- document post-pool active `96`;
- query post-pool active `80`;
- 4 epochs.

Result:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg Touched Docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| lexical BM25 | 0.2179 | 0.4870 | 0.2814 | 0.1212 | 1296 |
| token-hidden SAE-BM25 | 0.2928 | 0.4291 | 0.2572 | 0.1134 | 3610 |
| unified scale 0.5 | 0.2975 | 0.5325 | 0.3161 | 0.1465 | 3610 |
| unified scale 1.0 | 0.3067 | 0.5481 | 0.3276 | 0.1548 | 3610 |
| unified scale 2.0 | 0.3119 | 0.5413 | 0.3165 | 0.1448 | 3610 |

Diagnostics:

- active atoms: `499`;
- max df ratio: `1.0`;
- head 1% df ratio: `0.9992`;
- avg doc active atoms: `96.0`;
- avg query active atoms: `22.9`;
- average SAE postings touched: about `29,924` per query.

Interpretation:

- Token-hidden SAE-BM25 does add semantic recall over lexical BM25.
- The atom vocabulary is not BM25-healthy: head atoms effectively cover the
  whole corpus, so IDF cannot suppress semantic noise.
- The best unified row is useful but still below the existing global M1000
  official `nfcorpus` frontier, where global unified rows reached roughly
  `Recall@100 0.314-0.321` and `NDCG@10 0.347-0.351`.

Decision:

- Do not expand M1020.0 to full BEIR15.
- Run one targeted posting-friendly canary before parking the route.

## M1020.1 Posting-Friendly Regularization Canary

Status: completed for official `nfcorpus`.

Artifacts:

- Remote:
  `/home/huoju/leask/runs/ii42-m1020-token-hidden-posting-sae-v1/nfcorpus_m1020_token_hidden_reg_v1.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/nfcorpus_m1020_token_hidden_reg_v1.json`

Configuration changes from M1020.0:

- latent dims `8192`;
- token active `4`;
- document post-pool active `32`;
- query post-pool active `32`;
- stronger head usage penalty `0.2`;
- no entropy bonus;
- activation L1 `0.00005`.

Result:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg Touched Docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| lexical BM25 | 0.2179 | 0.4870 | 0.2814 | 0.1212 | 1296 |
| token-hidden SAE-BM25 | 0.2385 | 0.3170 | 0.1777 | 0.0729 | 3290 |
| unified scale 0.5 | 0.2779 | 0.5163 | 0.3039 | 0.1388 | 3296 |
| unified scale 1.0 | 0.2818 | 0.5215 | 0.3049 | 0.1373 | 3296 |
| unified scale 2.0 | 0.2731 | 0.4859 | 0.2756 | 0.1197 | 3296 |

Diagnostics:

- active atoms: `255`;
- max df ratio: `0.9994`;
- head 1% df ratio: `0.9810`;
- avg doc active atoms: `32.0`;
- avg query active atoms: `11.5`;
- average SAE postings touched: about `11,549` per query.

Interpretation:

- Stronger clipping and head regularization reduced postings but did not fix the
  root df problem.
- Quality fell sharply, especially SAE-only ranking.
- This is not a viable promotion path.

## M1020 Decision

The direct token-hidden route is not yet a better M1000 path.

What we learned:

- Transformer token hidden states can be turned into pooled SAE postings.
- A plain TopK token SAE does not automatically create a BM25-compatible term
  vocabulary.
- The problem is not only active clipping; the learned atom space itself is too
  high-fanout and too corpus-wide.

What not to do next:

- Do not run full BEIR15 with M1020.0/M1020.1.
- Do not tune only `token_active_k`, `doc_post_active_k`, or scalar SAE weight.
- Do not treat segment/token granularity as solved by simple hidden-state
  pooling.

If this route is reopened, the next attempt should be a new training objective:

1. train atoms against posting-level utility, not only reconstruction;
2. use qrel/BM25/dense-miss positives to reward atoms that admit useful
   documents;
3. penalize atoms by corpus df and touched postings after document pooling;
4. keep lexical tokens and SAE atoms in one unified sparse objective from the
   start.

That is closer to a new M1030/M1100 training line than a continuation of the
current M1020 smoke.
