# SAE M80 Two-Stage DiffSAE Reset Plan

Date: 2026-05-21

Status: active implementation. M80 replaces the M70-M76 fine-tune loop as the
next model-side line. The initial M80-A corpus is neutral and mixed, not a
policy-state-specific corpus.

## Summary

M80 returns to a two-stage design, informed by the local `diffsae` and
`diffsae-codex` repos and by our own M52-M76 evidence.

The current conclusion is:

```text
M70-M76 proved the final BM25+SAE ranking path is executable,
but direct from-scratch text -> atoms final-ranking training keeps collapsing
toward a conservative BM25-preserving regime.
```

The `diffsae` evidence points to a cleaner route:

```text
Stage A:
  text -> strong dense retrieval encoder
       -> sparse SAE code that preserves that dense retrieval geometry

Stage B:
  strong sparse representation
       -> BM25-complement supervised ranking and candidate-budget shaping
```

M80 therefore concentrates first on Stage A. Stage B is intentionally parked
until Stage A proves that the semantic representation is strong enough and the
sparse bottleneck is not the main source of quality loss.

## Why M70-M76 Should Stop

M70-M76 changed losses, teacher residuals, query-family gates, candidate
utility weighting, and row weighting. These experiments produced useful
diagnostics, but not a promotable model:

- M70 from-zero training scaled, but SAE scale decayed and the model stayed
  close to BM25.
- M71 and M72 showed teacher semantic signal is useful locally but too blunt as
  a global final-ranking residual.
- M73 showed query-family diagnostics are useful, while family-gated training
  did not scale.
- M74 showed dense/SAE teacher candidates recover many BM25-missed qrel
  positives, but hard filtering destroys too much training surface.
- M75 showed candidate utility weighting has signal but is too diluted.
- M76 showed query-row weighting is too blunt.

The common failure mode is not lack of another scalar weight. It is that the
underlying text-to-atoms representation is not strong enough before final
BM25+SAE ranking supervision starts. That causes final-ranking training to use
SAE cautiously instead of learning a robust semantic retrieval surface.

## What DiffSAE Adds

The code under `/Users/leask/Documents/II/diffsae` provides a more successful
shape:

- Train a Snowflake-backed dense student encoder with same-document
  multi-positive supervised contrastive learning.
- Keep teacher distillation as a small geometry regularizer, not the main
  objective.
- Train a TopK SAE after the dense student is strong.
- Distill sparse scores from dense scores so sparse retrieval preserves the
  trained dense ranking.
- Use differentiable soft top-k only during training; inference remains hard
  top-k and sparse dot product.

The most important result is that the dense student does most of the semantic
work, and the SAE mostly preserves it:

| Path | Hit@1 | Hit@5 | Hit@10 |
| --- | ---: | ---: | ---: |
| In-DB Snowflake teacher | 56.4% | 89.3% | 91.6% |
| Trained dense student | 70.1% | 92.4% | 94.9% |
| Trained student + SAE, k=32 | 69.4% | 91.6% | 94.5% |
| Trained student + SAE, k=64 | 70.2% | n/a | 94.9% |

This is the missing separation in our current line. We have been asking sparse
atoms and final ranking to learn representation, complementarity, calibration,
and physical cost at the same time.

## What DiffSAE-Codex Adds

The code under `/Users/leask/Documents/II/diffsae-codex` is the stronger M80
engineering reference. It turns the earlier idea into a package with tests,
BEIR preparation, text/Snowflake training, sparse-index serving, and benchmark
paths.

Important pieces to carry into M80:

- `AdaptiveTopKSAE`: fixed or input-dependent TopK with `k_min/k_max`, dynamic
  `k_head`, and soft/straight-through/hard TopK modes.
- `DifferentiableSparseRetriever`: training over candidate sets, not over a
  whole corpus soft TopK.
- `RetrievalTrainingWeights`: separate recall, multi-positive CE,
  teacher-KL, embedding distillation, reconstruction, RAG marginal, and
  k-budget losses.
- `SentenceTransformerTextEncoder`: trainable Snowflake/SentenceTransformer
  wrapper with frozen, last-N-layer, or full train modes.
- `prepare_beir_general.py`: public BEIR normalizer that writes train queries,
  train qrels, benchmark qrels, and candidate rows.
- `train_beir_snowflake_sae.py`: frozen-embedding and live-text SAE trainer
  over mixed BEIR query/document qrels.
- `SparseInvertedIndex`, libtorch backend, and production C++ backend:
  the same sparse score contract can be checked in Python and served through a
  native inverted index.

`diffsae-codex` also gives a useful caution: do not soft-TopK over the whole
corpus. Train over candidate sets containing positives, dense hard negatives,
sparse hard negatives, in-batch negatives, and random negatives, then evaluate
with the hard sparse index.

The verified BEIR `small_general` results are especially relevant because they
are not policy-state-specific. A Stella 16k SAE with k=96 reached sparse
`ndcg@10=0.519`, `recall@10=0.635`, and C++ sparse p50 around `0.34 ms`, above
the dense Snowflake baseline and BM25s baseline on that subset. M80 should use
that style as the neutral harness pattern, while keeping our final model and
data choices open.

## M80 Principle

M80 should not start from a specific policy corpus, and should not start by
optimizing full15 qrel ranking.

Stage A answers a narrower question:

```text
Can we train a strong retrieval representation from text and then compress it
to hard sparse atoms with small quality tax?
```

Only after that is true should Stage B answer:

```text
Can those atoms complement BM25 and improve human relevance ranking under the
actual unified sparse engine?
```

This keeps the representation learning problem separate from the final
BM25+SAE ranking problem.

## M80-A: Representation And Sparse Preservation

M80-A is the immediate focus.

### M80-A0: Neutral Corpus Manifest And Leakage Control

Build a deterministic Stage-A data manifest before training. The first corpus
should be neutral and mixed. Policy-state corpora are useful probes and later
domain-expansion sources, but they should not be the first promotion gate.

Primary sources:

| Source | Role |
| --- | --- |
| M52 balanced Stage-A corpus | existing neutral arXiv/PubMed/BEIR text mix and teacher-imitation evidence |
| BEIR general prepared corpora | public mixed retrieval data with train/benchmark qrels and no policy-domain lock-in |
| M39 validated broad synthetic queries | broad many-positive query style that our earlier hard datasets need |
| arXiv abstracts/chunks | scientific and technical discourse |
| PubMed abstracts/chunks | biomedical terminology and evidence-heavy language |
| Wikipedia or equivalent neutral passages | broad entity and encyclopedic coverage |
| policy chunks | held-out/domain probe first; later expansion after neutral A passes |

Stage-A splits must be by stable group id:

- source document id for long-document chunk groups;
- generated pseudo-document group id for synthetic chunks;
- BEIR dataset/document/query ids for qrel candidate-set rows;
- never by raw row index.

Same-document chunk retrieval is allowed only when groups contain multiple
chunks and held-out groups are unseen. For BEIR-style rows, Stage A should use
candidate-set retrieval and teacher-neighborhood preservation instead of
pretending every corpus row has same-document positives.

Manifest outputs:

- source name;
- document id;
- chunk id;
- text;
- teacher embedding id or materialized teacher embedding;
- split label: `train`, `validation`, `holdout`;
- source-family label for diagnostics;
- group id;
- group construction method: `document_chunks`, `teacher_neighbors`,
  `qrel_candidates`, or `synthetic_neutral`.

Stage-A must keep quality claims separate:

- same-document Hit@k and teacher-neighborhood overlap are allowed;
- full15 Recall/MRR/NDCG/MAP are only sanity checks until Stage B;
- real arXiv/PubMed/policy without qrels remain efficiency or representation
  diagnostics, not product relevance claims.

First M80-A0 smoke target:

```text
neutral_synthetic_mixed_smoke
  sources = BEIR general + M52 arXiv/PubMed/BEIR mix + M39 broad synthetic
  policy = excluded from promotion, optional held-out probe only
```

Implementation status:

- `scripts/research_sae_m80_neutral_corpus.py` now builds the M80-A0 neutral
  candidate-set corpus.
- The first default artifact is
  `/Volumes/Betty/Tmp/ii42_sae_m80/neutral-stage-a-v0`.
- V0 contains 28,351 documents, 540 queries, and 440 candidate rows.
- The detailed artifact report is `sae-m80-a0-neutral-corpus-report.md`.

### M80-A1: Dense Student Encoder

Train a dense retrieval student first.

Architecture baseline:

```text
text
  -> Snowflake/snowflake-arctic-embed-m-v2.0 backbone
  -> CLS pooling
  -> MLP projection head
  -> L2-normalized dense vector
```

Use the `diffsae-codex` text/SAE training design as the first implementation
target, with the older `diffsae` same-document result as supporting evidence:

- Snowflake backbone with `attn_implementation='sdpa'`;
- bf16 autocast with fp32 optimizer state;
- projection head first to 256 dims for apples-to-apples with `diffsae`;
- later control heads at 384/512 only if the 256-dim route saturates;
- no BM25/final-rank loss in this phase.

Loss:

```text
L_A1 =
  supervised multi-positive candidate/group loss
  + small teacher geometry regularizer
  + dense-teacher candidate-set KL
  + optional hard-negative contrastive term after the first stable run
```

Important constraints:

- single-positive InfoNCE is forbidden because same-group positives become
  false negatives;
- teacher distillation is a regularizer, not the main objective;
- qrel ranking is not used in A1, except as an external sanity check.

Promotion gate:

| Gate | Requirement |
| --- | --- |
| Neutral retrieval | dense student improves candidate/group retrieval on the neutral mixed corpus without relying on policy data |
| Generalization | held-out source-family metrics do not show one-source or one-synthetic-generator overfit |
| Teacher geometry | teacher-neighborhood overlap does not collapse; drift must be intentional and correlated with same-doc retrieval gains |
| Training stability | validation peak is tracked and best checkpoint is saved, not just final checkpoint |

Implementation status:

- `scripts/research_sae_m80_dense_student_smoke.py` now validates the M80-A1
  candidate-set loss path with a lightweight token dual encoder.
- The smoke run uses the M80-A0 V0 corpus, 160 train rows, and 73 eval rows.
- The result is intentionally not promotable, but it verifies multi-positive
  candidate CE, dense-teacher KL, source-family metrics, and checkpoint output.
- The detailed smoke report is `sae-m80-a1-dense-student-smoke-report.md`.
- `scripts/research_sae_m80_trainable_dense_student.py` is now the real M80-A1
  trainable text-encoder entrypoint. It supports Snowflake/SentenceTransformer
  backbones, projection heads, frozen/last-N/full train modes, source-family
  diagnostics, and Snowflake-safe attention config defaults.
- The detailed trainable-entrypoint report is
  `sae-m80-a1-trainable-dense-student-report.md`.
- Spark results show that a 256-dimensional random projection head overfits the
  narrow candidate-row supervision and degrades BEIR current-surface quality
  after the first epoch.
- The current promising route is full 768-dimensional Snowflake/GTE geometry
  preservation with `projection=truncate`, low LR last-2-layer tuning, stronger
  embedding-shape regularization, and explicit best-epoch selection. This route
  slightly exceeds the frozen teacher baseline on the M80-A0 V0 candidate eval.
- The current saved M80-A1 checkpoint candidate is
  `/home/huoju/leask/runs/m80-a1-snowflake-last2-truncate768-lr5e6-e2-savebest/trainable_dense_student.best.pt`
  on Spark.

### M80-A2: Dense-To-Sparse SAE Preservation

After A1 passes, freeze or pre-encode the dense student corpus and train the
SAE head. Use candidate sets rather than whole-corpus soft TopK.

Baseline:

```text
d_sae: 4096 first for Snowflake-sized vectors; 8192/16384 only if quality-cost improves
k:     32, 64, 96 first; include 16 only as an aggressive-cost control
```

Loss:

```text
L_A2 =
  KL( dense student retrieval distribution || sparse SAE distribution )
  + optional reconstruction loss
  + optional top-rank pairwise loss over dense-teacher top positions
  + optional multi-positive soft top-k recall loss
  + optional k-budget loss
```

Use hard top-k for evaluation and exported codes. Soft/straight-through top-k
is allowed as a training diagnostic, but current A2.1 evidence shows it does
not automatically improve hard-export ranking. Validation-based best-epoch
selection is required because later epochs can improve Recall@10 while
damaging MRR/NDCG.

Promotion gate:

| Gate | Requirement |
| --- | --- |
| Sparse tax | k=32 sparse Hit@10 within 1 point of dense; Hit@1 within 2 points |
| Capacity curve | k=32/64/96 produces a clear Pareto curve; k=96 should be near-dense |
| Atom health | no severe atom collapse; active atom utilization and fanout are reported |
| Physical cost | sparse postings and payload size are compatible with the evidence-atom engine |

Implementation status:

- `scripts/research_sae_m80_sparse_preservation.py` now implements the first
  A2 harness: load A1 dense checkpoint, encode docs/queries, train TopK SAE,
  and report dense-vs-sparse candidate sparse tax.
- The smoke path is valid, but the first 2048/k32 medium run is not promotable:
  sparse Recall@10/MRR/NDCG@10 drops by roughly `-0.0856/-0.1164/-0.1143`.
- Follow-up A2 diagnostics show 8192/k256 is the current best shared hard-TopK
  result, with sparse tax about `-0.0182/-0.0563/-0.0376` for
  Recall@10/MRR/NDCG@10. This is useful but not promotable, and k256 is too
  expensive as a default.
- A naive independently initialized query/document asymmetric SAE is now parked:
  it breaks latent coordinate alignment and worsens sparse tax. If asymmetric
  sparse preservation is reopened, it needs shared initialization, tied atoms,
  or an explicit coordinate-alignment loss.
- A2.1 implemented soft/ST TopK, top-rank pairwise loss, and hard-eval
  validation selection. Soft/ST is parked as the primary fix because it improves
  coverage more than top-rank quality under hard export. Best-epoch selection
  is positive: `m80-a2-hard-l8192-k256-kl1-pair01-e50-select-ndcg` reaches
  sparse tax about `-0.0154/-0.0278/-0.0177`.
- A2.2 adds balanced selection and sparse checkpoint eval. The current strongest
  practical checkpoint is
  `m80-a2-hard-l8192-k256-kl1-pair01-e50-select-balanced`, with sparse tax about
  `+0.0039/-0.0295/-0.0202`. k192 does not preserve top-rank well enough, and
  simple value transforms (`sqrt`, `log1p`, `binary`) are parked because they
  flatten useful activation magnitude information.
- A2.3 adds calibrated sparse scoring, budget-aware selection reporting, and a
  focused expanded Stage-A surface. The best post-hoc raw-dot calibration only
  moves the V0 k256 tax to about `+0.0039/-0.0289/-0.0193`, so activation
  magnitude helps only marginally. k160 and k224 budget runs fail to approach
  k256 quality, and the focused expanded surface (`36,867` documents,
  `1,013` queries, `812` candidate rows) exposes a larger sparse tax of about
  `-0.0193/-0.0571/-0.0477`, mainly from the broad generated query family.
  Stage B is therefore still blocked.
- The next preferred direction is lowering active budget without losing the
  balanced k256 profile by changing representation/training itself, not by
  post-hoc k/scoring sweeps. Do not continue last-epoch scalar sweeps or use
  Stage B ranking loss to hide weak sparse atoms.
- The detailed A2 report is `sae-m80-a2-sparse-preservation-report.md`.

### M80-A3: Joint Training Control

Only after sequential A1 -> A2 is characterized, run a joint control:

```text
text encoder + SoftTopKSAE trained together
```

Decision rule:

- if joint closes the dense-vs-sparse gap without lowering dense quality,
  promote joint;
- otherwise keep sequential as canonical.

Joint training should not be used as the first M80 implementation because it
mixes representation quality and sparsity bottleneck diagnosis.

### M80-A4: Evidence-Atom Payload Bridge

Once A2 has a good sparse checkpoint, export hard query/doc atoms into the
existing evidence-atom payload harness.

This is a physical-cost bridge, not Stage-B productization.

Report:

- Python/C/PostgreSQL by-id parity if the payload path is used;
- candidate docs;
- SAE postings;
- rerank terms;
- payload MB;
- cached latency;
- atom fanout distribution.

No full product API freeze, mutable index work, or dense-removal claim is
allowed at M80-A.

## M80-B: BM25-Complement Ranking

M80-B starts only after M80-A passes.

Stage B should use what M74-M76 and `diffsae-codex` taught us, but in a cleaner
place:

- keep all normal qrel/BM25 final-ranking supervision;
- add candidate-budget loss for teacher-extra qrel positives missed by BM25;
- use candidate-source diagnostics to mine useful semantic hard positives and
  hard negatives;
- preserve BM25 ranking for lexical-heavy queries;
- train query-side first with fixed document atoms before doc-side finetuning;
- use BEIR/general candidate rows and M39 broad synthetic rows as supervised
  training data before policy-specific supervision;
- only then consider runtime selectors or adaptive export profiles.

Stage B target:

```text
BM25 tokens + SAE semantic atoms
  -> unified sparse engine
  -> candidate coverage and final ranking improve together
```

Stage B must not be used to repair a weak Stage-A encoder. If A1/A2 cannot
produce a strong sparse semantic retriever, Stage B will keep regressing toward
BM25-only behavior.

## Initial M80-A Execution Plan

1. Port or adapt the `diffsae-codex` package interfaces into this repo instead
   of editing `diffsae-codex` in place.
2. Build `M80-A0` manifests for a neutral mixed smoke:
   BEIR general + M52 arXiv/PubMed/BEIR mix + M39 validated broad synthetic.
   Keep policy corpora as optional held-out probes.
3. Run `M80-A1` dense student:
   multi-positive candidate/group contrastive + small teacher regularizer +
   dense-teacher candidate KL.
4. Add hard-negative refresh only after the first dense run reproduces the
   `diffsae` shape.
5. Run `M80-A2` SAE preservation on the best dense checkpoint:
   k=16/32/64 at d_sae=2048.
6. Scale to medium neutral mixed data first, then add policy chunks as
   held-out probes, then as balanced training data only if they do not dominate.
7. Only after sparse tax is low, bridge to evidence-atom physical harness.

## Expected Outputs

M80-A should produce:

- a data manifest report;
- dense student same-doc retrieval report;
- dense-to-sparse SAE preservation report;
- sparsity/cost Pareto report;
- optional joint-vs-sequential control report;
- updated current-status decision.

## Non-Goals

- Do not tune M70-M76 scalar weights further.
- Do not use full15 qrel metrics as the Stage-A promotion metric.
- Do not claim query-time dense dependency is removed.
- Do not freeze SQL/API or mutable index design.
- Do not optimize physical cost before representation quality is proven.

## Decision Criteria

M80-A is a success if it proves:

```text
text -> dense student is strong,
dense student -> hard sparse atoms has small quality tax,
and the exported atoms have a plausible inverted-index cost profile.
```

If M80-A fails, the project should stop treating final-ranking loss tuning as
the primary blocker. The blocker would instead be model architecture, data
quality, or teacher/positive construction for representation learning.

If M80-A passes, M80-B becomes the right place to reintroduce BM25/qrel
supervision and candidate-budget complementarity.
