# M1000 SAE-BM25 Atom Retrieval Smoke Report

## Summary

M1000 tests whether the BM25-V idea from `2603.05781v1` transfers to text-side
SAE atoms:

- Treat SAE latent atoms as posting terms.
- Treat document atom activations as term frequency.
- Compute corpus `df/idf` for atoms.
- Score with BM25 before any learned fusion/scorer.

The initial smoke is positive but narrow. SAE-BM25 atoms are useful as a
first-stage admission/retrieval signal, especially when the atom space is kept
aggressively sparse. They are not yet a better top-rank scorer than SAE dot
product at high active counts.

This supports the paper-inspired direction: fix atom posting quality, clipping,
`df/idf`, and fanout first. Do not jump directly to learned fusion.

## Paper Mapping

The paper, "Visual Words Meet BM25", applies Okapi BM25 to SAE activations from
ViT patch features. Its retrieval unit is:

1. Patch feature -> top-k SAE visual words.
2. Sum-pool patch activations into image-level visual-word term frequency.
3. Apply post-pool top-k filtering.
4. Compute corpus `df/idf` over visual words.
5. Use an inverted index and BM25 as first-stage retrieval.
6. Optionally dense-rerank the sparse candidate set.

The closest text-side analogue is not learned fusion. It is a sparse retrieval
surface where lexical BM25 terms and SAE atoms are both posting terms with
term frequency, document frequency, IDF, and length normalization.

The largest difference is representation granularity. BM25-V uses patch-level
features, then pools them. Our smoke uses global text embeddings and their SAE
atoms. That is weaker and less local. A fairer text analogue may need
sentence/chunk/token-level atom pooling before document-level clipping.

## Experiment Setup

Worktree:

- `/Volumes/Betty/Tmp/ii42-m1000/psql_bm25s_sae_m1000`

Branch:

- `codex/m1000-sae-bm25-atoms`

Machine:

- `spark-2`

Checkpoint:

- `/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1/bm25sae_stageb_best.pt`

Script:

- `scripts/research_sae_m1000_sae_bm25_atoms.py`

Datasets:

- `nfcorpus_dev_120`
- `scifact_train`

Scoring variants:

- `dot`: SAE sparse dot-product baseline.
- `bm25_binary_query`: BM25 over SAE atoms, query atom presence only.
- `bm25_query_weighted`: BM25 over SAE atoms, multiplied by normalized query atom activation.

Sweep:

- `doc_active_k`: `16, 32, 64`
- `query_active_k`: `16, 32, 80`
- BM25: `k1=1.5`, `b=0.75`

## Results

### nfcorpus dev 120

| Variant | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs | Max df ratio | Median df ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| d16 q16 dot | 0.1597 | 0.3557 | 0.1890 | 0.0582 | 264.8 | 0.408 | 0.0008 |
| d16 q16 BM25 weighted | 0.1659 | 0.3503 | 0.1937 | 0.0624 | 264.8 | 0.408 | 0.0008 |
| d16 q32 dot | 0.2022 | 0.3741 | 0.2009 | 0.0674 | 450.1 | 0.408 | 0.0008 |
| d16 q32 BM25 weighted | 0.1990 | 0.3707 | 0.2014 | 0.0682 | 450.1 | 0.408 | 0.0008 |
| d64 q80 dot | 0.2630 | 0.4888 | 0.2721 | 0.1010 | 1867.0 | 0.658 | 0.0017 |
| d64 q80 BM25 weighted | 0.2716 | 0.4697 | 0.2705 | 0.1002 | 1867.0 | 0.658 | 0.0017 |

Readout:

- BM25 weighted improves Recall@100 in the high-active setting, but top-rank
  quality is still better with dot.
- At low active counts, BM25 weighted improves NDCG/MAP over dot, but the
  signal is modest.
- The max df ratio grows from `0.408` at `doc_active_k=16` to `0.658` at
  `doc_active_k=64`, which is a warning sign: high-active atoms make many docs
  touch the same high-DF dimensions.

### scifact train

| Variant | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs | Max df ratio | Median df ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| d16 q16 dot | 0.9302 | 0.5789 | 0.6136 | 0.5693 | 1187.4 | 0.287 | 0.0006 |
| d16 q16 BM25 weighted | 0.9413 | 0.5907 | 0.6267 | 0.5811 | 1187.4 | 0.287 | 0.0006 |
| d16 q32 dot | 0.9687 | 0.6547 | 0.6901 | 0.6426 | 1720.4 | 0.287 | 0.0006 |
| d16 q32 BM25 weighted | 0.9718 | 0.6640 | 0.6969 | 0.6498 | 1720.4 | 0.287 | 0.0006 |
| d64 q80 dot | 0.9977 | 0.9559 | 0.9539 | 0.9377 | 4084.5 | 0.470 | 0.0017 |
| d64 q80 BM25 weighted | 0.9955 | 0.9322 | 0.9367 | 0.9154 | 4084.5 | 0.470 | 0.0017 |

Readout:

- In sparse/clipped regimes, BM25 weighted beats dot on every listed metric:
  `d16 q16` and `d16 q32` both improve Recall@100, MRR@20, NDCG@10, and MAP@100.
- In high-active regimes, dot dominates top-rank metrics. This matches the
  BM25-V paper's warning: when too many dimensions survive, IDF is diluted and
  high-DF atoms start to behave like noise.
- This is the strongest signal that SAE atoms can behave like BM25 terms, but
  only when the posting vocabulary is kept compact.

## Interpretation

M1000 confirms that the paper is relevant to our direction, but it also narrows
the likely path:

- SAE atoms can be scored as BM25 terms.
- Query-weighted BM25 is better than binary query BM25 in the first smoke.
- Low/post-pool active clipping is important.
- High active counts increase touched docs and weaken IDF.
- SAE-BM25 should first target candidate admission and recall coverage.
- Top-rank scoring still needs either dot, dense rerank, or a later learned
  utility scorer.

This argues against continuing to treat fusion weights as the main problem. The
more basic question is whether the atom vocabulary has the right `df/idf`
distribution and whether post-pool clipping keeps the sparse retrieval surface
healthy.

## Differences From The Vision Paper

The paper's strongest structural advantage is patch-level evidence. A visual
document contains many patch SAE activations, then sum-pooling creates robust
term frequencies. Our current text smoke has only one global embedding per
document/query, so it cannot express multiple local evidence regions.

Text-side next steps should therefore test:

- Sentence/chunk-level document atom pooling.
- Query-side global atoms versus query phrase/chunk atoms.
- Post-pool top-k clipping after pooling, not only top-k on the original global
  vector.
- Corpus-level atom Zipf and IDF diagnostics before ranking training.

## Next M1000 Steps

1. Add a dedicated `df/idf/fanout` diagnostic report for SAE atom postings.
   The first hard question is whether the atom distribution is actually
   BM25-compatible across more than two datasets.

2. Add post-pool clipping simulation.
   The paper's core trick is not just SAE sparsity; it is post-pool top-k after
   aggregation. Our global-embedding smoke only approximates that.

3. Run one medium dataset with the same evaluator.
   A good next target is `fiqa` or `scidocs`. The goal is not full BEIR15 yet,
   but checking whether the low-active BM25 benefit survives outside scifact.

4. Prototype chunk/sentence pooling on a small corpus.
   If pooling improves `df/idf` shape and recall at low fanout, M1000 becomes a
   stronger route than another learned-fusion pass.

5. Keep M1000 separate from M307.
   M307 is a learned posting-level scorer surface. M1000 is a representation and
   sparse-index sanity check: can SAE atoms be made into good terms before
   training a scorer?

## Current Decision

Continue M1000, but do not scale directly to full training yet.

The next useful experiment is not a learned fusion model. It is:

- atom `df/idf` diagnostics,
- low-active/post-pool clipping,
- one medium dataset smoke,
- then a chunk/sentence pooling smoke.

If those pass, the paper-inspired route becomes a clean input to M307-style
posting-level training. If they fail, learned fusion is probably compensating
for an unhealthy atom vocabulary.

## M1000.1 Follow-Up

After the first smoke, the evaluator was extended with:

- lower `doc_active_k=8`, to better simulate the paper's aggressive post-pool
  clipping regime;
- high-DF atom filtering with `df_max_ratio` sweep: `0`, `0.1`, `0.25`, `0.5`;
- richer `df/idf/fanout` diagnostics, including IDF quantiles, df entropy, and
  a Zipf slope proxy;
- one medium dataset smoke: `fiqa_dev_120`.

Remote run:

- `spark-2`
- `/home/huoju/leask/runs/ii42-m1000-sae-bm25-v2`

Local result copy:

- `/Volumes/Betty/Tmp/ii42-m1000/results-v2`

### Selected Follow-Up Matrix

| Dataset | Variant | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs | Max df ratio |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | d8 q32 dot | 0.1662 | 0.3517 | 0.1799 | 0.0536 | 279.7 | 0.265 |
| nfcorpus | d8 q32 BM25 weighted | 0.1768 | 0.3368 | 0.1775 | 0.0556 | 279.7 | 0.265 |
| nfcorpus | d8 q80 dot | 0.1875 | 0.3774 | 0.1875 | 0.0599 | 512.7 | 0.265 |
| nfcorpus | d8 q80 BM25 weighted | 0.2002 | 0.3966 | 0.1960 | 0.0655 | 512.7 | 0.265 |
| nfcorpus | d64 q80 dot | 0.2630 | 0.4888 | 0.2721 | 0.1010 | 1867.0 | 0.658 |
| nfcorpus | d64 q80 BM25 weighted | 0.2716 | 0.4697 | 0.2705 | 0.1002 | 1867.0 | 0.658 |
| nfcorpus | d64 q80 dfmax0.1 dot | 0.2640 | 0.4782 | 0.2795 | 0.1057 | 1463.5 | 0.658 |
| scifact | d8 q32 dot | 0.9075 | 0.5112 | 0.5518 | 0.4978 | 1198.5 | 0.209 |
| scifact | d8 q32 BM25 weighted | 0.9199 | 0.5373 | 0.5746 | 0.5236 | 1198.5 | 0.209 |
| scifact | d8 q80 dot | 0.9434 | 0.5537 | 0.5959 | 0.5399 | 1812.6 | 0.209 |
| scifact | d8 q80 BM25 weighted | 0.9522 | 0.5727 | 0.6143 | 0.5588 | 1812.6 | 0.209 |
| scifact | d64 q80 dot | 0.9977 | 0.9559 | 0.9539 | 0.9377 | 4084.5 | 0.470 |
| scifact | d64 q80 BM25 weighted | 0.9955 | 0.9322 | 0.9367 | 0.9154 | 4084.5 | 0.470 |
| fiqa | d8 q32 dot | 0.5549 | 0.2914 | 0.2306 | 0.1807 | 10012.8 | 0.088 |
| fiqa | d8 q32 BM25 weighted | 0.5695 | 0.3116 | 0.2373 | 0.1911 | 10012.8 | 0.088 |
| fiqa | d8 q80 dot | 0.5844 | 0.3039 | 0.2543 | 0.2039 | 15600.8 | 0.088 |
| fiqa | d8 q80 BM25 weighted | 0.6009 | 0.3186 | 0.2539 | 0.2030 | 15600.8 | 0.088 |
| fiqa | d64 q80 dot | 0.7519 | 0.4845 | 0.4146 | 0.3554 | 40253.8 | 0.285 |
| fiqa | d64 q80 BM25 weighted | 0.7425 | 0.4700 | 0.3947 | 0.3405 | 40253.8 | 0.285 |

### Follow-Up Readout

The M1000.1 run strengthens the initial conclusion:

- Low-active atom clipping is the most important control.
  `doc_active_k=8` made BM25 weighted consistently useful on scifact and fiqa.
  This is the closest current text analogue to the paper's post-pool top-k
  filtering.

- SAE-BM25 is a real admission/recall signal.
  On fiqa, `d8 q32 BM25 weighted` improves Recall@100 from `0.5549` to
  `0.5695` and MRR@20 from `0.2914` to `0.3116`. On scifact, it improves all
  listed metrics.

- SAE-BM25 is not yet the best high-active top-rank scorer.
  At `d64 q80`, dot wins top-rank metrics on scifact and fiqa. This means BM25
  atoms should not replace dot/scorer directly in high-fanout regimes.

- High-DF filtering is useful as a diagnostic, not a fixed global policy.
  `dfmax0.1` improves nfcorpus `d64 q80 dot` NDCG/MAP while reducing touched
  docs, but it hurts or does not help scifact/fiqa. The right policy is likely
  adaptive to atom distribution and active count, not a hard global cutoff.

- The medium dataset result matters.
  fiqa confirms the low-active BM25 atom benefit is not scifact-only.

## Updated Decision

M1000 should continue as a representation/index route, not merely as a scoring
optimization.

The new likely product shape is:

1. Build a compact SAE atom term space with controlled document fanout.
2. Use BM25/IDF over SAE atoms as first-stage candidate admission.
3. Keep dot or a later learned utility scorer for high-active/top-rank scoring.
4. Feed the clean atom posting evidence into M307-style learned posting scoring
   only after the sparse vocabulary is healthy.

The next experiment should be chunk/sentence pooling. The strongest remaining
difference from BM25-V is that the paper has patch-level features. Our global
embedding atom vectors probably lose local evidence. If segment pooling improves
low-active recall without exploding fanout, M1000 becomes a serious new mainline
candidate.

## M1000.2 Segment Pooling Smoke

M1000.2 tested the closest cheap text analogue of BM25-V patch pooling:

1. Split each document into word segments.
2. Embed each segment with `perplexity-ai/pplx-embed-v1-0.6b`.
3. Encode each segment embedding into SAE atoms.
4. Sum-pool segment atom activations per document.
5. Apply document-level post-pool top-k clipping.
6. Score query global SAE atoms against pooled document atom postings.

Dataset:

- `nfcorpus_dev_120`

Segment setup:

- `segment_words=96`
- `segment_stride=96`
- `max_segments_per_doc=8`
- `segment_active_k=16`
- generated segments: `10,530`

Output:

- `/home/huoju/leask/runs/ii42-m1000-segment-pool-v1`
- local copy: `/Volumes/Betty/Tmp/ii42-m1000/segment-pool-v1`

### Segment Pooling Result

| Surface | Variant | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Global best recall | d64 q80 BM25 weighted dfmax0.25 | 0.2730 | 0.4703 | 0.2703 | 0.1003 | 1739.1 |
| Global best top-rank | d64 q80 dot dfmax0.1 | 0.2640 | 0.4782 | 0.2795 | 0.1057 | 1463.5 |
| Segment post32 | BM25 weighted | 0.2106 | 0.3354 | 0.1871 | 0.0647 | 1325.8 |
| Segment post64 | BM25 weighted | 0.2162 | 0.3460 | 0.1889 | 0.0652 | 1431.3 |
| Segment post96 | BM25 weighted | 0.2162 | 0.3460 | 0.1894 | 0.0652 | 1431.5 |

Within the segment-pooled surface, BM25-over-atoms clearly beats segment dot:

| Segment Surface | Best dot | Best BM25 atom | Readout |
| --- | ---: | ---: | --- |
| post32 q80 | Recall 0.1905 / MRR 0.3306 / NDCG 0.1717 / MAP 0.0552 | Recall 0.2127 / MRR 0.3388 / NDCG 0.1871 / MAP 0.0647 | BM25 scoring helps the pooled atom surface. |
| post64 q80 | Recall 0.1904 / MRR 0.3234 / NDCG 0.1696 / MAP 0.0549 | Recall 0.2162 / MRR 0.3460 / NDCG 0.1908 / MAP 0.0661 | Larger post-pool k helps slightly. |

### Segment Pooling Interpretation

This is a useful partial negative result:

- Segment pooling did not beat global-vector M1000.1 on nfcorpus.
- Segment pooling did validate the paper's internal scoring claim: once we have
  a pooled atom surface, BM25/IDF is much better than raw dot on that same
  surface.
- Increasing post-pool k from `32` to `64/96` improves a little, then saturates.
- The failure is probably not "BM25 over atoms is wrong"; it is that current
  segment embeddings and query-global atoms are not aligned enough.

The likely mismatch is query/document granularity. BM25-V compares image query
patch visual words to image document patch visual words. M1000.2 compared one
global query atom vector to pooled document segment atoms. That may miss local
query evidence and over-emphasize document-side local fragments.

### Updated M1000.2 Decision

Do not promote segment pooling as-is.

However, keep the direction alive because it exposed the right next problem:

- BM25/IDF is useful on pooled atom postings.
- But query-side representation must also become local or evidence-aware.

The next high-value test is therefore not bigger segment pooling. It is one of:

1. Query expansion into phrase/subquery segment atoms, then pooled-query versus
   pooled-document BM25.
2. Segment pooling only for document admission, followed by global dot or M307
   posting scorer for top-rank.
3. Joint lexical+SAE posting surface where lexical BM25 anchors exact query
   terms and SAE segment atoms only contribute semantic expansion.

The current M1000 conclusion is now more precise:

- SAE atoms are viable posting terms.
- BM25/IDF is a real scoring primitive for atom postings.
- The image-paper route does not transfer by simply segmenting documents.
- Text needs query-side locality or lexical anchoring before segment pooling can
  become a mainline replacement.

## M1000.3 Query-Side Pooling And Expansion

M1000.3 tested the missing query-side analogue of BM25-V patch matching.

Dataset:

- `nfcorpus_dev_120`

Artifacts:

- Remote: `/home/huoju/leask/runs/ii42-m1000-query-pool-v1`
- Local: `/Volumes/Betty/Tmp/ii42-m1000/query-pool-v1`

Setup:

- Document surface reused M1000.2 pooled document segments.
- Query phrase cache used PPLX embeddings with `query_segment_words=4`,
  `query_segment_stride=2`, `max_query_segments=8`.
- Query-side segment atoms used `query_segment_active_k=16`.
- Mixed query expansion used `global q80 + weight * segment q64`, with
  weights `0.25`, `0.5`, and `1.0`.

### Query Replacement Result

Replacing global query atoms with phrase-pooled query atoms did not work.

| Query surface | Best variant | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Global query baseline | post64 global q80 BM25 weighted | 0.2162 | 0.3460 | 0.1889 | 0.0652 | 1431.3 |
| Segment query best recall | post32 segment q64 BM25 weighted | 0.1729 | 0.3009 | 0.1637 | 0.0531 | 527.3 |
| Segment query best MAP | post64 segment q64 BM25 weighted | 0.1702 | 0.3091 | 0.1673 | 0.0548 | 582.6 |

Interpretation: short phrase pooling loses too much global query intent. It is
not a replacement for global query atoms.

### Mixed Query Expansion Result

Using phrase atoms as an expansion signal produced a small top-rank gain, but
not a material admission breakthrough.

| Surface | Variant | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Global best recall | post64 global q80 BM25 weighted | 0.2162 | 0.3460 | 0.1889 | 0.0652 | 1431.3 |
| Global best NDCG | post64 global q80 BM25 binary | 0.2154 | 0.3418 | 0.1908 | 0.0658 | 1431.3 |
| Mixed best recall | post64 mixed g80+s64 q96 BM25 binary | 0.2163 | 0.3460 | 0.1926 | 0.0663 | 1441.5 |
| Mixed best NDCG | post64 mixed g80+s64 q128 BM25 binary | 0.2159 | 0.3460 | 0.1931 | 0.0663 | 1442.3 |
| Mixed lower-cost MAP | post64 mixed g80+s64 q128 dfmax0.1 BM25 binary | 0.2090 | 0.3466 | 0.1886 | 0.0664 | 1143.1 |

Readout:

- Phrase expansion improved NDCG@10 from `0.1908` to `0.1931` and MAP@100
  from `0.0658` to `0.0663`.
- Recall@100 was effectively flat: `0.2154-0.2163`.
- Query-weighted BM25 got worse under mixed expansion; binary query BM25 was
  the safer scorer.
- The improvement is too small to justify training or scaling by itself.

### M1000.3 Decision

Do not promote query phrase pooling as a mainline retrieval path.

Keep two useful findings:

- Query phrase atoms are weak as a replacement but can slightly improve
  top-rank when used as a small expansion.
- Binary BM25 over atom terms is more robust than query-weighted BM25 when
  mixing global and phrase atoms.

The next useful M1000 test should shift from query/document phrase pooling to a
true unified posting surface:

1. Lexical BM25 terms anchor exact query intent.
2. SAE atom terms provide semantic expansion.
3. Both are represented as posting terms with df/idf/fanout diagnostics.
4. SAE contribution should be tested first as admission expansion, not final
   ranking replacement.

If this does not beat the global atom baseline on at least `nfcorpus` plus one
medium corpus, M1000 should be parked as an indexing primitive rather than a new
main route.

## M1000.4 Unified Lexical+SAE Posting Surface

M1000.4 tested the more direct version of the paper-inspired route:

1. Lexical tokens are BM25 posting terms.
2. SAE atoms are BM25 posting terms.
3. Both surfaces are combined before any learned scorer.

This is closer to the final II-42 product hypothesis than query/document
pooling: lexical terms anchor exact intent, while SAE atoms add semantic
expansion.

Script:

- `scripts/research_sae_m1000_unified_posting_surface.py`

Artifacts:

- Remote: `/home/huoju/leask/runs/ii42-m1000-unified-posting-v1`
- Local: `/Volumes/Betty/Tmp/ii42-m1000/unified-posting-v1`

Important scope limit:

- These runs use M190 row-build validation surfaces, not official full BEIR
  corpora. They are valid for same-surface direction finding, but they are not
  official full-corpus results.

### nfcorpus Validation Surface

Surface:

- Documents: `3,066`
- Queries: `120`

| Surface | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| Lexical BM25 | 0.1415 | 0.4934 | 0.2634 | 0.0784 | 429.1 |
| SAE BM25 weighted, d64/q80 | 0.2822 | 0.4718 | 0.2698 | 0.1021 | 1598.9 |
| Best unified Recall | 0.2911 | 0.5105 | 0.3012 | 0.1148 | 1441.8 |
| Best unified NDCG | 0.2815 | 0.5557 | 0.3197 | 0.1173 | 1441.8 |
| Best unified MAP | 0.2895 | 0.5470 | 0.3168 | 0.1181 | 1625.5 |

Readout:

- Unified lexical+SAE is clearly better than either individual surface.
- Recall@100 improves over SAE-only from `0.2822` to `0.2911`.
- Top-rank improves much more strongly: NDCG@10 reaches `0.3197`, versus
  lexical `0.2634` and SAE-only `0.2698`.
- The best cost point used high-DF SAE filtering, which reduced touched docs
  while improving metrics.

### fiqa Validation Surface

Surface:

- Documents: `8,057`
- Queries: `120`

| Surface | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| Lexical BM25 | 0.5206 | 0.2874 | 0.2219 | 0.1831 | 7475.1 |
| SAE BM25 weighted, d64/q80 | 0.8376 | 0.4544 | 0.3879 | 0.3317 | 6113.5 |
| Best SAE-only Recall | 0.8397 | 0.4530 | 0.3883 | 0.3315 | 5833.4 |
| Best unified MRR | 0.8242 | 0.5059 | 0.4161 | 0.3555 | 7862.1 |
| Best unified NDCG/MAP | 0.8232 | 0.5056 | 0.4176 | 0.3587 | 7848.6 |

Readout:

- Unified lexical+SAE improves top-rank quality substantially.
- It does not beat SAE-only Recall@100 on fiqa; the best SAE-only row remains
  `0.8397`, while best unified top-rank rows are around `0.823-0.824`.
- The gain is ranking quality: MRR@20 improves from `0.4544` to `0.5059`,
  NDCG@10 from `0.3879` to `0.4176`, and MAP@100 from `0.3317` to `0.3587`.
- This means lexical terms are acting as a useful top-rank calibration signal,
  not merely another recall-expansion source.

### M1000.4 Decision

M1000.4 is the strongest M1000 result so far.

Compared with M1000.2 and M1000.3, unified lexical+SAE postings produce a much
larger and more interpretable gain:

- Document segment pooling alone was negative.
- Query phrase pooling alone was negative.
- Mixed phrase expansion was only a tiny top-rank improvement.
- Unified lexical+SAE postings improved ranking strongly on two validation
  surfaces, and improved recall on `nfcorpus`.

The route should continue, but the next step must be stricter:

1. Run an official full-corpus canary for at least `nfcorpus` and `fiqa`.
2. Replace the simple tokenizer with the real II-42/BM25 tokenization path or
   explicitly document tokenizer differences.
3. Compare against the current BM25, SAE-only, and BM25+SAE product baselines
   on the same corpus.
4. If the full-corpus trend holds, move from score-sum smoke to a real
   unified posting layout with `lex:*` and `sae:*` namespaces.

Do not jump directly to learned scorer training from this result. The important
new evidence is lower-level: lexical tokens and SAE atoms do complement each
other when treated as posting evidence on the same sparse surface.

## M1000.5 Official Full-Corpus Canary

M1000.5 reran the unified lexical+SAE posting surface on official full-corpus
BEIR splits, rather than the smaller M190 validation row surfaces.

Artifacts:

- Remote: `/home/huoju/leask/runs/ii42-m1000-official-unified-v1`
- Local: `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1`

Datasets:

- `nfcorpus`: `3,633` documents, `323` official test queries.
- `scifact`: `5,183` documents, `300` official test queries.
- `fiqa`: `57,638` documents, `648` official test queries.

The fiqa official materialization initially exposed an important qrels/query-id
surface issue: the official test qrels needed canonical `beir15:*:q:*` query
ids, while the reused query embedding file did not contain those rows. The
runner now materializes missing official query embeddings from the prepared
official query texts and validates full qrels/query coverage before evaluation.

### Official Canary Matrix

| Dataset | Surface | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | Lexical BM25 | 0.2263 | 0.5061 | 0.2931 | 0.1274 | 1,296.3 |
| nfcorpus | SAE BM25 weighted d64/q80 | 0.3029 | 0.5090 | 0.3089 | 0.1401 | 2,133.2 |
| nfcorpus | Best unified Recall | 0.3207 | 0.5403 | 0.3425 | 0.1567 | 2,427.0 |
| nfcorpus | Best unified NDCG | 0.3147 | 0.5616 | 0.3508 | 0.1638 | 2,336.8 |
| scifact | Lexical BM25 | 0.8692 | 0.6140 | 0.6471 | 0.6077 | 4,762.6 |
| scifact | SAE BM25 weighted d64/q80 | 0.9577 | 0.7265 | 0.7554 | 0.7123 | 4,092.4 |
| scifact | Best unified Recall | 0.9760 | 0.7580 | 0.7882 | 0.7472 | 5,072.7 |
| scifact | Best unified MAP | 0.9727 | 0.7610 | 0.7883 | 0.7487 | 5,072.7 |
| fiqa | Lexical BM25 | 0.4972 | 0.2881 | 0.2267 | 0.1817 | 49,291.6 |
| fiqa | SAE BM25 weighted d64/q80 | 0.7386 | 0.4992 | 0.4186 | 0.3620 | 39,919.7 |
| fiqa | Best unified | 0.7583 | 0.5249 | 0.4448 | 0.3839 | 54,387.9 |

### Official Canary Readout

The official canary confirms that M1000.4 was not only a validation-surface
artifact:

- `nfcorpus`: unified postings improve both admission and top-rank quality over
  lexical BM25 and SAE-only.
- `scifact`: unified postings improve strongly over already strong SAE-only
  results across all listed metrics.
- `fiqa`: unified postings improve over SAE-only across all listed metrics,
  including Recall@100 from `0.7386` to `0.7583` and NDCG@10 from `0.4186` to
  `0.4448`.

This is the first M1000 result that looks like a real alternate route rather
than a diagnostic-only experiment.

The cost side is the blocker. On fiqa, the best unified row touches about
`54.4k` documents per query, while SAE-only touches about `39.9k`. The unified
surface is better, but it is not yet a deployable first-stage profile.

### Updated M1000 Decision

Continue M1000 as a serious route.

The next target should not be learned scorer training yet. The priority is a
cost-controlled unified admission design:

1. Keep the unified `lex:*` and `sae:*` posting abstraction.
2. Preserve the official canary quality gains.
3. Reduce touched documents/postings with top-k admission, capped per-source
   contribution, or impact-ordered traversal.
4. Only after the cost profile is credible, feed the unified posting evidence
   into a learned M307-style scorer.

The core lesson from the paper now transfers cleanly to text: SAE atoms are more
useful when they are treated as posting terms with corpus statistics, and
lexical BM25 should be part of the same sparse evidence surface rather than a
late mechanical fusion after two independent retrieval systems.

## M1000.6 Bounded Unified Admission

M1000.6 tested whether M1000.5 quality requires scoring the full lexical+SAE
union, or whether a bounded per-source candidate union can preserve the gains.

Implementation:

- Added `--admission-cap` to
  `scripts/research_sae_m1000_unified_posting_surface.py`.
- Each query keeps only the top `N` lexical BM25 docs and top `N` SAE-BM25 docs
  before unified scoring.
- This is still a simulation: source scorers are computed first, then capped.
  It validates the admission shape before implementing impact-ordered traversal.

Artifacts:

- Remote: `/home/huoju/leask/runs/ii42-m1000-official-unified-cap-v1`
- Local: `/Volumes/Betty/Tmp/ii42-m1000/official-unified-cap-v1`

Focused surface:

- `doc_active_k=64`
- `query_active_k=80`
- `sae_df_max_ratio=0`
- `sae_scale=1.0, 2.0`
- `admission_cap=100, 200, 500, 1000, 2000`

### Bounded Admission Curve

The table tracks the strongest M1000.5 row,
`unified_norm_lex_sae2_d64_q80_dfall_weighted`, under per-source caps.

| Dataset | Cap | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | full | 0.3207 | 0.5403 | 0.3425 | 0.1567 | 2,427.0 |
| nfcorpus | 100 | 0.3214 | 0.5366 | 0.3358 | 0.1537 | 146.3 |
| nfcorpus | 200 | 0.3209 | 0.5370 | 0.3391 | 0.1556 | 285.5 |
| nfcorpus | 500 | 0.3216 | 0.5397 | 0.3415 | 0.1564 | 679.3 |
| nfcorpus | 1000 | 0.3206 | 0.5400 | 0.3416 | 0.1566 | 1,263.6 |
| nfcorpus | 2000 | 0.3208 | 0.5404 | 0.3425 | 0.1567 | 2,044.0 |
| scifact | full | 0.9760 | 0.7580 | 0.7882 | 0.7472 | 5,072.7 |
| scifact | 100 | 0.9743 | 0.7445 | 0.7752 | 0.7337 | 174.8 |
| scifact | 200 | 0.9760 | 0.7452 | 0.7773 | 0.7345 | 347.6 |
| scifact | 500 | 0.9767 | 0.7523 | 0.7836 | 0.7412 | 849.3 |
| scifact | 1000 | 0.9760 | 0.7572 | 0.7875 | 0.7462 | 1,626.2 |
| scifact | 2000 | 0.9760 | 0.7580 | 0.7881 | 0.7471 | 2,979.4 |
| fiqa | full | 0.7583 | 0.5249 | 0.4448 | 0.3839 | 54,387.9 |
| fiqa | 100 | 0.7435 | 0.5090 | 0.4204 | 0.3629 | 182.3 |
| fiqa | 200 | 0.7593 | 0.5097 | 0.4263 | 0.3673 | 361.5 |
| fiqa | 500 | 0.7455 | 0.5179 | 0.4339 | 0.3736 | 892.8 |
| fiqa | 1000 | 0.7455 | 0.5203 | 0.4395 | 0.3782 | 1,766.9 |
| fiqa | 2000 | 0.7475 | 0.5223 | 0.4417 | 0.3810 | 3,489.5 |

### Bounded Admission Readout

The result is stronger than expected:

- `nfcorpus`: cap `500` slightly improves Recall@100 and nearly matches full
  MRR/NDCG/MAP while reducing touched docs from `2,427` to `679`.
- `scifact`: cap `1000-2000` is almost indistinguishable from full quality, and
  cap `500` already retains most quality while reducing touched docs from
  `5,073` to `849`.
- `fiqa`: cap `200` slightly improves Recall@100 while using only `361` docs,
  and cap `2000` recovers nearly all top-rank quality while reducing touched
  docs from `54,388` to `3,490`.

This means the M1000.5 quality gain does not require scoring the whole
lexical+SAE union. A bounded sparse admission policy is plausible.

### Updated M1000.6 Decision

Promote bounded unified admission as the next M1000 mainline.

The next implementation step should be impact-ordered traversal rather than
another scorer:

1. Generate top lexical candidates and top SAE candidates independently.
2. Merge them into a bounded candidate set.
3. Apply unified scoring inside that candidate set.
4. Replace this post-hoc cap simulation with real posting traversal that can
   stop early without materializing full source maps.

The current best default candidate cap is not fixed yet:

- `cap500` is a good quality/cost balance on `nfcorpus` and `scifact`.
- `cap1000-2000` is safer for top-rank quality.
- `fiqa` shows that `cap200` can preserve recall, but `cap2000` is safer for
  NDCG/MAP.

The next experiment should therefore test adaptive caps based on query length,
lexical score concentration, SAE atom entropy, and predicted SAE fanout. It
must not use dataset-specific profiles.

## M1000.7 Dense Distance And Active Clipping Diagnosis

M1000.7 answers the immediate question: how far is the M1000 unified
SAE-BM25 route from dense on the same official corpus/query/qrel surface?

Dense baseline artifact:

- Remote:
  `/home/huoju/leask/runs/ii42-m1000-official-unified-v1/dense_baseline_m1000_official.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/dense_baseline_m1000_official.json`

The dense baseline was recomputed from the same M1000 official roots. As a
sanity check, recomputed lexical BM25 exactly matched the M1000 official JSON
metrics, so the query/qrel/doc-id surface is aligned.

### Dense Distance

| Dataset | Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| nfcorpus | dense | 0.3270 | 0.5786 | 0.3581 | 0.1680 |
| nfcorpus | BM25+dense score fusion | 0.3299 | 0.5749 | 0.3556 | 0.1657 |
| nfcorpus | M1000 full | 0.3207 | 0.5403 | 0.3425 | 0.1567 |
| nfcorpus | M1000 cap500 | 0.3216 | 0.5397 | 0.3415 | 0.1564 |
| scifact | dense | 0.9633 | 0.7223 | 0.7479 | 0.7136 |
| scifact | BM25+dense score fusion | 0.9627 | 0.6952 | 0.7298 | 0.6882 |
| scifact | M1000 full | 0.9760 | 0.7580 | 0.7882 | 0.7472 |
| scifact | M1000 cap500 | 0.9767 | 0.7523 | 0.7836 | 0.7412 |
| fiqa | dense | 0.8289 | 0.6017 | 0.5168 | 0.4576 |
| fiqa | BM25+dense score fusion | 0.7981 | 0.5141 | 0.4397 | 0.3729 |
| fiqa | M1000 full | 0.7583 | 0.5249 | 0.4448 | 0.3839 |
| fiqa | M1000 cap500 | 0.7455 | 0.5179 | 0.4339 | 0.3736 |

Readout:

- `scifact` is already beyond dense on every listed metric.
- `nfcorpus` is close on Recall@100 but still behind on MRR/NDCG/MAP.
- `fiqa` is the real remaining blocker. The full M1000 row is behind dense by
  about `0.0706` Recall@100, `0.0769` MRR@20, `0.0720` NDCG@10, and `0.0737`
  MAP@100.

This means the route is not generally weak. It is dataset/query-shape sensitive.
The next work should focus on why `fiqa` misses dense-retrieved relevant docs.

### fiqa Gap Decomposition

Artifact:

- Remote:
  `/home/huoju/leask/runs/ii42-m1000-official-unified-v1/fiqa_dense_gap_diagnostic_m1000.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/fiqa_dense_gap_diagnostic_m1000.json`

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.4972 | 0.2881 | 0.2267 | 0.1817 |
| dense | 0.8289 | 0.6017 | 0.5168 | 0.4576 |
| SAE-BM25 | 0.7387 | 0.4998 | 0.4188 | 0.3622 |
| M1000 unified full | 0.7583 | 0.5249 | 0.4447 | 0.3838 |
| M1000 unified cap500 | 0.7463 | 0.5170 | 0.4338 | 0.3735 |

Coverage view:

| Pool | Mean qrel recall | Query any-hit rate |
| --- | ---: | ---: |
| BM25 top100 | 0.4972 | 0.6991 |
| dense top100 | 0.8289 | 0.9306 |
| SAE top100 | 0.7387 | 0.8719 |
| M1000 unified top100 | 0.7583 | 0.8935 |
| BM25 top100 union SAE top100 | 0.7826 | 0.9090 |
| dense top100 union SAE top100 | 0.8413 | 0.9367 |
| BM25 union SAE union dense | 0.8557 | 0.9460 |

Miss accounting over `1706` qrel instances:

- `171` qrel instances are dense hits but M1000 unified misses.
- Of those, `111` are not in the BM25-or-SAE top100 pool at all.
- `60` are in the BM25-or-SAE pool but lose during unified ranking.
- M1000 also finds `31` qrel instances that dense misses.

Interpretation:

The fiqa gap is mostly an admission/representation gap, not just a final
fusion-score bug. The BM25-or-SAE top100 union reaches only `0.7826` recall,
which is below dense top100 at `0.8289`. A scorer can recover the `60` pool
misses, but not the `111` relevant docs that never enter the sparse pool.

### Active Clipping Sweep

Artifact:

- Remote:
  `/home/huoju/leask/runs/ii42-m1000-active-sweep-v1/fiqa_active_sweep_m1000.json`
- Local:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/fiqa_active_sweep_m1000.json`

Focused sweep:

- `doc_active_k`: `64, 96, 128`
- `query_active_k`: `80, 128`
- `sae_df_max_ratio`: `0`
- `sae_scale`: `2.0`
- `admission_cap`: `500`

Top rows:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| d64 q80 weighted full | 0.7583 | 0.5249 | 0.4447 | 0.3838 | 54,385.4 |
| d64 q128 weighted full | 0.7634 | 0.5286 | 0.4483 | 0.3866 | 54,766.2 |
| d96 q80 weighted full | 0.7619 | 0.5363 | 0.4553 | 0.3932 | 55,166.2 |
| d96 q128 weighted full | 0.7622 | 0.5380 | 0.4581 | 0.3944 | 55,545.0 |
| d128 q128 weighted full | 0.7622 | 0.5380 | 0.4581 | 0.3944 | 55,545.0 |
| d96 q128 weighted cap500 | 0.7492 | 0.5309 | 0.4453 | 0.3813 | 890.1 |

Readout:

- Increasing `query_active_k` from `80` to `128` helps, but only modestly.
- Increasing `doc_active_k` from `64` to `96` helps top-rank metrics.
- Increasing `doc_active_k` from `96` to `128` gives no additional benefit.
  This checkpoint effectively saturates around `doc_active_k=96`.
- Active clipping is not the main fiqa blocker. The best full row improves
  over M1000.5 by only about `+0.0039` Recall@100 and `+0.0135` NDCG@10, still
  far from dense.

### Updated M1000.7 Decision

The current delta is small on `nfcorpus` and favorable on `scifact`, but not on
`fiqa`. `fiqa` proves that global-document SAE atoms still miss dense semantic
evidence that does not enter the BM25-or-SAE top100 pool.

Do not spend more time on larger active budgets. The next credible experiment
is M1000.8 segment/sentence pooling:

1. Split document text into sentence/word windows.
2. Embed each segment with the same PPLX model.
3. Encode segment embeddings through the SAE.
4. Sum-pool segment atoms to document-level atom term frequency.
5. Apply post-pool active clipping.
6. Score with SAE-BM25 and unified lexical+SAE BM25.

This is closer to the paper than global embedding SAE atoms. It tests whether
text needs patch-like local evidence before SAE atoms become a dense-equivalent
first-stage representation.

### M1000.8 Segment/Patch-Level Pooling Canary

Artifacts:

- Remote doc-segment canary:
  `/home/huoju/leask/runs/ii42-m1000-segment-pool-fiqa-v1/fiqa_segment_pool_m4_v1.json`
- Remote query-segment partial canary:
  `/home/huoju/leask/runs/ii42-m1000-segment-pool-fiqa-v2/fiqa_segment_pool_m4_query_v1.partial.log`
- Local doc-segment canary:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/fiqa_segment_pool_m4_v1.json`
- Local query-segment partial canary:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/fiqa_segment_pool_m4_query_v1.partial.log`

Setup:

- Dataset: `fiqa`
- Document segmentation: `96` words, stride `96`, max `4` segments per doc
- Cached document segments: `100,503`
- Segment embedding: `perplexity-ai/pplx-embed-v1-0.6b`
- SAE checkpoint: `bm25sae_stageb_best.pt`
- Segment active atoms: `16`
- Post-pool document active atoms: `64`, `96`
- Query variants:
  - global query atoms
  - query segment atoms: `8` words, stride `4`, max `8`
  - mixed global + query-segment atoms with weights `0.25`, `0.5`, `1.0`

Best completed rows:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| doc segments, global query, weighted atom-BM25 | 0.6325 | 0.3211 | 0.2628 | 0.2118 | 29,652.7 |
| query segments only, weighted atom-BM25 | 0.5147 | 0.2345 | 0.1856 | 0.1504 | 16,550.9 |
| mixed global + query segments, weight 0.25 | 0.6375 | 0.3235 | 0.2630 | 0.2143 | 28,863.2 |
| mixed global + query segments, weight 0.5 | 0.6251 | 0.3091 | 0.2523 | 0.2069 | 28,863.2 |
| mixed global + query segments, weight 1.0 | 0.6066 | 0.2896 | 0.2385 | 0.1951 | 28,862.0 |

Comparison anchors:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| dense | 0.8289 | 0.6017 | 0.5168 | 0.4576 |
| global M1000 full | 0.7583 | 0.5249 | 0.4448 | 0.3839 |
| global active best d96 q128 | 0.7622 | 0.5380 | 0.4581 | 0.3944 |
| segment-pool best mixed | 0.6375 | 0.3235 | 0.2630 | 0.2143 |

Readout:

- Simple document segment pooling is much worse than global SAE atoms on `fiqa`.
- Query segment pooling is worse than global query atoms.
- A small mixed query-segment weight recovers only about `+0.005` Recall@100
  over the doc-segment/global-query row, but remains far below global M1000.
- `post_pool_active_k=64` and `96` produced identical completed doc-segment
  scores, so this canary is not limited by that post-pool budget.

Decision:

Do not promote naive patch-level pooling. The image paper's patch-level result
does not transfer by simply splitting text and passing each segment through the
same global PPLX-to-SAE encoder. The likely mismatch is architectural: image
patch features are native local tokens, while PPLX embeddings are already
global semantic summaries. Segment pooling therefore produces a different atom
space with poorer query/document alignment.

The useful conclusion is narrower:

1. The paper still supports the atom-as-posting / df-idf / BM25-over-atoms
   physical model.
2. Text needs segment-aware atoms or a native token/phrase encoder before
   patch-style pooling can work.
3. The immediate next productive direction is not another segment pooling grid.
   It is either:
   - train a segment-aware SAE/text encoder where segment atoms are first-class
     training targets; or
   - continue the posting-level learned scorer direction, where lexical BM25
     postings and SAE postings are trained together against final
     admission/ranking.

### M1001 Follow-Up: Bounded Traversal And Segment-Max Canaries

This follow-up tested two concrete routes after M1000.8:

1. **Route A: bounded unified posting traversal.** This converts the M1000.6
   bounded-admission idea into a more index-like canary: for each query token
   and SAE atom, read only the top-impact postings, then cap the combined
   candidate pool.
2. **Route B: query-aware segment max/top-k scoring.** This keeps segment
   evidence at segment level instead of naive document sum-pooling, then scores
   documents by the max or top-k segment evidence.

Artifacts:

- Route A script:
  `scripts/research_sae_m1001_impact_traversal.py`
- Route B script:
  `scripts/research_sae_m1001_segment_max_bm25.py`
- Route A remote result:
  `/home/huoju/leask/runs/ii42-m1001-impact-traversal-fiqa-v2/fiqa_impact_traversal_v2.json`
- Route B remote result:
  `/home/huoju/leask/runs/ii42-m1001-segment-max-fiqa-v1/fiqa_segment_max_v1.json`
- Route A local result:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/fiqa_impact_traversal_v2.json`
- Route B local result:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/fiqa_segment_max_v1.json`

#### Route A: Impact-Ordered Bounded Traversal

Setup:

- Dataset: `fiqa`
- Checkpoint: `bm25sae_stageb_best.pt`
- `doc_active_k=64`, `query_active_k=80`
- `sae_scale=2.0`
- `per_token_limit`: `50`, `100`
- `per_atom_limit`: `25`, `50`
- `source_budget`: `200`, `500`

Top rows:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs | Lex read | SAE read |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| tok100 atom50 budget500 | 0.4747 | 0.2550 | 0.2001 | 0.1641 | 932.4 | 1,036.1 | 3,884.9 |
| tok100 atom50 budget200 | 0.4694 | 0.2541 | 0.2003 | 0.1639 | 374.3 | 1,036.1 | 3,884.9 |
| tok50 atom50 budget200 | 0.4633 | 0.2392 | 0.1901 | 0.1540 | 376.1 | 525.3 | 3,884.9 |
| tok50 atom50 budget500 | 0.4603 | 0.2401 | 0.1915 | 0.1554 | 885.8 | 525.3 | 3,884.9 |

Comparison anchors:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Avg touched docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1000.6 bounded cap200 | 0.7593 | 0.5097 | 0.4263 | 0.3673 | 361.5 |
| M1000 full | 0.7583 | 0.5249 | 0.4448 | 0.3839 | 54,387.9 |
| dense | 0.8289 | 0.6017 | 0.5168 | 0.4576 | n/a |

Readout:

- Route A is a clear negative result in this form.
- It touches a similar number of documents to M1000.6 `cap200`, but recall
  collapses from `0.7593` to `0.4694`.
- The problem is not the final source budget; it is the early per-token and
  per-atom posting cutoff. Relevant documents are never admitted because the
  traversal reads high-impact local postings without a safe upper-bound
  expansion policy.
- This means a real impact/WAND-style engine cannot be a simple per-term top-N
  truncation. It needs block upper bounds and adaptive continuation until the
  top-k threshold is safe.

Decision:

Do not promote this bounded traversal canary. Keep the M1000.6 source-budget
admission result as the current bounded evidence, and only revisit traversal
after implementing true block max / WAND-style upper-bound pruning.

#### Route B: Query-Aware Segment Max Scoring

Setup:

- Dataset: `fiqa`
- Cached document segments: `100,503`
- Segment window: `96` words, stride `96`, max `4` segments per doc
- `segment_active_k=16`
- `query_active_k`: `80`, `128`
- Segment aggregation: top `1`, `2`, `4` segment scores per document
- Scorers: `dot`, `bm25_binary`, `bm25_weighted`

Top rows:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Docs touched | Segments touched | Postings read |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| q128 bm25_weighted topseg1 | 0.6492 | 0.3413 | 0.2822 | 0.2338 | 29,652.7 | 43,913.5 | 81,512.1 |
| q80 bm25_weighted topseg1 | 0.6395 | 0.3381 | 0.2785 | 0.2325 | 27,837.7 | 40,930.0 | 73,687.5 |
| q80 dot topseg1 | 0.6304 | 0.3401 | 0.2772 | 0.2300 | 27,837.7 | 40,930.0 | 73,687.5 |
| q128 dot topseg1 | 0.6301 | 0.3404 | 0.2793 | 0.2305 | 29,652.7 | 43,913.5 | 81,512.1 |

Comparison anchors:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| dense | 0.8289 | 0.6017 | 0.5168 | 0.4576 |
| global M1000 full | 0.7583 | 0.5249 | 0.4448 | 0.3839 |
| global active best d96 q128 | 0.7622 | 0.5380 | 0.4581 | 0.3944 |
| M1000.8 segment-pool best mixed | 0.6375 | 0.3235 | 0.2630 | 0.2143 |
| M1001 segment-max best | 0.6492 | 0.3413 | 0.2822 | 0.2338 |

Readout:

- Segment-max is better than naive segment sum-pooling, but still far below
  global M1000.
- `topseg1` is consistently best. `topseg2` and `topseg4` degrade recall and
  ranking, which means multiple weak segment matches inject noise rather than
  recover evidence.
- Increasing query active atoms from `80` to `128` helps only slightly
  (`0.6395` to `0.6492` recall in the best weighted row), so the main bottleneck
  is not query atom budget.
- The best route still touches about `29.7k` docs and `43.9k` segments per
  query, so it is not a cheap first-stage replacement.

Decision:

Do not promote segment-max as a new retrieval route. It is useful evidence that
segment-level max pooling is the only sane segment aggregation shape, but it
does not close the dense gap and does not beat global-document SAE atoms.

The productive next use of segment evidence is narrower:

1. Use segment-max as a diagnostic or rerank feature for sparse candidates.
2. If segment evidence is revisited, train a native segment-aware encoder/SAE
   rather than reusing global PPLX embeddings on arbitrary text windows.
3. Keep M1000's strongest route as unified lexical + global SAE atom postings,
   then focus on true block-bound traversal or learned posting utility.

#### M1001 Overall Review

Both routes are completed and neither replaces the current best M1000 path.

- **Route A failed hard** because early per-posting truncation destroyed
  admission. The lesson is structural: bounded traversal must be WAND/block-max
  style, not fixed top-N per query term.
- **Route B improved the segment route** but remained materially weaker than
  global SAE atoms. The lesson is representational: patch-style text retrieval
  needs a segment-native representation, not post-hoc splitting of a global
  embedding model.

The next useful M1000-family step is therefore not another small route grid.
It should be one of:

1. Implement a real block-max/upper-bound traversal simulator over the unified
   lexical + SAE posting space.
2. Build a segment-native training target where local text spans produce atoms
   that are aligned with query spans.
3. Use segment-max only as an auxiliary feature inside a learned scorer, not as
   a standalone retrieval model.

### M1002 Follow-Up: Block Bounds And Posting Utility

M1002 completed the first item from the M1001 review: implement an exact
upper-bound traversal simulator over the unified lexical + SAE posting space.
It also tested a minimal learned posting-utility canary.

Artifacts:

- Simulator: `scripts/research_sae_m1002_block_wand_simulator.py`
- Utility learner: `scripts/research_sae_m1002_posting_utility_canary.py`
- Baseline artifact:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/fiqa_blockwand_v5.json`
- Utility artifact:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/fiqa_blockwand_utility_v3.json`
- Heldout utility artifact:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/fiqa_posting_utility_v2.json`

Setup:

- Dataset: `fiqa`
- Checkpoint: `ii42-m190-m150-fusionnorm-strictscale-v1`
- `doc_active_k=64`
- `query_active_k=80`
- `sae_scale=2.0`
- Top-k: `100`
- Utility buckets: `lex|sae` x `rare|mid|high`
- Utility split: deterministic query hash split, `328` train queries and `320`
  eval queries

#### Block Traversal Results

Quality is unchanged because both traversal routes are exact top-k simulators.

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| raw full scoring | 0.7279 | 0.4922 | 0.4111 | 0.3512 |
| raw impact-ordered exact | 0.7279 | 0.4922 | 0.4111 | 0.3512 |
| utility full scoring | 0.7320 | 0.5008 | 0.4221 | 0.3602 |
| utility impact-ordered exact | 0.7320 | 0.5008 | 0.4221 | 0.3602 |

Traversal cost:

| Row | Exact top-k | Entry/full ratio | Entry visits | Unique docs | Exact score terms | Block64 decoded/full |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| raw impact-ordered | 648/648 | 0.455 | 116,018.1 | 42,945.0 | 3,920,873.1 | 0.999 |
| utility impact-ordered | 648/648 | 0.407 | 103,742.6 | 40,368.1 | 3,687,512.5 | 0.999 |

Readout:

- Doc-id contiguous block-max is exact but not useful on this surface. Even
  with `block64`, it still decodes about `99.9%` of the full postings.
- Impact-ordered exact frontier traversal is the useful route. It preserves
  exact top-k and reduces sorted entry visits to about `45.5%` of full postings.
- The utility multipliers improve both quality and traversal cost. Entry visits
  drop from about `116k` to `104k` per query, and unique candidate docs drop
  from about `42.9k` to `40.4k`.
- The remaining cost is exact random-access scoring. The simulator currently
  recomputes full query scores for every newly seen doc, producing millions of
  score-term probes per query. A native engine needs a compact doc payload,
  cached term impacts, accumulator reuse, or a stricter MaxScore-style design.

#### Posting Utility Canary

The learned multipliers were:

| Bucket | Multiplier |
| --- | ---: |
| `lex:high` | 0.9812 |
| `lex:mid` | 1.1092 |
| `lex:rare` | 1.1778 |
| `sae:high` | 1.1331 |
| `sae:mid` | 1.3248 |
| `sae:rare` | 1.5342 |

The shape is plausible: rare and mid-frequency SAE atoms receive the largest
boost, rare lexical terms receive a smaller boost, and high-frequency lexical
terms are slightly downweighted.

Heldout split:

| Split | Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| train | before | 0.7402 | 0.5142 | 0.4283 | 0.3621 |
| train | after | 0.7455 | 0.5191 | 0.4411 | 0.3701 |
| eval | before | 0.7154 | 0.4696 | 0.3935 | 0.3401 |
| eval | after | 0.7181 | 0.4820 | 0.4027 | 0.3502 |

This is not just a train-set artifact. The eval split improves on all tracked
metrics, though the learner is still only a coarse source/df-bucket canary.

#### M1002 Decision

Promote the impact-ordered WAND/MaxScore simulator as the next execution
harness for this route.

Do not promote doc-id contiguous block-max. It is exact, but it does not reduce
work on the current FIQA posting surface.

Promote posting utility training as promising, not complete. The next step
should be M1003:

1. Replace the coarse six-bucket multiplier with a richer posting-level utility
   model using source, df/idf, impact, query weight, fanout, and BM25/SAE source
   interaction features.
2. Keep exact impact-ordered traversal as the validation surface so quality and
   traversal cost move together.
3. Add `nfcorpus` and `scifact` before claiming robustness.
4. Prototype a native-friendly scorer layout that avoids full per-doc
   random-access rescoring for every frontier document.

### M1003 Follow-Up: Rich Posting Utility

M1003 implemented the next step from the M1002 decision. The utility key was
expanded from:

```text
source x df_bucket
```

to:

```text
source x df_bucket x posting_impact_bucket
```

where `source` is `lex` or `sae`, `df_bucket` is `rare|mid|high`, and
`posting_impact_bucket` is `low|mid|high` from source-local impact quantiles.

Artifacts:

- Script: `scripts/research_sae_m1003_rich_posting_utility.py`
- Full-strength outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1003`
- Conservative outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1003-strength05`

Setup:

- Datasets: `nfcorpus`, `scifact`, `fiqa`
- Checkpoint: `ii42-m190-m150-fusionnorm-strictscale-v1`
- `doc_active_k=64`
- `query_active_k=80`
- `sae_scale=2.0`
- Candidate rows for learning: `500`
- Train/eval split: deterministic query hash split
- Full-strength run: `multiplier_strength=1.0`
- Conservative run: `multiplier_strength=0.5`

#### Full-Strength Utility

| Dataset | Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `nfcorpus` | before | 0.3141 | 0.5592 | 0.3496 | 0.1631 |
| `nfcorpus` | after | 0.3159 | 0.5577 | 0.3483 | 0.1626 |
| `scifact` | before | 0.9632 | 0.7129 | 0.7464 | 0.7055 |
| `scifact` | after | 0.9632 | 0.7207 | 0.7540 | 0.7120 |
| `fiqa` | before | 0.7279 | 0.4922 | 0.4111 | 0.3512 |
| `fiqa` | after | 0.7345 | 0.5037 | 0.4234 | 0.3622 |

Readout:

- `fiqa` improves more than M1002: `Recall@100 +0.0066`,
  `NDCG@10 +0.0123`, and `MAP@100 +0.0109`.
- `scifact` keeps recall fixed and improves top-rank quality.
- `nfcorpus` gets a tiny recall gain but small MRR/NDCG/MAP regression. This is
  the first sign that the richer utility is useful but too aggressive as a
  fixed policy.

Impact-ordered exact diagnostics:

| Dataset | Row | Diagnostic queries | Entry visits | Unique docs | Exact score terms | Exact top-k |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | after | 323 | 3,143.2 | 1,628.2 | 136,872.7 | 1.000 |
| `scifact` | after | 300 | 9,560.3 | 3,841.6 | 356,225.0 | 1.000 |
| `fiqa` | after | 120 | 115,526.5 | 42,309.1 | 3,876,065.5 | 1.000 |

`fiqa` traversal was capped at `120` diagnostic queries because the Python
simulator spends most of its time doing random-access exact rescoring for
frontier documents. This is a simulator/native-layout blocker, not a quality
blocker.

#### Conservative Utility

`multiplier_strength=0.5` applies the same learned utility in log space, but
pulls multipliers closer to `1.0`.

| Dataset | Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `nfcorpus` | before | 0.3141 | 0.5592 | 0.3496 | 0.1631 |
| `nfcorpus` | after | 0.3149 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | before | 0.9632 | 0.7129 | 0.7464 | 0.7055 |
| `scifact` | after | 0.9632 | 0.7154 | 0.7494 | 0.7064 |
| `fiqa` | before | 0.7279 | 0.4922 | 0.4111 | 0.3512 |
| `fiqa` | after | 0.7297 | 0.4980 | 0.4188 | 0.3570 |

Readout:

- The conservative policy is the safer default: all three datasets move
  non-negatively on the tracked metrics.
- The tradeoff is clear: conservative utility fixes the `nfcorpus` regression
  but gives up part of the `fiqa` and `scifact` upside.
- This suggests the route should not be a hard-coded global multiplier table.
  It needs either a runtime-safe query/corpus feature gate or a native
  posting-level utility model with regularization.

#### M1003 Decision

M1003 is a positive result.

Compared with M1002, the richer feature surface produces larger upside on
`fiqa` and still improves `scifact`. The conservative strength setting also
shows that utility regularization can remove the small `nfcorpus` top-rank
regression.

Promote the following:

1. Keep `source x df_bucket x impact_bucket` as the next utility feature base.
2. Keep `multiplier_strength` or an equivalent regularizer; full-strength is
   useful for upper-bound analysis, but `0.5` is the safer cross-dataset policy.
3. Continue using impact-ordered exact traversal as the correctness harness.

Do not promote the current Python simulator as an efficiency estimate for native
runtime. It proves exactness and exposes sorted-entry savings, but its
random-access rescoring path is too slow and too pessimistic.

Next M1004 work should focus on native-friendly scoring:

1. Replace per-frontier-doc full rescoring with accumulator reuse or compact
   doc-term payloads.
2. Add a lightweight runtime-safe utility gate so `fiqa`-like queries can use
   stronger SAE utility while `nfcorpus`-like queries stay conservative.
3. Validate on at least one larger dataset after the scoring path is optimized.

### M1004 Follow-Up: Accumulator Frontier Scoring

M1004 implemented the first native-friendly scoring check from the M1003
decision. The simulator no longer treats every seen frontier document as a
mandatory full random-access rescore target. Instead, it uses an accumulator
lower bound plus a global remaining-frontier upper bound:

1. Traverse impact-ordered lexical and SAE postings.
2. Maintain an accumulated lower-bound score for seen documents.
3. Stop admission when the remaining frontier cannot beat the current
   lower-bound top-k threshold.
4. Exact-score only documents whose accumulator score plus the remaining
   frontier can still beat that threshold.

This preserves exact top-k because the accumulator is only used for admission.
Final ranking still uses exact lexical + SAE scores for the surviving candidate
documents.

Artifacts:

- Script: `scripts/research_sae_m1004_accumulator_frontier.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1004`
- Utility policy:
  M1003 conservative `multiplier_strength=0.5`

Setup:

- Datasets: `nfcorpus`, `scifact`, and `fiqa`
- Checkpoint: `ii42-m190-m150-fusionnorm-strictscale-v1`
- `doc_active_k=64`
- `query_active_k=80`
- `sae_scale=2.0`
- Top-k: `100`
- `fiqa` was capped at `120` queries, matching the M1003 diagnostic scope.

#### Exactness And Quality

The accumulator path exactly matched full scoring on all tested queries.

| Dataset | Query Scope | Exact Top-k | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 323/323 | 0.3153 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | 300 | 300/300 | 0.9632 | 0.7154 | 0.7494 | 0.7064 |
| `fiqa` | 120 | 120/120 | 0.7474 | 0.5730 | 0.4923 | 0.4213 |

#### Cost Diagnostics

| Dataset | Entry/full ratio | Seen docs | Finalized docs | Pruned seen docs | Exact score terms | Exact terms/seen-all |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 0.468 | 1,728.0 | 1,178.9 | 549.1 | 99,513.7 | 0.691 |
| `scifact` | 0.371 | 3,944.6 | 3,429.9 | 514.7 | 318,380.5 | 0.874 |
| `fiqa` | 0.491 | 43,155.8 | 42,638.3 | 517.5 | 3,906,091.8 | 0.995 |

Readout:

- The impact-ordered accumulator is a correct execution model. It keeps exact
  top-k parity on all tested surfaces.
- It consistently cuts sorted posting entry visits to about `37%` to `49%` of
  full scoring.
- Deferred exact finalization helps on smaller/lower-fanout surfaces, especially
  `nfcorpus`, where exact score terms drop to about `69%` of the seen-doc
  rescore baseline.
- The same bound is too loose for `fiqa`: it prunes only about `518` of `43k`
  seen docs and leaves exact score terms at `99.5%` of the seen-doc baseline.
  This means the remaining bottleneck is not sorted posting admission; it is
  high-fanout exact finalization.

#### M1004 Decision

M1004 is a correctness win and a partial efficiency win, but it is not yet the
native runtime design.

Promote:

1. Keep accumulator + frontier admission as the exactness harness for the unified
   lexical + SAE posting engine.
2. Keep the conservative M1003 utility policy as the current safe scoring shape.
3. Use `entry_vs_full_postings` and `exact_terms_vs_seen_all_terms` as separate
   diagnostics; they expose different bottlenecks.

Do not promote:

1. Do not claim that accumulator finalization solves the large/fanout case.
   `fiqa` shows that exact finalization still dominates.
2. Do not replace exact final scoring with accumulator scores; the accumulator
   path is an admission bound, not the final ranker.

Next M1005 work should focus on the exact-finalization bottleneck:

1. Add a tighter per-document or per-block residual upper bound so many seen
   documents can be safely excluded from exact scoring.
2. Prototype a compact doc-term payload layout so final exact scoring touches
   only query-matched terms instead of probing all query terms for every
   finalized document.
3. Keep `fiqa` as the stress case because it exposes the high-fanout failure
   mode that `nfcorpus` and `scifact` hide.

### M1005 Follow-Up: Per-Document Residual Bounds

M1005 implemented the first M1004 recommendation: replace the global
finalization bound with a per-document residual upper bound.

M1004 used:

```text
doc_upper = accum_score + global_remaining_frontier
```

That is exact but very loose. M1005 tracks a query-term bitmask for every seen
document and uses:

```text
doc_upper = accum_score + sum(remaining_head(term) for term not seen in doc)
```

This remains exact because a document that has already appeared in a term
posting has no further residual contribution from that term. A document that has
not appeared can still contribute at most the current impact-ordered head for
that term.

Artifacts:

- Script: `scripts/research_sae_m1005_per_doc_residual.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1005`
- Utility policy:
  M1003 conservative `multiplier_strength=0.5`

Setup:

- Datasets completed: `nfcorpus`, `scifact`
- Stress attempt: `fiqa` with `120` queries
- Checkpoint: `ii42-m190-m150-fusionnorm-strictscale-v1`
- `doc_active_k=64`
- `query_active_k=80`
- `sae_scale=2.0`

#### Exactness And Quality

The per-document residual path exactly matched full scoring on completed
datasets.

| Dataset | Query Scope | Exact Top-k | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 323/323 | 0.3153 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | 300 | 300/300 | 0.9632 | 0.7154 | 0.7494 | 0.7064 |

#### Cost Diagnostics

| Dataset | Entry/full ratio | M1004 finalized docs | M1005 finalized docs | Extra pruned docs | M1004 exact terms ratio | M1005 exact terms ratio | Compact payload ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 0.468 | 1,178.9 | 1,018.6 | 160.3 | 0.691 | 0.597 | 0.052 |
| `scifact` | 0.371 | 3,429.9 | 3,015.8 | 414.2 | 0.874 | 0.769 | 0.071 |

Readout:

- The per-document residual bound is correct on the completed datasets.
- It gives a real but moderate extra reduction over M1004: exact score terms
  drop by about `12%` to `14%` relative to global-frontier finalization.
- The compact payload diagnostic is much stronger: only about `5%` to `7%` of
  residual exact probes are actual matching query terms. This means a native
  doc-term payload/intersection layout could cut exact scoring far more than the
  bound alone.
- `fiqa` remains the stress blocker. The Python per-doc bitmask simulator was
  stopped after it exceeded the previous M1004 runtime by a wide margin without
  producing the `120`-query JSON. This is not a quality failure, but it is a
  runtime-design warning: the current Python residual-bound implementation is
  too expensive for high-fanout queries.

#### M1005 Decision

M1005 is a useful diagnostic, but not a promoted runtime plan by itself.

Promote:

1. Keep per-document residual bounds as an exact pruning component.
2. Keep `compact_payload_terms` as a required diagnostic. It shows the likely
   native payoff better than exact-probe counts.
3. Treat `fiqa` as the mandatory stress surface before promoting any
   finalization strategy.

Do not promote:

1. Do not promote Python per-doc residual scanning as the execution model. It is
   too slow on `fiqa`.
2. Do not expect residual bounds alone to solve high-fanout finalization. The
   completed datasets only show moderate savings.

Next M1006 work should move from Python-bound simulation to layout design:

1. Add an SAE/lex posting cache so repeated runtime experiments do not rebuild
   document atoms every run.
2. Prototype a compact doc-term payload/intersection scorer and compare it
   against full term-probe exact scoring.
3. Re-run `fiqa` only after the payload path is implemented; otherwise the
   Python simulator will continue to measure itself more than the retrieval
   design.

### M1006 Follow-Up: Compact Payload Finalization

M1006 implemented the layout-oriented follow-up from M1005:

1. Materialize a reusable lexical + SAE posting cache.
2. Build a doc-local compact payload containing all lexical and SAE term impacts
   for each document.
3. Reuse accumulator scores during finalization and only probe missing query
   terms from the compact doc payload.

This is still an exact final scorer. The payload path is not allowed to change
the final top-k order; it only changes how exact scores are recovered for
finalized documents.

Artifacts:

- Script: `scripts/research_sae_m1006_compact_payload.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1006`
- Remote cache example:
  `/home/huoju/leask/runs/ii42-m1006-compact-payload-v1/cache`

Setup:

- Datasets completed: `nfcorpus`, `scifact`, and `fiqa` `q50`
- Stress attempt: `fiqa` `q120`
- Checkpoint: `ii42-m190-m150-fusionnorm-strictscale-v1`
- `doc_active_k=64`
- `query_active_k=80`
- `sae_scale=2.0`
- Utility policy: M1003 conservative `multiplier_strength=0.5`

#### Exactness And Quality

The compact payload path exactly matched full scoring on all completed runs.

| Dataset | Query Scope | Exact Top-k | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 323/323 | 0.3153 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | 300 | 300/300 | 0.9632 | 0.7154 | 0.7494 | 0.7064 |
| `fiqa` | 50 | 50/50 | 0.7084 | 0.5116 | 0.4378 | 0.3721 |

#### Cost Diagnostics

| Dataset | Query Scope | Entry/full ratio | Finalized docs | Legacy terms ratio | Payload probe/legacy | Matched terms/legacy | Reused terms/active |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 0.468 | 1,018.6 | 0.597 | 0.902 | 0.022 | 0.032 |
| `scifact` | 300 | 0.371 | 3,015.8 | 0.769 | 0.949 | 0.039 | 0.032 |
| `fiqa` | 50 | 0.449 | 34,901.9 | 0.842 | 0.966 | 0.030 | 0.033 |

Readout:

- Posting cache works and is important. `fiqa` produced a reusable compact
  payload cache of about `158MB`; subsequent `fiqa` runs skip the expensive
  document-atom materialization.
- Accumulator reuse is correct, but it does not cut enough work by itself.
  Most finalized documents have only about `3%` of active query terms already
  reused from traversal, so the scorer still probes most missing query terms.
- The actual matched term ratio is tiny (`2%` to `4%`), which confirms that
  most finalization probes are negative. The remaining opportunity is not a
  better Python loop; it is a native payload/index layout that can skip negative
  doc-term probes.
- `fiqa q120` with cache was still too slow in the Python simulator and was
  stopped. This confirms that high-fanout `fiqa` is now measuring Python heap
  traversal and payload-probe loops more than the underlying scoring idea.

#### M1006 Decision

M1006 is a useful layout diagnostic and cache foundation, but it is not a
runtime breakthrough.

Promote:

1. Keep the reusable posting/doc-payload cache for all later M1000 runtime
   experiments.
2. Keep accumulator reuse in exact finalization; it is correct and strictly
   better than rescoring from zero.
3. Treat `matched_terms/legacy` as the key native-layout opportunity. The sparse
   intersection is extremely selective, but the current Python payload path
   still pays for almost all negative probes.

Do not promote:

1. Do not promote doc-local hash probing as the final native design. It still
   probes `90%+` of the legacy query-term work on completed surfaces.
2. Do not spend more time optimizing the Python heap traversal path for
   `fiqa q120`; it is the wrong level of implementation.

Next M1007 work should move below Python dictionary simulation:

1. Build a term-major or bitset-style intersection simulator for finalized docs
   that charges only positive matches or compressed intersection operations.
2. Compare that against the current doc-local hash payload to estimate the
   native lower bound.
3. Only after that decide whether this route should become a C/native payload
   prototype.

### M1007 Follow-Up: Term-Major Finalization Lower Bound

M1007 tests the next layout step directly:

1. Keep the M1006 accumulator + residual exactness path.
2. Replace doc-local missing-term probing with term-major intersection over the
   finalized doc set.
3. Report both the Python execution result and the native-layout lower-bound
   cost model.

The scoring contract is unchanged: term-major finalization must exactly match
full scoring top-k. The only question is whether a native term-major or
compressed bitset layout can avoid the negative probes that dominated M1006.

Artifacts:

- Script: `scripts/research_sae_m1007_term_major_finalization.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1007`
- Remote run directory:
  `/home/huoju/leask/runs/ii42-m1007-term-major-v1`
- Input cache:
  `/home/huoju/leask/runs/ii42-m1006-compact-payload-v1/cache`

Setup is identical to M1006:

- Checkpoint: `ii42-m190-m150-fusionnorm-strictscale-v1`
- `doc_active_k=64`
- `query_active_k=80`
- `sae_scale=2.0`
- Utility policy: M1003 conservative `multiplier_strength=0.5`

#### Exactness And Quality

| Dataset | Query Scope | Exact Top-k | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 323/323 | 0.3153 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | 300 | 300/300 | 0.9632 | 0.7154 | 0.7494 | 0.7064 |
| `fiqa` | 10 | 10/10 | 0.7012 | 0.5750 | 0.4950 | 0.3915 |

`fiqa q50` was intentionally stopped because the Python term-lookup prototype
was again measuring interpreter loops. `fiqa q10` is enough to validate the
high-fanout cost shape before moving native.

#### Layout Cost

| Dataset | Query Scope | Entry/full ratio | Native intersection/legacy | Positive/legacy | Positive/native |
| --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 0.468 | 0.074 | 0.022 | 0.299 |
| `scifact` | 300 | 0.371 | 0.083 | 0.039 | 0.471 |
| `fiqa` | 10 | 0.443 | 0.076 | 0.031 | 0.408 |

Compare this with M1006:

- M1006 doc-local payload still probed `90%+` of legacy missing query-term
  work.
- M1007 term-major intersection reduces the native finalization lower-bound to
  roughly `7%` to `8%` of legacy term probes across all tested surfaces.
- Positive intersections are only `2%` to `4%` of legacy probes. This confirms
  that the sparse signal is highly selective, but the physical layout must be
  able to skip negative doc-term checks.

#### M1007 Decision

This is the first M1000 runtime result that looks like a real physical-engine
direction rather than a Python-level optimization.

Promote:

1. Move the runtime design from doc-local hash payloads to term-major or
   compressed bitset-style finalization.
2. Keep accumulator + residual exact admission; it provides exact top-k and
   cuts traversal work before finalization.
3. Use native intersection operations, not Python `doc x term` probing, as the
   main cost model for final exact scoring.

Do not promote:

1. Do not use the current Python M1007 runtime as latency evidence. It still
   builds lookup dictionaries and is single-core interpreter-bound.
2. Do not treat `fiqa q10` as quality evidence. It only validates high-fanout
   cost shape.

Next M1008 should be a compact native-shape prototype:

1. Build sorted doc-id postings or bitset blocks for the M1007 finalization
   phase.
2. Implement exact intersection scoring in C/C++ or a vectorized NumPy/Numba
   proxy before touching PostgreSQL.
3. Re-run `fiqa q50/q120` and compare exactness, intersection ops, and wall
   time against M1006/M1007 Python paths.

### M1008 Follow-Up: NumPy Sorted-Posting Intersection

M1008 turns the M1007 native lower-bound model into an executable
native-shaped proxy:

1. Reuse the same accumulator + residual exact admission path.
2. Convert lexical and SAE postings into sorted `doc_id` NumPy arrays plus
   aligned impact arrays.
3. Finalize scores by vectorized `searchsorted` intersection between each
   active term's posting list and the residual finalized doc set.

The goal is not to prove PostgreSQL latency yet. The goal is narrower: confirm
that term-major intersection is executable, exact on verified slices, and
still preserves the `7%-8%` finalization cost shape from M1007.

Artifacts:

- Script: `scripts/research_sae_m1008_numpy_intersection.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1008`
- Remote run directory:
  `/home/huoju/leask/runs/ii42-m1008-numpy-intersection-v1`

Setup remains identical to M1006/M1007:

- Checkpoint: `ii42-m190-m150-fusionnorm-strictscale-v1`
- `doc_active_k=64`
- `query_active_k=80`
- `sae_scale=2.0`
- Utility policy: M1003 conservative `multiplier_strength=0.5`

#### Exactness And Quality

| Dataset | Query Scope | Verify Mode | Exact Top-k | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | full exact | 323/323 | 0.3153 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | 300 | full exact | 300/300 | 0.9632 | 0.7154 | 0.7494 | 0.7064 |
| `fiqa` | 10 | full exact | 10/10 | 0.7012 | 0.5750 | 0.4950 | 0.3915 |
| `fiqa` | 20 | skip full | n/a | 0.6639 | 0.4993 | 0.4054 | 0.3225 |
| `fiqa` | 50 | skip full | n/a | 0.7084 | 0.5116 | 0.4378 | 0.3721 |

`skip full` means the M1008 path is evaluated directly, but full exact scoring
is not recomputed in the same run. This is only used after exactness is already
validated on `nfcorpus`, `scifact`, and `fiqa q10`, because the Python full
scorer dominates high-fanout `fiqa` wall-time.

#### Layout Cost

| Dataset | Query Scope | NumPy intersection/legacy | Positive/legacy | Positive/intersection |
| --- | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 0.074 | 0.022 | 0.299 |
| `scifact` | 300 | 0.083 | 0.039 | 0.471 |
| `fiqa` | 10 | 0.076 | 0.031 | 0.408 |
| `fiqa` | 20 | 0.074 | 0.029 | 0.394 |
| `fiqa` | 50 | 0.073 | 0.030 | 0.415 |

Readout:

- M1008 reproduces the M1007 cost shape in an executable vectorized path:
  finalization work stays near `7%` to `8%` of legacy term probes.
- Exactness holds on all full-verified surfaces: `nfcorpus`, `scifact`, and
  `fiqa q10`.
- `fiqa q50` no longer needs full exact rescoring to compute the M1008 ranking
  and cost, but the Python accumulator heap traversal remains single-core and
  still dominates wall-time on high-fanout canaries.

#### M1008 Decision

M1008 confirms the term-major direction, but it also shows that finalization
alone is not enough.

Promote:

1. Keep sorted-posting / vectorized-intersection finalization as the preferred
   physical layout.
2. Treat `7%-8%` finalization work as a credible native target.
3. Keep exact parity tests on small/medium surfaces before any larger
   skip-full run.

Do not promote:

1. Do not treat the current Python accumulator + NumPy finalizer as production
   latency evidence.
2. Do not continue optimizing only the finalizer. The next high-fanout
   bottleneck is accumulator traversal and threshold maintenance.

Next M1009 should move both phases into a native-shaped traversal:

1. Replace the Python heap accumulator with an array-backed impact traversal or
   WAND/MaxScore-style block iterator.
2. Keep M1008 sorted-posting intersection for exact finalization.
3. Re-test `fiqa q50/q120`; success means exact top-k plus acceptable wall-time
   without `skip-full` becoming the main proof surface.

### M1009 Follow-Up: Batched Array-Backed Accumulator

M1009 moves the accumulator itself toward a native-shaped execution model:

1. Keep the same impact-ordered traversal and frontier stopping rule.
2. Replace one-entry-at-a-time heap traversal with batched per-term slices.
3. Store accumulator scores in a dense NumPy array instead of a Python score
   dictionary.
4. Keep M1008 sorted-posting intersection for exact finalization.

This is still exact on fully verified slices. Batching may over-read some
entries before the next frontier check, but over-reading is safe: it only adds
evidence before exact residual finalization and never drops candidates.

Artifacts:

- Script: `scripts/research_sae_m1009_batched_accumulator.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1009`
- Remote run directory:
  `/home/huoju/leask/runs/ii42-m1009-batched-accumulator-v1`

Setup:

- Checkpoint: `ii42-m190-m150-fusionnorm-strictscale-v1`
- `doc_active_k=64`
- `query_active_k=80`
- `sae_scale=2.0`
- Utility policy: M1003 conservative `multiplier_strength=0.5`
- `batch_entries=64`

#### Exactness And Quality

| Dataset | Query Scope | Verify Mode | Exact Top-k | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | full exact | 323/323 | 0.3153 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | 300 | full exact | 300/300 | 0.9632 | 0.7154 | 0.7494 | 0.7064 |
| `fiqa` | 20 | full exact | 20/20 | 0.6639 | 0.4993 | 0.4054 | 0.3225 |
| `fiqa` | 50 | skip full | n/a | 0.7084 | 0.5116 | 0.4378 | 0.3721 |
| `fiqa` | 120 | skip full | n/a | 0.7474 | 0.5730 | 0.4923 | 0.4213 |

Compared with M1008, the important change is operational: `fiqa q50` and
`fiqa q120` now complete as practical skip-full canaries. M1008 `fiqa q50`
remained dominated by Python heap traversal and had to be stopped.

#### Layout Cost

| Dataset | Query Scope | Entries/batch | Entry visits/query | Finalized docs/query | Intersection/legacy |
| --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 30.8 | 3,404.1 | 1,041.1 | 0.073 |
| `scifact` | 300 | 49.5 | 9,711.9 | 3,032.7 | 0.083 |
| `fiqa` | 20 | 62.9 | 108,401.4 | 34,707.0 | 0.070 |
| `fiqa` | 50 | 62.4 | 105,701.5 | 35,286.3 | 0.072 |
| `fiqa` | 120 | 62.6 | 113,320.2 | 36,475.8 | 0.068 |

Readout:

- Exactness holds on every full-verified surface, including `fiqa q20`.
- The sorted-posting finalization shape remains stable: intersection work is
  still about `7%` to `8%` of legacy term probes.
- Batching converts high-fanout `fiqa` from an impractical Python-heap canary
  into a practical execution surface.
- Entry visits remain high on `fiqa` at about `100k+` per query. M1009 improves
  interpreter overhead, but it does not yet reduce traversal work enough by
  itself.

#### M1009 Decision

M1009 is the first end-to-end runtime-shape result that should be promoted.

Promote:

1. Keep batched/array-backed accumulator traversal as the next engine shape.
2. Keep M1008 sorted-posting intersection finalization.
3. Use `fiqa q120` as the high-fanout canary for future runtime work; it is now
   practical enough to catch regressions.

Do not promote:

1. Do not stop at batched traversal. It reduces Python overhead but still reads
   too many entries on `fiqa`.
2. Do not use skip-full canaries as exactness proof. Exactness is proven on
   `nfcorpus`, `scifact`, and `fiqa q20`; larger skip-full runs are cost and
   wall-time signals.

Next M1010 should reduce traversal work, not just traversal overhead:

1. Add block-level upper bounds on top of the M1009 array-backed traversal.
2. Skip low-impact blocks before they enter the accumulator.
3. Compare exactness, entry visits, and wall-time against M1009 on
   `nfcorpus`, `scifact`, `fiqa q20`, and `fiqa q120`.

### M1010 Follow-Up: Impact-Order Block Upper-Bound Skipping

M1010 tests a conservative exact block-skip layer on top of M1009:

1. Keep impact-ordered traversal.
2. Process postings in `block_entries=64` blocks.
3. Before accumulating a block, compute a per-doc upper bound:
   current score + the block's exact current-term impact + remaining unseen
   term heads.
4. Skip the block only if every doc in the block is still below the current
   top-k threshold.

This is an exactness-preserving test. It is intentionally conservative: if the
bound is not strong enough, it should process the block rather than risk
dropping a candidate.

Artifacts:

- Script: `scripts/research_sae_m1010_block_skip.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1010`
- Remote run directory:
  `/home/huoju/leask/runs/ii42-m1010-block-skip-v1`

#### Exactness And Quality

| Dataset | Query Scope | Verify Mode | Exact Top-k | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | full exact | 323/323 | 0.3153 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | 300 | full exact | 300/300 | 0.9632 | 0.7154 | 0.7494 | 0.7064 |
| `fiqa` | 120 | skip full | n/a | 0.7474 | 0.5730 | 0.4923 | 0.4213 |

#### Traversal Cost

| Dataset | Query Scope | Check Every | Entry visits/query | Considered entries/query | Skip entry ratio | Intersection/legacy |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 256 | 3,403.6 | 3,404.7 | 0.0009 | 0.073 |
| `scifact` | 300 | 256 | 9,712.1 | 9,712.1 | 0.0000 | 0.083 |
| `fiqa` | 120 | 256 | 113,320.2 | 113,320.2 | 0.0000 | 0.068 |
| `fiqa` | 120 | 64 | 113,217.0 | 113,217.0 | 0.0000 | 0.067 |

Readout:

- Exactness holds on the full-verified surfaces.
- The block upper bound is too loose in the current impact-order traversal.
  It almost never skips blocks.
- Lowering `check_every` from `256` to `64` does not make the route useful; it
  increases threshold-maintenance overhead while still producing no meaningful
  skipping.

#### M1010 Decision

Do not promote this exact block-skip shape.

What this result rules out:

1. Simple impact-order block upper bounds are not enough to reduce entry visits.
2. More frequent threshold checks do not fix the issue.
3. Continuing to tune `block_entries` around this formulation is unlikely to be
   productive.

What remains valuable:

1. M1009 batched accumulator remains the best current runtime shape.
2. M1008 sorted-posting finalization remains the right exact finalizer.
3. The next pruning attempt needs a real MaxScore/WAND-style layout, not a
   bolt-on bound over impact-order traversal.

Next M1011 should change the traversal geometry:

1. Build doc-id-ordered blocks with per-term block max scores.
2. Evaluate a WAND/MaxScore-style pivot over doc blocks or block IDs.
3. Keep exact finalization with M1008/M1009 machinery.
4. Promote only if entry visits drop materially on `fiqa q120` while exactness
   remains verified on smaller full-exact slices.

### M1011 Follow-Up: Doc-Block MaxScore Traversal

M1011 tests the next pruning geometry after M1010 failed:

1. Build doc-id ordered blocks.
2. Store per-term per-block max impact.
3. For each query, sum active-term block max values into a block upper bound.
4. Open blocks in descending upper-bound order, which is a favorable
   MaxScore-style oracle for quickly raising the top-k threshold.
5. Skip the remaining blocks once their upper bound falls below threshold.
6. Score every opened block exactly, using the same sorted posting surface from
   M1008/M1009. No approximate final score is introduced.

This is stricter than a literal block-id pivot WAND canary: if this
upper-bound-sorted MaxScore order cannot skip blocks, a block-id ordered pivot
implementation with the same per-term block max bounds is unlikely to improve
the pruning result. It may reduce metadata construction overhead in a native
engine, but it will not change bound tightness.

Artifacts:

- Script: `scripts/research_sae_m1011_doc_block_wand.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1011`
- Remote run directory:
  `/home/huoju/leask/runs/ii42-m1011-doc-block-wand-v1`

#### Exactness And Quality

| Dataset | Query Scope | Verify Mode | Block Size | Exact Top-k | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | full exact | 512 | 323/323 | 0.3153 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | 300 | full exact | 512 | 300/300 | 0.9632 | 0.7154 | 0.7494 | 0.7064 |
| `fiqa` | 20 | full exact | 128 | 20/20 | 0.6639 | 0.4993 | 0.4054 | 0.3225 |
| `fiqa` | 20 | full exact | 256 | 20/20 | 0.6639 | 0.4993 | 0.4054 | 0.3225 |
| `fiqa` | 20 | full exact | 512 | 20/20 | 0.6639 | 0.4993 | 0.4054 | 0.3225 |
| `fiqa` | 120 | skip full | 512 | n/a | 0.7474 | 0.5730 | 0.4923 | 0.4213 |

#### Traversal Cost

| Dataset | Query Scope | Block Size | Opened Blocks/query | Skip Block Ratio | Decoded Postings/query | Block Metadata/query | Decoded/full |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 512 | 8.0 | 0.0004 | 7,370.5 | 413.4 | 1.000 |
| `scifact` | 300 | 512 | 11.0 | 0.0000 | 27,222.2 | 847.1 | 1.000 |
| `fiqa` | 20 | 128 | 450.0 | 0.0017 | 263,874.6 | 28,327.9 | 1.000 |
| `fiqa` | 20 | 256 | 225.0 | 0.0033 | 263,874.6 | 16,399.2 | 1.000 |
| `fiqa` | 20 | 512 | 113.0 | 0.0000 | 263,899.1 | 9,099.0 | 1.000 |
| `fiqa` | 120 | 512 | 113.0 | 0.0000 | 244,681.6 | 8,848.8 | n/a |

Readout:

- Exactness holds on every full-verified surface.
- The block max bound is far too loose for the current SAE atom fanout shape.
- `fiqa` opens essentially every doc block. Smaller blocks reduce metadata
  granularity but do not reduce decoded postings.
- This is materially worse than M1009 on `fiqa`: M1009 reads about
  `113k` impact entries/query at `q120`, while M1011 block512 decodes about
  `245k` postings/query.
- M1011 confirms that the blocker is not just Python heap overhead. The current
  high-DF active atom distribution makes per-block max upper bounds too high
  across almost every block.

#### M1011 Decision

Do not promote this doc-block MaxScore shape.

What this result rules out:

1. A simple per-term block max layer is not enough for this unified SAE+BM25
   surface.
2. Tuning doc block size around `128/256/512` is unlikely to produce a runtime
   breakthrough.
3. A native block-id pivot WAND implementation with the same upper bounds is
   unlikely to outperform this favorable upper-bound-sorted canary.

What remains valuable:

1. M1009 remains the best current exact traversal shape.
2. M1008 remains the right exact finalization shape.
3. Future pruning must first make the upper bounds tighter, not just rearrange
   traversal order.

Next directions:

1. Use term partitioning or source-aware bounds so high-DF SAE atoms do not
   dominate every block upper bound.
2. Add impact-quantile block metadata, for example block max plus block top-r
   mass, and test whether a safe two-level bound can avoid opening all blocks.
3. Explore lossy candidate admission only as a clearly separated recall/cost
   tradeoff; keep M1008/M1009 as the exact finalization baseline.

### M1012-M1014 Follow-Up: Tightening Block Bounds

M1011 showed that naive per-term block max scores are exact but too loose.
M1012-M1014 test whether the problem is block pruning itself or only the bound
definition.

#### M1012: Block Top-r Head/Tail Bound

M1012 tests a two-level exact bound:

1. Open a doc block.
2. Decode only the top-r postings per active term inside the block.
3. Use the next posting impact as a per-term tail max.
4. Finalize only docs whose head score plus tail frontier can still reach the
   current threshold.

Artifacts:

- Script: `scripts/research_sae_m1012_block_head_tail.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1012`
- Remote run directory:
  `/home/huoju/leask/runs/ii42-m1012-block-head-tail-v1`

`fiqa q20`, block512:

| Head per term | Exact Top-k | Skip Block Ratio | Finalized Docs/query | Work/full |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 20/20 | 0.000 | 55,421.5 | 1.034 |
| 2 | 20/20 | 0.000 | 55,421.5 | 1.065 |
| 4 | 20/20 | 0.000 | 55,421.5 | 1.117 |
| 8 | 20/20 | 0.000 | 55,345.9 | 1.199 |

M1012 readout:

- Exactness holds, but the tail frontier remains too high.
- The route falls back to finalizing almost every touched document.
- Larger head sizes only add work before the exact finalizer.
- Do not promote top-r head/tail as currently defined.

#### M1013: Oracle Exact Block-Max Diagnostic

M1013 is not a runtime proposal. It computes the true max score for each block
from full scores, then simulates ideal block pruning. This measures the upper
limit of block-level pruning if metadata were perfect.

Artifacts:

- Script: `scripts/research_sae_m1013_oracle_block_max.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1013`
- Remote run directory:
  `/home/huoju/leask/runs/ii42-m1013-oracle-block-max-v1`

`fiqa q120`:

| Block Size | Exact Top-k | Skip Block Ratio | Oracle decoded/full | M1011 loose/exact |
| ---: | ---: | ---: | ---: | ---: |
| 512 | 120/120 | 0.414 | 0.589 | 4.62 |
| 64 | 120/120 | 0.895 | 0.110 | 2.72 |

M1013 readout:

- Block-level pruning has real potential. With perfect block max, block64 could
  decode only about `11%` of full postings on `fiqa q120`.
- M1011 fails because the upper bound is loose, not because block pruning is
  inherently useless.
- The gap between naive block max and oracle block max is large enough to
  justify a tighter metadata route.

#### M1014: Membership-Aware Safe Block Bound

M1014 tests a safe bound between M1011 and the oracle:

1. For each active term/block, apply that term's block max only to the docs
   that actually contain the term.
2. Take the max doc-level upper score inside the block.
3. Traverse blocks by this membership-aware upper bound.
4. Finalize opened blocks exactly using the full score surface for diagnostic
   validation.

This simulates a native bitmap/bitset metadata design: the Python canary loops
over doc memberships, but a real engine would evaluate block membership with
bitset word operations.

Artifacts:

- Script: `scripts/research_sae_m1014_membership_block_bound.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1014`
- Remote run directory:
  `/home/huoju/leask/runs/ii42-m1014-membership-block-bound-v1`

`fiqa q120`:

| Block Size | Exact Top-k | Skip Block Ratio | Decoded/full | Membership upper/exact | Bitset word ops/query |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 512 | 120/120 | 0.036 | 0.979 | 1.37 | 70,790.5 |
| 256 | 120/120 | 0.183 | 0.862 | 1.33 | 63,021.1 |
| 128 | 120/120 | 0.458 | 0.601 | 1.29 | 53,630.3 |
| 64 | 120/120 | 0.725 | 0.320 | 1.23 | 43,131.8 |

M1014 readout:

- Exactness holds on the high-fanout `fiqa q120` surface.
- Membership-aware upper bounds are much tighter than M1011's naive block max:
  about `1.2x` to `1.4x` over exact block max instead of `2.7x` to `4.6x`.
- Block64 is the first block-pruning result that materially improves traversal:
  decoded/full drops to `0.320`.
- This does not yet beat the oracle bound (`0.110` at block64), but it is close
  enough to justify a native bitset/block-metadata design.
- The Python canary's `membership_doc_visits/full` is still `1.0` because it
  materializes membership by iterating postings. The relevant native proxy is
  the bitset word-op count, not Python loop work.

#### M1012-M1014 Decision

Promote M1014 as the next engineering direction, not as a finished runtime.

Do not promote:

1. M1012 top-r head/tail finalization.
2. M1011 naive per-term block max.
3. Any lossy admission route yet.

Promote:

1. Doc blocks should be small, with block64 currently the best signal.
2. Store per-term block membership as bitsets or equivalent compressed bitmap
   metadata.
3. Compute membership-aware block upper bounds before exact finalization.
4. Keep M1008/M1009 exact scoring for opened candidate blocks.

Next concrete work:

1. Build an executable bitset simulator that uses word-level operations instead
   of Python per-doc membership loops.
2. Track both word ops and decoded postings against M1009 on `fiqa q120`.
3. If the bitset simulator preserves M1014's decoded savings with acceptable
   metadata cost, move the design into a native payload layout draft.

### M1015 Follow-Up: Word-Level Bitset Membership Bound

M1015 implements the M1014 direction as an executable bitset simulator.

The canary keeps the same safe upper-bound semantics as M1014, but changes the
membership representation:

1. Use block64 so each doc block is one 64-bit word.
2. Store each active term/block membership as a 64-bit mask.
3. Add each term's weighted block max only to the lanes enabled by the mask.
4. Traverse blocks by the resulting membership-aware max lane score.
5. Finalize opened blocks exactly against the full score surface for diagnostic
   validation.

The Python implementation uses NumPy lane updates for clarity, but the cost
model now exposes native-relevant `bitset_word_ops`.

Artifacts:

- Script: `scripts/research_sae_m1015_bitset_membership_bound.py`
- Outputs:
  `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/m1015`
- Remote run directory:
  `/home/huoju/leask/runs/ii42-m1015-bitset-membership-bound-v1`

#### Exactness And Quality

| Dataset | Query Scope | Block Size | Exact Top-k | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 64 | 323/323 | 0.3153 | 0.5623 | 0.3500 | 0.1635 |
| `scifact` | 300 | 64 | 300/300 | 0.9632 | 0.7154 | 0.7494 | 0.7064 |
| `fiqa` | 120 | 64 | 120/120 | 0.7474 | 0.5730 | 0.4923 | 0.4213 |

#### Runtime Proxy

| Dataset | Query Scope | Skip Block Ratio | Decoded/full | Bitset word/full | Bitset word ops/query | Upper/exact |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 323 | 0.210 | 0.847 | 0.195 | 1,438.9 | 1.10 |
| `scifact` | 300 | 0.120 | 0.895 | 0.152 | 4,147.8 | 1.17 |
| `fiqa` | 120 | 0.725 | 0.320 | 0.176 | 43,131.8 | 1.23 |

M1015 readout:

- Exactness holds across all verified surfaces.
- On high-fanout `fiqa q120`, block64 bitset membership preserves M1014's
  decoded savings: `decoded/full=0.320`.
- The metadata-side proxy is now plausible for native execution:
  `bitset_word/full=0.176` on `fiqa q120`.
- Small corpora still show less decoded-posting reduction because the top-k
  threshold cannot prune many blocks, but exactness and word-op accounting are
  stable.

#### M1015 Decision

Promote M1015 as the best current M1000 runtime-engine direction.

This is the first path that satisfies all of these:

1. Keeps exact top-k.
2. Improves traversal work materially on a high-fanout canary.
3. Has a plausible native metadata shape.
4. Preserves M1008/M1009 exact finalization as the correctness floor.

Next concrete work:

1. Draft the native payload layout:
   term -> block id -> membership word, block max, compressed postings.
2. Implement a lower-level C or C-like simulator for block64 bitset scoring.
3. Compare wall time against M1009 on `fiqa q120`, not just decoded/word-op
   proxies.
4. If the C simulator is stable, port the layout into the read-only PostgreSQL
   payload path.
