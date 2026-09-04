# M1513 SSR Grouped-Posting Canary Report

## Decision

M1513 is stopped after S2. The paper-aligned grouped sparse objective is
learnable and improves a seeded random sparse head, but the trained model is
far below the existing P1/dense quality frontier and its K=4 coarse postings
still touch most or nearly all documents. M1514/M1515 broader scaling and
native integration were not launched.

This is a negative result for the tested frozen-BERT grouped branch, not a
claim that every late-interaction sparse retriever is impossible.

## Audited implementation

The experiment starts from SSR commit
`c64bc0599f3b00669db7b8dbddf130c07a6b3ea8`. The local patch restores the
independent `1/8 * L_recon(4K)` term, exposes warmup and sequence lengths,
seeds model construction before SAE initialization, adds the permitted
frozen-backbone control, and fixes checkpoint loading. Before the loader fix,
trained checkpoints silently rebuilt a random width-3072 SAE instead of
loading the saved width-16384 SAE.

The tested objective was:

`L_recon(K) + 1/8 L_recon(4K) + 1/32 L_aux + 0.1 L_sparse_cl + 0.05 L_MaxSim`

Configuration: BERT base, frozen backbone, 16,384 latents, K=32, auxK=2,048,
query/document length 32, batch size 64, learning rate `1e-3`, 160 warmup
steps, and 1,600 total steps. Training used 100,000 deterministic MS MARCO
BM25 triplets; selection used 5,000 disjoint heldout triplets.

S0 verified all five losses, finite non-zero gradients, checkpoint identity,
and deterministic checkpoint-zero creation. A joint BERT+SAE S1 branch
collapsed to 32 globally active features and was rejected. The single
permitted structural correction, freezing BERT and training the sparse
projector, passed S1 and was scaled to S2.

## Data and evaluation corrections

Two evaluation defects were found and corrected before the final gate:

1. The canary runner hard-coded the S1 validation path. The first S2-labeled
   replay therefore used the old 1,000-row heldout. It is preserved as
   `m1513_s2_checkpoint_on_s1_heldout_regression.json`, but is not S2 evidence.
   The corrected run explicitly used the S2 5,000-row heldout.
2. The official-data adapter originally symlinked raw BEIR JSONL. SSR's loader
   reads only `text`, while SSR's official preparation concatenates
   `title + text`. NFCorpus and SciFact were rebuilt with that exact contract
   before final evaluation.

The final official gate uses complete corpora, not samples:

| Dataset | Documents | Test queries | Positive qrel pairs |
| --- | ---: | ---: | ---: |
| NFCorpus | 3,633 | 323 | 12,334 |
| SciFact | 5,183 | 300 | 339 |

Quality uses exact K=32 sparse MaxSim through SSR's inverted-index scorer.
Cost uses the paper-native K=4 coarse support on both query and document
tokens. Its candidate upper bound is therefore the qrel recall among all
documents sharing at least one coarse atom; it is not the same pool definition
as the legacy II-42 candidate-upper-bound column.

## S2 heldout result

| Metric | Checkpoint zero | Trained | Delta |
| --- | ---: | ---: | ---: |
| Pair accuracy | 0.3576 | 0.5114 | +0.1538 |
| Mean positive-negative margin | -0.26954 | 0.00996 | +0.27950 |
| K=32 reconstruction cosine | 0.54278 | 0.83434 | +0.29156 |
| K=32 reconstruction MSE | 0.22323 | 0.08563 | -0.13760 |
| K=4 mean touched-document ratio | 0.86501 | 0.80916 | -0.05586 |
| K=4 max DF ratio | 0.55820 | 0.40670 | -0.15150 |

The true heldout gate passes. Training is doing real work, and the result is
not explained by a broken loader or an unseeded checkpoint-zero comparison.

## Full official quality

| Dataset | Checkpoint | NDCG@10 | MAP@100 | Recall@100 | MRR@20 |
| --- | --- | ---: | ---: | ---: | ---: |
| NFCorpus | Initial | 0.02671 | 0.01105 | 0.06460 | 0.06385 |
| NFCorpus | Trained | 0.07638 | 0.02764 | 0.13099 | 0.16581 |
| SciFact | Initial | 0.04523 | 0.03614 | 0.23428 | 0.03587 |
| SciFact | Trained | 0.06018 | 0.05648 | 0.26067 | 0.05996 |

The trained checkpoint improves every reported quality metric over checkpoint
zero on both independent corpora. That is a valid learning signal, but it is
not competitive retrieval quality.

## Existing frontier context

| Dataset | Method | NDCG@10 | MAP@100 | Recall@100 | MRR@20 |
| --- | --- | ---: | ---: | ---: | ---: |
| NFCorpus | BM25 | 0.28860 | 0.13318 | 0.22852 | 0.48091 |
| NFCorpus | Dense | 0.32283 | 0.13978 | 0.26911 | 0.54879 |
| NFCorpus | P1-a0125 | 0.30398 | 0.13928 | 0.28641 | 0.51418 |
| NFCorpus | External SPLARE reproduction | 0.36954 | 0.18810 | 0.33558 | 0.58855 |
| SciFact | BM25 | 0.66393 | 0.62683 | 0.88256 | 0.63484 |
| SciFact | Dense | 0.71406 | 0.68514 | 0.89822 | 0.69445 |
| SciFact | P1-a0125 | 0.67899 | 0.64887 | 0.90433 | 0.65490 |
| SciFact | External SPLARE reproduction | 0.69560 | 0.65445 | 0.95333 | 0.66338 |

On NFCorpus, trained SSR NDCG@10 remains 0.22760 below P1-a0125. On
SciFact, it remains 0.61881 below P1-a0125. The gap is too large to justify a
broader run merely because checkpoint zero was beaten.

## Full official posting cost

| Dataset | Checkpoint | K=4 CUB | K=4 touch ratio | K=4 max DF | Head 1% posting share |
| --- | --- | ---: | ---: | ---: | ---: |
| NFCorpus | Initial | 0.84011 | 0.82584 | 0.78503 | 0.42303 |
| NFCorpus | Trained | 0.89809 | 0.85384 | 0.96862 | 0.69367 |
| SciFact | Initial | 1.00000 | 0.99742 | 0.74822 | 0.42208 |
| SciFact | Trained | 1.00000 | 0.99903 | 0.95273 | 0.73948 |

Training improves the coarse candidate upper bound, but it also concentrates
postings into head features and worsens official-corpus fanout. SciFact is an
effective full-corpus scan even at K=4. Exact K=32 touches 100% of documents
on both trained rows.

## Conclusion and retained knowledge

M1510 showed that retrieval-trained sparse features can carry quality while a
pooled representation collapses cost. M1513 shows that retaining token groups
does not automatically repair that frontier: the grouped objective learns
pair ordering and reconstruction, but the tested sparse support remains too
common and its zero-shot retrieval ranking is weak.

The useful retained result is narrower:

- The SSR loss implementation and checkpoint path are now audited.
- Frozen-backbone training avoids the catastrophic 32-feature collapse seen
  in joint training.
- Pairwise heldout improvement is not sufficient evidence for retrieval or
  posting efficiency.
- A cost gate must be evaluated on complete independent corpora; a 10,000-doc
  MS MARCO sample understated the SciFact fanout failure.
- Future grouped work would require a retrieval-pretrained late-interaction
  backbone or a source/objective that explicitly penalizes corpus-wide DF.
  Repeating this frozen generic-BERT schedule is not justified.

Per the M1513 contract, failure to improve the existing quality/cost frontier
stops the branch. No K sweep, FiQA, shared15, native integration, or M1514/M1515
scale run is authorized from this checkpoint.

## Retired-checkpoint archive

The five superseded S0/S1 checkpoint directories were archived to:

`/Volumes/Betty/Tmp/ii42-spark-archive/spark-1/20260710-m1513-retired-checkpoints/m1513-retired-checkpoints.tar.zst`

The clean archive is 13,720,958,263 bytes with SHA-256
`e04103b2b4bb30c5dfc5424a1d1d83e454a31bf0575b1494cd27bd8084ba36f5`.
It passes zstd integrity, direct standard tar listing, exact top-level
directory comparison, and 214/214 non-directory file comparison. The five
verified source directories were then removed, releasing 15,706,660,864
bytes on spark-1. `s1-frozen-200steps` and `s2-frozen-1600steps` remain on the
node; data, results, and logs were not touched.

The first transfer contained an NGC banner before the tar stream. It was not
accepted as the final archive. The sidecar rewrote a clean standard tar.zst
and repeated all checks before deleting any source directory. Complete
evidence is in the adjacent `archive-metadata.txt`.

## Artifacts

- `docs/research-sae/reports/m1500-m1599/ii42-m1513-ssr-grouped-canary-contract.md`
- `patches/m1513-ssr-paper-alignment.patch`
- `scripts/evaluate_m1513_ssr_canary.py`
- `scripts/evaluate_m1513_ssr_official.py`
- `scripts/prepare_m1513_ssr_canary_data.py`
- `scripts/prepare_m1513_official_eval_data.py`
- `scripts/run_m1513_ssr_grouped_canary_spark.sh`
- `scripts/run_m1513_ssr_canary_eval_spark.sh`
- `scripts/run_m1513_ssr_official_eval_spark.sh`
- `runs/m1513_ssr_grouped_canary_v1/`
