# M1000 SAE-BM25 Atom Retrieval Plan

## Goal

Explore whether the BM25-V idea from `2603.05781v1` transfers to text-side
SAE atoms:

- Treat SAE latent atoms as posting terms.
- Treat document SAE activation magnitude as term frequency.
- Compute corpus `df/idf` over SAE atoms.
- Retrieve with BM25 over atom postings before any learned scorer.

This is intentionally separated from the main M307/M3xx learned-fusion line.
The first goal is not to train a new model, but to test whether the current SAE
atom vocabulary already has a healthy BM25-compatible posting structure.

## Paper Mapping

The paper uses ViT patch SAE activations as visual words:

1. Patch features -> SAE top-k atoms.
2. Sum-pool patch activations into image-level term frequencies.
3. Apply post-pool top-k clipping.
4. Compute `df/idf` over visual words.
5. Use BM25 as sparse first-stage retrieval.
6. Optionally rerank the sparse candidate set with dense vectors.

For text we do not yet have patch-token SAE activations. The M1000 smoke uses
global PPLX/Snowflake-style embeddings as the source representation:

1. Query/document embedding -> existing text SAE atoms.
2. Clip document atoms to `doc_active_k`.
3. Clip query atoms to `query_active_k`.
4. Use document atom activation as BM25 term frequency.
5. Use query atoms either as binary terms or query-weighted terms.

The key diagnostic is whether atom `df` is heavy-tailed enough for IDF to help.
If high-DF atoms dominate, learned fusion is probably trying to repair a bad
posting vocabulary.

## M1000.0 Smoke

Dataset:

- `nfcorpus_dev_120`, using existing M190/PPLX 1024 materialized corpus.

Checkpoint:

- `/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1/bm25sae_stageb_best.pt`

Variants:

- `dot`: existing SAE dot-product posting score.
- `bm25_binary_query`: BM25 over SAE atoms; query atom presence only.
- `bm25_query_weighted`: BM25 over SAE atoms, multiplied by normalized query
  atom activation.

Sweeps:

- `doc_active_k`: `16, 32, 64`
- `query_active_k`: `16, 32, 80`

Metrics:

- Recall@100, MRR@20, NDCG@10, MAP@100.
- Atom `df` diagnostics: active atoms, head ratio, Zipf slope proxy, average
  postings touched per query.

Pass Signal:

- BM25 atom retrieval improves over or approaches dot SAE on Recall@100 while
  reducing fanout.
- Stronger signal: BM25 atom retrieval improves NDCG/MAP without learned
  scorer.

Fail Signal:

- BM25 atom retrieval loses large recall vs dot SAE and atom `df` is flat or
  high-DF dominated.
- This would imply global embedding SAE atoms do not behave like BM25-V visual
  words; we would need sentence/chunk/token-level atom pooling before this
  route is fair.

## Non-Goals

- No learned fusion in M1000.0.
- No modification of M307 training outputs.
- No use of spark-1 while the main session is using it.
- No claim about full BEIR15 until at least two small and one medium dataset
  pass this smoke.

## M1000.1 Follow-Up

Status: completed.

Changes:

- Added `doc_active_k=8` to test a more aggressive clipping regime.
- Added `df_max_ratio` sweep to diagnose high-DF atom noise.
- Added richer `df/idf/fanout` diagnostics.
- Added medium dataset smoke on `fiqa_dev_120`.

Decision:

- Continue M1000.
- Treat SAE-BM25 as first-stage admission/recall, not final top-rank scoring.
- Low-active clipping is the strongest control found so far.
- High-DF filtering is diagnostic and possibly adaptive; it is not a universal
  fixed threshold.

## M1000.2 Next Step

The next test must address the biggest gap between the image paper and our text
setup: representation granularity.

BM25-V pools patch-level SAE activations. Our M1000.0/M1000.1 smoke used one
global embedding per document/query. The next experiment should simulate
text-side patching:

1. Split documents into short text segments or reuse existing chunks when
   available.
2. Encode each segment into SAE atoms.
3. Sum-pool segment atoms to document-level term frequencies.
4. Apply post-pool top-k clipping.
5. Score with SAE-BM25 and compare against global-vector M1000.1.

Pass signal:

- Better Recall@100 than global-vector SAE-BM25 at similar or lower touched
  docs.
- No severe NDCG/MAP collapse.
- Healthier df distribution: lower max/head df ratio without losing rare
  positive evidence.

Fail signal:

- Segment pooling explodes touched docs/postings without recall gain.
- Top-rank metrics collapse versus global-vector dot and BM25 weighted.

Only after M1000.2 passes should we integrate this with M307-style learned
posting scoring.

## M1000.2 Result

Status: completed for `nfcorpus_dev_120`.

Result:

- Document segment pooling was implemented with PPLX segment embeddings.
- The pooled segment surface did not beat the global-vector M1000.1 baseline.
- Inside the pooled segment surface, BM25/IDF clearly beat raw dot scoring.

Decision:

- Do not promote document-only segment pooling as-is.
- Keep BM25-over-atoms as a valid primitive.
- The next blocker is query/document granularity mismatch.

M1000.3 should test one of these:

1. Query-side phrase/subquery atom pooling.
2. Segment-pooled document admission followed by global dot or M307 scorer.
3. Unified lexical+SAE posting surface where lexical BM25 anchors query terms
   and SAE segment atoms provide semantic expansion.

The route remains worth exploring, but the breakthrough is not "split documents
and pool atoms" alone.

## M1000.3 Query-Side Pooling Result

Status: completed for `nfcorpus_dev_120`.

Result:

- Query phrase/subquery atom pooling was added.
- Replacing global query atoms with phrase-pooled query atoms was clearly worse.
- Mixing phrase atoms into global query atoms gave only a tiny top-rank gain.
- Binary query BM25 was more robust than query-weighted BM25 when phrase atoms
  were mixed into the query surface.

Decision:

- Do not promote query phrase pooling as a mainline path.
- Keep phrase atoms only as a possible small expansion signal.
- Move the next M1000 step to a true unified posting surface.

## M1000.4 Unified Lexical+SAE Posting Surface

Status: completed smoke.

Rationale:

The paper-inspired direction is not "SAE atoms only". The more product-relevant
shape is:

1. Lexical BM25 terms anchor exact query intent.
2. SAE atom terms add semantic expansion.
3. Both surfaces are represented as sparse posting terms with `tf`, `df`, IDF,
   and fanout/cost diagnostics.

This test deliberately stays below learned scoring. The goal is to determine
whether unified postings are already a better first-stage retrieval primitive
than either lexical BM25 or SAE-BM25 alone.

Implementation:

- Script: `scripts/research_sae_m1000_unified_posting_surface.py`
- Lexical namespace: simple lowercased token BM25 from `documents.jsonl` and
  `queries.jsonl`.
- SAE namespace: current M1000 SAE atom BM25.
- Unified score surfaces:
  - raw score sum: lexical BM25 + `scale * SAE_BM25`;
  - per-query normalized score sum: normalized lexical + `scale * normalized
    SAE_BM25`.

Current smoke limitation:

- The first M1000.4 runs use M190 row-build validation surfaces, not official
  full corpus. They are valid for comparing scoring variants on the same
  candidate/evaluation surface, but they must not be mixed with official
  full-corpus BEIR results.

Pass signal:

- Unified lexical+SAE improves top-rank quality over SAE-only without severe
  Recall@100 loss.
- Stronger signal: unified surface improves Recall@100 and top-rank metrics
  together.

Fail signal:

- Unified surface only adds lexical noise or only works on one narrow dataset.
- SAE-only remains strictly better across Recall/MRR/NDCG/MAP.

Decision rule:

- If `nfcorpus` and at least one medium surface both show top-rank gains, M1000
  should proceed to an official full-corpus canary and then a real posting-level
  unified index design.
- If the gains vanish on full corpus, keep M1000 as a diagnostic/indexing idea
  rather than a product route.

## M1000.5 Official Full-Corpus Canary

Status: completed.

M1000.5 promoted the M1000.4 unified posting surface from validation row
surfaces to official full-corpus BEIR canaries.

Artifacts:

- Remote: `/home/huoju/leask/runs/ii42-m1000-official-unified-v1`
- Local: `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1`

Completed datasets:

- `nfcorpus`: `3,633` docs, `323` official test queries.
- `scifact`: `5,183` docs, `300` official test queries.
- `fiqa`: `57,638` docs, `648` official test queries.

Result:

- Official `nfcorpus`: unified postings beat lexical BM25 and SAE-only on both
  recall and top-rank metrics.
- Official `scifact`: unified postings beat already strong SAE-only rows across
  all listed metrics.
- Official `fiqa`: unified postings beat SAE-only across all listed metrics,
  with Recall@100 `0.7583` and NDCG@10 `0.4448`.

Decision:

- M1000 is no longer only a diagnostic idea. Unified lexical+SAE posting terms
  are a credible alternate retrieval route.
- The remaining blocker is not whether the evidence surfaces complement each
  other. They do.
- The blocker is cost: the best fiqa unified row touches about `54.4k` docs per
  query, which is too high for a deployable first-stage profile.

## M1000.6 Cost-Controlled Unified Admission

Status: completed smoke.

Goal:

- Preserve the M1000.5 official quality gains.
- Reduce touched docs/postings enough for a credible first-stage retrieval
  design.
- Keep M1000 below learned scoring until the sparse admission profile is sane.

Candidate controls:

1. Per-source capped admission.
   Keep top lexical docs and top SAE docs separately, then score only the union.

2. Impact-ordered SAE traversal.
   Iterate SAE postings by high query impact and stop once enough candidate
   evidence has accumulated.

3. Adaptive high-DF suppression.
   Use atom `df/idf` and query atom entropy to suppress very broad SAE atoms
   only when they are likely to add noise.

4. Two-phase sparse admission.
   First build a compact candidate set from lexical BM25 plus low-fanout SAE
   atoms, then apply the full unified score inside that set.

Promotion signal:

- Match or approach M1000.5 quality on `nfcorpus`, `scifact`, and `fiqa`.
- Reduce fiqa average touched docs materially below the current `54.4k`.
- Avoid dataset-specific profiles. The policy must depend on query/corpus
  statistics, not dataset names.

Non-goal:

- Do not train an M307-style scorer until M1000.6 proves that a unified posting
  candidate set can be both high quality and bounded.

Result:

- Per-source capped admission preserved most or all M1000.5 quality on
  `nfcorpus`, `scifact`, and `fiqa`.
- `nfcorpus` cap `500` nearly matched full quality while cutting touched docs
  from `2,427` to `679`.
- `scifact` cap `1000-2000` nearly matched full quality; cap `500` was already
  close while cutting touched docs from `5,073` to `849`.
- `fiqa` cap `200` slightly improved Recall@100 while using only `361` touched
  docs, and cap `2000` nearly recovered full NDCG/MAP while cutting touched
  docs from `54,388` to `3,490`.

Decision:

- Bounded unified admission is now the main M1000 path.
- The remaining work is to replace post-hoc source caps with real
  impact-ordered posting traversal and adaptive cap selection.

## M1000.7 Adaptive Bounded Traversal

Status: next.

Goal:

- Turn the M1000.6 post-hoc cap simulation into a real sparse traversal
  strategy.
- Keep the same unified posting abstraction: `lex:*` terms and `sae:*` atoms.
- Avoid dataset-specific profiles.

Candidate signals:

- Query token count.
- Lexical BM25 score concentration.
- SAE atom entropy/top-atom mass.
- Predicted SAE posting fanout from atom df/idf.
- Early overlap between lexical candidates and SAE candidates.

Candidate policies:

1. Static balanced cap.
   Use one conservative cap such as `500` or `1000` for both sources.

2. Query-adaptive cap.
   Expand SAE cap when lexical concentration is low or SAE entropy is sharp;
   shrink SAE cap when atom fanout is broad.

3. Impact-ordered traversal.
   Walk lexical and SAE postings by upper-bound impact and stop when the
   candidate budget is filled.

4. Rerank-ready evidence payload.
   Store per-candidate lexical score, SAE score, source membership, matched term
   counts, and atom fanout diagnostics so later M307-style scoring can use the
   same bounded candidate set.

Promotion signal:

- Preserve M1000.6 cap quality on `nfcorpus`, `scifact`, and `fiqa`.
- Show materially lower source traversal cost than the current full-source
  simulation.
- Produce a payload shape that can map cleanly into a future II-42 native
  unified posting index.
