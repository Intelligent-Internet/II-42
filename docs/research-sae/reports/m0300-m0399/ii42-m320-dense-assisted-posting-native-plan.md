# ii42 M320 Dense-Assisted Posting-Native Atom Encoder Plan

## Summary

M320 tests a stricter version of the current BM25+SAE direction:

```text
text -> encoder -> final posting atoms -> BM25 + latent unified retrieval
```

Dense retrieval is still used as a teacher, but no longer as the primary
representation target. The primary target is final retrieval behavior inside a
posting engine: qrel positives should be admitted and ranked, BM25 false
positives should be suppressed by semantic atoms, and the produced atoms must
have usable df/fanout.

This is different from the older SAE-first route:

```text
text -> dense-like SAE representation -> latent atoms -> post-hoc fusion
```

M320 keeps dense semantic supervision as an auxiliary stabilizer, while making
the exported posting atoms the training object.

## Hypothesis

The current M310 results show that candidate coverage is no longer the main
blocker. The harder problem is top-rank admission/scoring after BM25 and latent
terms meet. A posting-native encoder may improve this because the model learns
which atoms are useful as inverted-index terms, instead of first learning an
SAE reconstruction space and later trying to make that space behave like
postings.

Dense teacher is still important. Without it, direct posting objectives can
overfit local qrels and produce unstable heldout behavior. Therefore M320
should be:

- retrieval-native in the main loss;
- dense-assisted in auxiliary supervision;
- BM25-aware in hard negatives and false-positive suppression;
- fanout-aware before export, not only after export.

## Training Objective

For each training triple `(query, positive_doc, negative_doc)`:

- `negative_doc` is primarily a BM25 hard negative.
- Dense teacher score is computed from the same frozen embedding backbone.
- The encoder outputs query/doc atom vectors.
- Atom vectors are scored through a differentiable BM25-like posting score.

The loss has five parts:

1. **Posting rank loss**
   - Positive atom-BM25 score should beat BM25 hard negatives.
   - This is the main objective.

2. **Dense teacher retention**
   - If dense teacher clearly prefers the qrel positive over the hard negative,
     the atom posting score should preserve that ordering.
   - This protects semantic generalization without forcing atoms to mimic
     dense vectors directly.

3. **Positive atom coverage**
   - Query atoms should overlap with positive document atoms under atom IDF.
   - This avoids learning a scorer that only works through accidental logits.

4. **BM25 false-positive suppression**
   - Query atoms should not broadly overlap with BM25 hard negatives when dense
     and qrels disagree with BM25.
   - This makes latent atoms complement lexical BM25 instead of duplicating it.

5. **Fanout and background overlap penalties**
   - Head atoms and broad background overlap are penalized.
   - The goal is a posting vocabulary that can be stored and queried cheaply.

## M320.0 Canary Scope

The first version is intentionally small and comparable with prior direct
posting work:

- reuse M1050 multi-dataset preparation;
- reuse M1040 atom-BM25 and unified lexical+atom evaluation;
- add dense-assisted retrieval-native loss;
- run on small BEIR datasets first, then expand only if heldout metrics and
  df/fanout move in the right direction.

The M1050 path is a baseline data root only. M320 does not rerun the M1040 or
M1050 training objective. Reusing that root keeps the first canary on the same
document/query/qrel surface as the older direct-posting baseline, so differences
come from the M320 loss rather than from a different split.

The first canary should compare:

- lexical BM25;
- posting-native atom-BM25;
- lexical BM25 + posting-native atoms;
- M1040/M1050 direct-posting baselines where available.

## Promotion Gate

M320 should only continue if it improves at least one of these without a
serious regression:

- heldout atom-BM25 ranking quality over M1040/M1050-style direct posting;
- unified BM25+atom ranking over M310 fixed policies on the same small surface;
- df/fanout compared with M310/M1050;
- query-time posting reads or touched docs at comparable quality.

If atom-only heldout collapses or fanout explodes, stop and redesign the
objective before scaling.

## M320 Canary Outcome

M320 produced a useful but not product-ready result.

What was validated:

- Direct posting-native atom training is trainable.
- Dense/PPLX can remain as an auxiliary semantic admission teacher.
- The admission-style objective can substantially improve atom-BM25 heldout
  quality over M1050 on the same seed-1050 split.

What blocked promotion:

- The best admission model relies on high-DF head atoms and effectively touches
  most or all documents on the canary datasets.
- Making training and evaluation clipping consistent reduces cost but also
  removes useful semantic admission signal.
- Batch DF overflow penalties reduce head pressure, but they are too blunt and
  trade away too much ranking quality.

Therefore M320 should not scale directly to full15. The next version should be
M321, focused on atom utility and final candidate-pool ranking.

## M321 Direction

M321 should not continue generic loss-weight sweeps. The next useful tests are:

1. **Post-hoc atom utility / DF pruning diagnostic**
   - Start from the strongest M320 admission checkpoint.
   - Remove or downweight high-DF atoms by learned or measured utility.
   - Check whether M320 admission quality can be preserved while reducing
     touched docs by at least an order of magnitude.

2. **Final candidate-pool listwise scorer**
   - Use BM25 candidates, M320 atom candidates, dense-hit/BM25-miss positives,
     and high-BM25 false positives.
   - Optimize final top-k admission and top-rank ordering directly.
   - Dense remains a teacher signal, not a runtime dependency claim.

3. **Promotion rule**
   - Do not proceed to full15 unless the canary keeps M320 admission's
     atom-quality advantage and materially reduces cost.
   - If quality only survives at full-corpus fanout, the route remains research
     evidence rather than a product index design.

## M321 Result And M322 Gate

M321 post-hoc DF pruning showed that the M320 admission atoms contain useful
semantic posting signal, but raw DF is too blunt as an export rule:

- `scifact` improved atom-BM25 ranking after pruning atoms above `df<=0.75`
  or `df<=0.50`, while postings dropped sharply.
- `nfcorpus` lost atom-only recall as DF tightened, but unified BM25+atom
  top-rank metrics stayed close.

This means high-DF atoms are mixed: some are useful semantic hubs, some are
noise. M322 should learn or estimate atom utility rather than applying a global
DF cutoff.

M322 gate:

- Build per-atom utility features:
  - qrel-positive contribution;
  - dense-hit / BM25-miss contribution;
  - high-BM25 false-positive overlap;
  - document frequency and posting cost;
  - query activation frequency.
- Evaluate utility-pruned exports on the same seed-1050 surface.
- Continue only if utility pruning keeps the M320 admission quality advantage
  while reducing touched docs/postings by a material factor.

## M322 Result And M323 Gate

M322 changed the next-step conclusion.

The manual utility-pruning variants did not produce a reliable export rule, but
the learned candidate-pool scorer did produce a clear heldout gain when qrel
leakage was removed:

- invalid run:
  `/home/huoju/leask/runs/ii42-m322-candidate-pool-scorer-v1`
- valid run:
  `/home/huoju/leask/runs/ii42-m322-candidate-pool-scorer-v2`

`v1` is invalid because heldout qrel positives were forced into the candidate
pool. `v2` fixes this by only forcing positives for training examples. Heldout
examples are admitted only by BM25 and atom retrieval.

Best canary point:

- `df<=0.25`
- `m322_best`: R@100 `0.6247`, MRR@20 `0.6145`,
  NDCG@10 `0.5167`, MAP@100 `0.4054`
- `m322_scorer` aggregate row: R@100 `0.6090`, MRR@20 `0.6125`,
  NDCG@10 `0.5083`, MAP@100 `0.3934`
- fixed `unified_scale_0.5`: R@100 `0.5425`, MRR@20 `0.5130`,
  NDCG@10 `0.4358`, MAP@100 `0.3332`

This means final BM25+atom admission/ranking is learnable from posting-level
features. The next version should not restart encoder training. It should make
the M322 scorer validation stricter.

M323 gate:

- Keep the M320 admission checkpoint frozen.
- Use `df<=0.25` as the first cost/quality anchor, but keep `df<=0.5` as a
  comparison because it sometimes has smoother training.
- Add early stopping or shorter epochs; M322 often peaked at early epochs.
- Add no-leakage checks to fail if heldout qrel positives are inserted outside
  the BM25/atom candidate union.
- Expand from `nfcorpus`/`scifact` to a small multi-dataset canary before any
  full15 claim.
- Report both candidate-pool upper bound and final scorer quality, so scorer
  gains are not confused with candidate recall changes.

## Non-Goals

- Do not remove dense teacher from training.
- Do not claim dense-free product readiness.
- Do not freeze SQL/API.
- Do not merge this into the M310 evaluator until the canary shows signal.

## First Implementation

Files:

- `scripts/research_sae_m320_dense_assisted_posting_atoms.py`

Expected first run shape:

```bash
python3 scripts/research_sae_m320_dense_assisted_posting_atoms.py \
  --datasets-root /path/to/beir/root \
  --dataset nfcorpus \
  --dataset scifact \
  --output-json /tmp/m320.json \
  --checkpoint-path /tmp/m320.pt \
  --device cuda \
  --rank-epochs 4 \
  --dense-teacher-weight 0.35 \
  --max-train-queries-per-dataset 128 \
  --max-pairs-per-dataset 1000
```

After the canary passes, the next step is a full official-root surface using
the same data integrity checks as the current M310/M190 work.
