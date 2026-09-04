# II-42 Unified Posting Structural Diagnosis And Reset Report

Date: 2026-07-11

## Decision

The unified-posting research goal remains meaningful, but the current narrow
route should stop being treated as a likely path to a breakthrough:

> A single 30,522-dimensional, non-negative, vocabulary-bound SPLADE-style
> vector is unlikely to preserve dense geometry, absorb lexical evidence, and
> remain within the current native posting-cost budget through ordinary
> reconstruction, KD, contrastive, or listwise training alone.

The negative conclusion applies to this representation and training family. It
does not reject one encoder, one posting representation, or one physical
inverted index as product goals.

The next program should move from loss and teacher search to a representation
capacity proof. It should first establish which posting structure can preserve
dense scores under measured native-index constraints, then train an encoder to
reproduce that proven structure.

## Product Boundary Used By This Report

The literature comparison in this report uses a strict product definition. A
complete result must provide all of the following:

- one text encoder family for query and document encoding;
- one physical inverted index containing lexical and latent semantic posting
  namespaces;
- scalar posting impacts accumulated by one additive query-time score;
- no ANN or vector side index;
- no external BM25-plus-ANN candidate union;
- no exact-dense forward rerank;
- no token-level late-interaction stage after candidate generation;
- modern dense-level retrieval quality and cross-corpus generalization under
  measured posting bytes, document frequency, traversal, and latency gates.

A system that relaxes one of these requirements is valuable adjacent evidence,
but it is not a completed precedent for the II-42 product target. Conversely,
the absence of an exact published precedent is not an impossibility theorem or
evidence that no private system exists.

## Current Evidence

The project has tested many distinct mechanisms rather than merely many random
seeds. Relevant families include:

- traditional SAE reconstruction and sparsity;
- dense-to-posting distillation;
- P1 dense-root output compilers;
- signed coordinates and signed-dot scoring;
- token-aware and lexical residual postings;
- selector, gate, and boundary-preservation policies;
- learned vocabulary-sparse/SPLADE controls;
- score-spectrum KD;
- positive-pair contrastive learning;
- hard-negative and cross-teacher listwise supervision;
- native BMP and database replay gates.

Although not every M identifier is an independent training run, the recurring
failure pattern is now consistent across enough independent interventions to
support a structural diagnosis.

### Major Directional Evidence Chain

The following timeline records the principal research hypotheses rather than
every local ablation. Numbers from different surfaces are not treated as
directly comparable; each row is evidence for the stated mechanism only.

| Program phase | Main hypothesis and result | Durable conclusion |
| --- | --- | --- |
| M9-M130: SAE reconstruction and sparse preservation | Traditional SAE/text-to-atom training established real semantic atoms. M27 still missed the direct text-to-atoms gate, while M90 closed a dense-student preservation gate only at `k1024`. M121/M122 later produced the first joint quality/cost improvement, but Recall remained open. | SAE atoms are useful posting primitives, but reconstruction, sparsity, and interpretability losses do not by themselves produce a low-cost dense replacement. |
| M130-M353: BM25/SAE fusion, posting-native admission, and scorers | M310 latent terms reached Recall@100 `0.6831` versus dense `0.7092`, with `96.67%` candidate-hit rate on its taxonomy. M320 made atom retrieval much stronger but expanded posting work by orders of magnitude. M322/M323 and M334-M346 proved that learned interaction scorers beat fixed fusion but still left a large dense and candidate-upper-bound gap. | Candidate and scoring signals are real. Downstream admission/ranking can repair some errors, but cannot establish that the source representation is dense-faithful or selective. |
| M360-M370: dense-faithfulness reset | Existing learned atoms had very low dense top-k overlap. Signed dense coordinates changed the diagnosis: on the shared BEIR15 face, coordinate `k512` reached Recall@100 `0.8518` versus dense `0.8533` and NDCG@10 `0.7745` versus `0.7759`. It touched essentially the whole corpus. | Sparse posting algebra can express dense retrieval. The hard problem is a selective basis and access pattern, not the abstract existence of postings. |
| M392-M601: dense-derived tail and P1/M549U | M396's dense-tail-plus-BM25 route beat same-surface dense+BM25 by NDCG@10 `+0.00529` on MTEB10. M547/M549 preserved broad10 dense behavior at about O@100 `0.994`, and M549U validated the official raw-text PPLX root plus deterministic compiler. M550/M601 showed a fixed BM25 contribution can improve all four retrieval metrics. | Dense-derived posting shapes and lexical complement are valid capability surfaces. They do not yet prove bounded single-hop inverted access; fixed BM25 fusion is a two-signal capability baseline, not completion of the strict product. |
| M599-M730: output compiler and dense-boundary preservation | M599 rejected a locally positive selector on broader validation. M653 found that dense top100 was fully present inside P1 top256 while only about `94.28%` remained in P1 top100. M683 proved oracle support-safe boundary crossing exists. M730's trained two-head boundary/tail compiler still selected epoch 0 because every trained checkpoint damaged dense-equivalence gates. | The desired movement exists, but blind query-side compiler losses cannot safely reproduce it. Lower loss or longer training is not evidence of progress when O@100/O@256 declines. |
| M733-M1338: retrieval-conditioned generated postings | Source-aware teachers and candidate construction improved substantially. M1217's LODO contrastive teacher kept every macro metric positive; M1225's CUB-specific teacher produced stronger native gains; M1251's qrels-free `signed_sum_s1` was macro-positive. Row-level harm remained mixed and was not separable by deployable features; M1338 rejected simple atom-risk penalties. | Useful added atoms and qrels-free movement are not imaginary. The missing capability is row-safe target/harm observability inside the source or objective, not another post-hoc threshold or classifier. |
| M1400-M1518: structural interaction and retrieval-trained sparse controls | M1401 found token MaxSim tail signal but no bounded quality/cost Pareto. M1501 compressed `93.33%` of oracle utility into a sparse bilinear field, while M1502 recovered approximately none of it from held-out text roots. M1510 proved retrieval-trained sparse semantics can beat dense/P1 on some rows, but pooled/grouped sources touched nearly full corpora; post-hoc DF caps and DF-FLOPS did not remove universal support safely. | More expressive interactions and retrieval supervision can create semantics, but free oracle factorability is not text transcodability, and useful pooled sparse semantics remain non-selective. |
| M1520-M1557: URSI, protected tail, and lexical residual | M1520C's 32K token-state codebook met mean-touch budgets but recovered only `0%-35.29%` of dense-recoverable BM25 misses. M1541 showed signed PCA plus a tail sketch can be almost exact after broad accumulation. M1549 then achieved a row-safe official15 Recall gain of `+0.013873`, but by using native VectorChord plus BM25. M1557 moved exact lexical residuals into the P1 posting lifecycle and gained Recall `+0.056119` versus P1, yet remained `-0.116067` behind dense on the comparable official12 surface. | Lexical residual and protected-tail mechanisms are proven. The M1549 capability is not a pure posting solution, and the compact one-index form still lacks semantic candidate access. |
| M1560-M1620: frozen source conversion and balanced discrete routing | No tested frozen-dense route, exact-term source, product cell, or LSH source met O@100 `>=0.95`, O@256 `>=0.90`, and reads `<=0.30x` together. M1600's balanced 4,096-key source oracle reached O@100 `1.0` and O@256 `0.992906` at `0.130539x`, but its trained router reached only `0.824180/0.708773`. M1610 reproduced the source-capacity/query-observability gap with hierarchical and contextual-token routes. | Low-cost sparse covers exist, but local query representations do not reveal the oracle key order. Source capacity must not be reported as deployability. |
| M1630-M1710: engine controls and paper-native learned sparse training | M1630's exact signed engine preserved ranking but opened every block. M1640 showed a mature vocabulary-sparse model has real one-index quality and a more favorable distribution. M1660 then proved that this mature shape executes exactly in official BMP at mean latency `12.288 ms`, retaining `99.8818%` of float Recall. M1650-M1700 found real teacher/contrastive signals that escaped the cost or ordering gates, and M1710 rejected the frozen balanced/residual additive-code compiler before encoder training. | Mature learned sparse retrieval and its exact engine are valid. The unresolved conflict is training a project representation that is both dense-faithful and BMP-friendly at the cost gate. Frozen reconstruction codebooks are not the missing target. |

### What The Program Has Actually Achieved

| Capability | Best evidence | Status |
| --- | --- | --- |
| Native unified-posting infrastructure | Mutable posting lifecycle, exact native/database replay, generation shards, candidate-upper-bound decomposition, BMP controls, bytes/DF/touch accounting, and strict cache/provenance gates | Achieved and reusable independently of the next model |
| Exact practical learned-sparse engine | M1660 official BMP on complete FiQA: exact top100, mean latency `12.288 ms`, p95 `17.499 ms`, serialized index `208.5 MB`, and `99.8818%` float Recall retention | Achieved for the mature OpenSearch vocabulary-sparse root; not yet paired with a project-trained dense-equivalent representation |
| Dense-equivalent posting score when cost is relaxed | M1541 full-union signed PCA plus tail256: NDCG@10 `0.667459` versus exact BGE `0.667473`, Recall@100 `0.766407` versus `0.767131` | Representation expressivity achieved; access cost failed at `13.85` posting entries per corpus document |
| Near-dense deterministic posting transform | M547/M549/M549U preserve broad10 dense quality and approximately `0.994-0.997` top100 overlap on validated surfaces | Dense-derived compiler target achieved; low-touch inverted candidate access not established |
| Selective source capacity | M1600 balanced-code oracle: O@100 `1.000000`, O@256 `0.992906`, reads `0.130539x`, max DF below 1% | Capacity achieved only with dense-target oracle key selection |
| Retrieval-expanded posting movement | M683, M1217, M1225, and M1251 show support-safe or macro-positive query-local posting additions | Teacher/action signal achieved; row-safe deployable selection not achieved |
| Lexical complement to semantic retrieval | M1549 improves official15 Recall@100 by `+0.013873` and MAP@100 by `+0.000371` without NDCG/MRR loss | Capability achieved through separate native dense and lexical access; violates the strict one-posting-path target |
| Compact one-index lexical residual | M1557 improves P1 Recall@100 by `+0.056119` and CUB by `+0.122502` with protected head ranking | Useful product component achieved; absolute semantic gap to dense remains large |
| Learned sparse semantic retrieval | M1510 and M1640 show text-derived vocabulary-sparse models can carry strong retrieval signal in one inverted index | Mechanism achieved; tested roots do not jointly satisfy dense-equivalence, row safety, and native cost gates |

### What Remains Unattained

The project still has no model or deterministic compiler satisfying this
conjunction:

```text
raw text
    -> one query/document encoder
    -> one scalar lexical + latent semantic posting map
    -> one physical inverted index and one additive scoring pass
    -> modern dense-level broad-corpus retrieval
    -> row-safe lexical improvement
    -> bounded DF, posting bytes, traversal, and latency
```

The remaining gaps are now specific rather than qualitative:

| Required property | Closest evidence | Remaining gap |
| --- | --- | --- |
| Dense score and top-k fidelity | Coordinate/full-union signed postings and M549U | Fidelity requires broad accumulation or a dense-derived full support whose posting access is not selective |
| Selective candidate admission | M1600/M1610 source oracles | The useful query key/action order is not observable from deployable text-local features |
| Text-to-posting transcodability | M1501 sparse bilinear oracle retains `93.33%` utility | Nonlinear and ridge students recover approximately `0%` of the held-out gain |
| Row-safe retrieval expansion | M1225/M1251 macro-positive native replay | Harm remains dataset/query dependent and current qrels-free features do not separate it reliably |
| Learned-sparse quality under hard cost | M1640-M1700 mature root and paper-native training | Improving teacher/retrieval fit opens high-DF posting mass or damages global ordering before all gates pass |
| Dense-faithful representation with an efficient exact engine | M1660 proves exact BMP for a mature vocabulary-sparse root; M1630 proves exact scoring for signed dense coordinates | The BMP-friendly root is not dense-equivalent, while the dense-faithful signed representation opens essentially every block |
| Pure one-index reproduction of M1549 | M1557 token-aware residual | Lexical capacity transfers, but the semantic source remains below dense candidate capacity and quality |

The shortest accurate project-state statement is therefore:

> We can express dense behavior in postings when cost is relaxed, and we can
> construct low-cost posting covers when an oracle chooses the keys. We cannot
> yet make a qrels-free text encoder choose and weight those same selective
> postings while preserving dense geometry and row-safe lexical gains.

### M1691: No-Positive KD Pareto

The repaired paper schedule proved that the sparse root can absorb dense or
ensemble teacher information. At step 1,000, quality gates improved, but the
maximum sparse cost reached approximately `1.339x` to `1.469x`. Cost-safe
checkpoints did not preserve all ordering gates.

This rejected an optimizer-only explanation. Better teacher fit opened posting
mass, especially high-document-frequency terms.

### M1700: Positive Curriculum Pareto

M1700 introduced explicit positives and exact GradCache with an effective
negative pool of 2,048. The mechanism produced a real heldout signal:

- broad-2,048 InfoNCE improved from `0.015496` to `0.011344` after four
  updates;
- broad top1 improved from `0.996216` to `0.997925`;
- RLHN pairwise accuracy improved slightly;
- gradients were finite and peak reserved memory was only `5.85%`.

However, the formal ordering/cost ladder exposed a narrow safe region:

| Checkpoint | Ordering/cost gate | Maximum cost ratio | Main failure |
| --- | --- | ---: | --- |
| Step 4 | pass | `1.090x` | top1 already at `-0.004883`, close to the `-0.005` floor |
| Step 10 | fail | `1.200x` | top1 and cost |
| Step 25 | fail | `1.257x` | top1 and cost |
| Step 50 | fail | `1.223x` | top1, pairwise, Spearman, and cost |

This is evidence against undertraining. The positive-only objective improves
its local surface while quickly moving outside the global ordering and native
cost feasible region.

### M1701: Teacher Strength Is Not The Root Cause

The first M1701 MiniLM cross-encoder teacher was informative but failed the
predeclared observability gate:

- root positive top1: `0.747559`;
- teacher positive top1: `0.770996`;
- top1 gain: `+0.023438`, below the required `+0.05`;
- teacher pairwise accuracy: `0.953223`;
- pairwise gain: `+0.015918`, which passed its floor.

A stronger RankT5 teacher remains a valid diagnostic. It should not be treated
as a likely architectural rescue. A stronger target cannot guarantee that the
current sparse representation can realize the target while retaining its DF,
ordering, and traversal constraints.

### M1710: Frozen Residual Codes Fail Before Encoder Training

M1710 subsequently tested the most direct deterministic codebook instance of
the reset. It used frozen dense vectors, balanced first-order codes, a second
residual codebook, no qrels, and no text-encoder training. The exact rotated
dense control reproduced the cached dense ranking, so the audit surface was
healthy.

The first-plus-residual all-code ceiling reached only:

- O@10 `0.497600`;
- O@100 `0.524210`;
- O@256 `0.559961`;
- score Pearson `0.820708`.

Even exact-dense reranking over the largest bounded first-order touched set
reached only O@100 `0.917890` and O@256 `0.841230`. M1710 therefore identified
both a representation error and an access/admission error. It closes reuse of
the frozen M1600 balanced/residual codebook as an encoder target. It does not
close retrieval-oriented quantization, a learned latent sparse basis, or every
possible one-index representation.

## Public Literature Boundary And Adjacent Evidence

As of 2026-07-11, this review found no public result that satisfies every item
in the strict product boundary above. The architectural core is not new,
however. Prior work has succeeded on several neighboring formulations, and the
constraint each system relaxes is itself useful evidence.

| Work | What it establishes | Why it is adjacent rather than complete |
| --- | --- | --- |
| SNRM (`3269206.3271800`) | Learns high-dimensional latent sparse query and document vectors, treats each dimension as a latent term, builds an inverted index, and scores with a dot product. This is the closest early architectural ancestor. | It predates modern dense retrievers, uses query-likelihood weak supervision, and does not establish dense isometry, lexical-plus-latent unification, broad zero-shot quality, or the current native cost gates. |
| SPLADE-v3 (`2403.06789`) | Demonstrates that one vocabulary-sparse vector and one inverted index can be highly competitive across more than 40 query sets. | Its coordinates remain vocabulary-bound. Strong learned-sparse retrieval is not evidence that the same representation is a lossless image of an arbitrary dense space. |
| SPARTA (`2009.13013`) | Shows scalable neural sparse retrieval without ANN and reports stronger results than its dense counterpart on its OpenQA/ReQA surfaces. | Its representation is asymmetric and based on token-level query-to-answer interaction, not one shared scalar latent-posting bi-encoder. |
| COIL (`2104.07186`) | Stores contextualized token representations in exact-lexical inverted lists and combines lexical routing with neural semantics at competitive latency. | Posting payloads are contextual vectors and interaction is token-level; this is not scalar posting accumulation. |
| CITADEL (`2211.10411`) | Learns dynamic lexical routing keys for token vectors and reports ColBERTv2-level or better quality with much faster retrieval. | It preserves multi-vector token interaction under each routing key rather than collapsing semantics into one sparse scalar vector. |
| ColBERTv2 / PLAID (`2112.01488`, `2205.09707`) | Residual compression, centroid interaction, and pruning retain strong token-level retrieval quality at practical scale. | Their success depends on multi-vector structure and staged centroid/residual evaluation, which relaxes the single-hop additive-score requirement. |
| RepCONC / Distill-VQ (`2110.05789`, `2204.00185`) | Show that retrieval-oriented, jointly trained, and load-balanced discrete codes preserve dense retrieval better than reconstruction-only quantization. | They use PQ/IVF approximate vector search, not a unified lexical-semantic scalar posting index. |
| Anisotropic VQ (`1908.10396`) | Shows mathematically and empirically that MIPS quantization must penalize score-relevant residual directions differently from ordinary isotropic reconstruction. | It improves vector quantization rather than proving that bounded scalar postings can express the resulting search. |
| Searching Dense Representations with Inverted Indexes (`2312.01556`) | Demonstrates that dense-vector retrieval can be encoded into a conventional inverted-index mechanism with reasonable effectiveness and compact indexes. | The authors characterize the resulting traversal as impractically slow, so it fails the product trade-off. |
| Seismic (`2404.18812`) | Uses geometrically cohesive blocks and summary vectors to obtain fast approximate retrieval over learned sparse embeddings. | It solves organization and access for an existing sparse representation; it does not prove dense-to-posting representation capacity and uses approximate block admission. |

### What The Literature Actually Supports

The exact precedent base is small, but the component evidence is not. The
closest successful systems repeatedly relax at least one axis:

- SNRM and SPLADE retain a scalar sparse dot product but do not preserve a
  specified modern dense geometry;
- COIL, CITADEL, ColBERTv2, and PLAID retain more semantic structure through
  vector-valued postings or multi-vector interaction;
- RepCONC and Distill-VQ preserve dense information through retrieval-oriented
  VQ but retain an ANN/IVF execution model;
- direct dense-to-inverted conversion retains the physical index abstraction
  but loses the required latency trade-off;
- Seismic improves sparse-index execution after a suitable representation
  already exists.

This pattern supports a representation-first reset and argues against another
ordinary SPLADE loss sweep. It also narrows the remaining strict route: a new
representation must be score-aware, retrieval-oriented, explicitly
load-balanced, and co-designed with the index. Reconstruction-only codebooks,
post-hoc scalar pruning, and average sparsity regularization are already
insufficiently supported by both local and published evidence.

## Structural Diagnosis

### 1. Quality And Cost Are Coupled Through Document Frequency

The model commonly gains semantic coverage by activating terms that apply to
many documents. This improves local positive separation and teacher fit, but it
also lengthens posting lists and changes full-corpus score geometry.

Ordinary FLOPS regularization controls expected activation, not the worst
document frequency of individual terms. Therefore a model can satisfy an
average sparsity pressure while concentrating mass in a small number of very
expensive terms.

#### Post-v3 deployment evidence and training constraint

The sole-authority v3 rebuild makes this concern measurable in the product
engine rather than only in an offline simulator. On the complete 57,638-document
FiQA corpus, 30 warm public `ii42_query(...)` calls ranged from 48.191 ms to
492.260 ms. Fast queries decoded 66,932-207,239 postings and scored 746-6,337
documents. Slow queries decoded 379,087-691,470 postings and scored
52,698-56,854 documents. Query encoding remained about 34-38 ms; exact
high-DF accumulation dominated the tail.

This is consistent with published learned-sparse evidence:

- [DF-FLOPS](https://arxiv.org/abs/2505.15070) shows that ordinary FLOPS
  controls row sparsity but not term-level document frequency, and reports an
  approximately 10x production-engine latency reduction from directly
  penalizing high-DF terms.
- [Guided Traversal](https://arxiv.org/abs/2204.11314) reports average posting
  lengths of 21K-72K for WordPiece learned-sparse terms versus 100-178 for
  word-level sparse terms, and attributes poor dynamic pruning to broad,
  high-impact learned terms.
- [Block-Max Pruning](https://arxiv.org/abs/2405.01117) demonstrates that
  learned-sparse-specific block filtering can accelerate safe retrieval, but
  only when block bounds separate competitive from noncompetitive regions.
- [Seismic](https://arxiv.org/abs/2404.18812) obtains much larger gains from
  geometrically cohesive blocks and summary vectors, but its admission is
  approximate rather than an exact product replacement.
- [GPUSparse](https://arxiv.org/abs/2606.26441) shows that exhaustive exact
  sparse accumulation can instead be made bandwidth-efficient on a GPU. That
  is useful adjacent evidence, not the default PostgreSQL single-query path.

Future training must therefore treat corpus DF as a first-class checkpoint
gate, not a post-training observation. Every candidate checkpoint must report:

1. maximum and upper-percentile per-atom DF;
2. mean, p95, and maximum decoded postings per query;
3. touched-document fraction and exact BMP fallback rate;
4. native p50/p95 latency on held-out corpora;
5. retrieval metrics and row-level regressions at the same checkpoint.

Lower training loss, lower mean nnz, or better teacher fit cannot promote a
checkpoint when high-DF mass or native p95 traversal grows. DF-aware training
is authorized only after a query-time or static-pruning oracle identifies the
specific expensive atoms and demonstrates a quality/cost Pareto. A global
stopword-like deletion is not sufficient evidence because a high-frequency
atom may still be salient for a subset of queries.

Before any retraining, the product engine should test three model-free controls
in order: query-time DF contribution auditing, a hidden DF-threshold sweep with
quality gates, and improved exact block summaries or lexical-guided admission.
Only the first is exact by definition. Threshold pruning and guided traversal
must remain experimental unless their broader retrieval loss is explicitly
accepted; they must not silently replace the exact v3 default.

##### Model-free query-DF oracle

A hidden query-time control was added with a product-preserving default. It
drops a complete query atom only when its stable-root raw document frequency
exceeds a configured corpus ratio. The control is disabled whenever an active
or pending mutable projection exists, so it cannot change lexical-first CRUD or
snapshot behavior. At the default ratio of `1.0`, no atom is removed and the
exact product path is unchanged.

A 50-query FiQA canary established the quality/cost frontier. Ratios below
`0.35` reduced latency further but lost substantial Recall@100. Ratio `0.50`
was the only canary point that preserved Recall while improving all four
ranking metrics:

| Max DF ratio | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | p50 ms | p95 ms |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1.00 | 0.317326 | 0.275095 | 0.696024 | 0.350517 | 391.957 | 468.958 |
| 0.50 | 0.325717 | 0.283967 | 0.696024 | 0.365116 | 340.470 | 418.103 |
| 0.35 | 0.325541 | 0.283639 | 0.693524 | 0.359554 | 314.784 | 370.668 |
| 0.30 | 0.327799 | 0.282749 | 0.668524 | 0.360792 | 318.021 | 368.745 |
| 0.20 | 0.328669 | 0.289775 | 0.649635 | 0.353554 | 277.445 | 311.779 |
| 0.12 | 0.322111 | 0.280820 | 0.645357 | 0.377674 | 232.222 | 263.995 |

The complete official 648-query FiQA surface then compared exact `1.0` with
the sole surviving `0.50` candidate:

| Policy | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | p50 ms | p95 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Exact default | 0.334180 | 0.279181 | 0.618778 | 0.417735 | 375.532 | 454.391 |
| DF ratio 0.50 | 0.338530 | 0.282510 | 0.624560 | 0.424597 | 341.030 | 421.573 |

This is a real positive Pareto on FiQA: p50 fell 9.2%, p95 fell 7.2%, and all
reported retrieval metrics increased. One slow query removed four of 56 atoms
and reduced decoded postings from 691,470 to 526,010. It is not row-safe,
however: 8 of 648 queries lost Recall@100 while 16 improved, and the remaining
624 were unchanged. The official FiQA judgments were also used to select the
threshold, so this result is a representation oracle rather than an unbiased
promotion result.

Consequently `0.50` remains a hidden, checkpoint-specific research candidate.
It may be promoted only after a fixed-threshold shared15 or broader held-out
run shows macro improvement without unacceptable dataset-row regression. The
result does establish a concrete future-training requirement: the current
checkpoint places measurable retrieval noise in corpus-majority atoms, while
some high-DF atoms remain necessary for individual-query coverage. Future
DF-aware training must suppress that noise without implementing a universal
stopword deletion.

### 2. Local Training Surfaces Do Not Determine Global Retrieval Behavior

InfoNCE, candidate-set KL, pairwise losses, and reconstruction objectives are
computed over finite batches or candidate sets. The deployment constraints are
global:

- top-100 boundaries over the complete corpus;
- per-term document frequency;
- block-max traversal and exact BMP behavior;
- row-level safety across heterogeneous datasets.

Training loss can therefore improve without preserving the full-corpus ranking
or index cost. Longer training amplifies this mismatch rather than necessarily
solving it.

### 3. The Current Representation Carries Too Many Responsibilities

Dense retrieval uses continuous, distributed, signed global geometry. The
current learned-sparse representation is non-negative, vocabulary-bound, and
scored by one sparse dot product. The same coordinates must encode:

- lexical identity;
- semantic expansion;
- dense-like geometry;
- corpus frequency and posting cost;
- ranking calibration.

This is a stronger requirement than training a competitive learned-sparse
retriever. SPLADE demonstrates that vocabulary-sparse retrieval can be strong;
it does not establish that this representation is a lossless isometry of an
arbitrary dense model.

### 4. The Missing Result Is A Learnable Compiler, Not An Index Signal

Earlier dense-derived and oracle posting experiments showed that useful
posting structures exist. M1549 also showed that dense and lexical evidence are
complementary in native retrieval. The unresolved problem is producing the
required posting structure directly from text under the fixed representation
and cost constraints.

This distinction matters:

- posting/index capacity may exist in a richer latent or signed structure;
- the current vocabulary-sparse encoder may not be able to generate it safely;
- better post-hoc gates cannot repair an insufficient source representation.

## Research Reset

The reset began as a quantized semantic-posting capacity program rather than
another SPLADE loss variant. M1710 now closes the naive frozen
reconstruction-first balanced/residual-code instance. The remaining strict
branch must co-design retrieval-oriented geometry, scalar score decomposition,
and corpus load constraints; it must not simply add more frozen PQ codebooks.

```text
dense vector
    -> retrieval-oriented latent/signed compiler
    -> score-decomposable semantic postings
    + lexical postings in a separate namespace
    -> one physical native inverted index
```

This remains one index. Semantic code postings and lexical term postings occupy
different namespaces but are scored in one native retrieval operation. It is
not an ANN/BM25 candidate union or a post-hoc reranker.

### Stage 0: Close The Current Route

1. Complete the already-running step-4 FiQA native evaluation.
2. Treat the RankT5 check as one final teacher diagnostic only.
3. Preserve M1660 plus the unmodified OpenSearch sparse root as the learned
   sparse baseline.
4. Preserve M1700 step 4 as evidence of a real but narrow positive-curriculum
   signal; do not scale it to one million pairs.
5. Do not authorize another LR, temperature, lambda, pruning, or teacher grid.

### Stage 1: Representation Capacity And Co-Design Oracle

Do not train a text encoder. First preserve the completed negative controls:

1. current non-negative vocabulary postings: M1640-M1701 expose the stable
   quality/DF/traversal Pareto;
2. frozen balanced plus residual codebooks: M1710 fails the unbounded score
   ceiling and the bounded admission ceiling;
3. broad signed-coordinate accumulation: M1541/M1630 is expressive but not
   selective enough under the strict single-hop cost boundary.

The remaining strict experiment is not another deterministic reconstruction
compiler. It is a retrieval-oriented representation/index co-design over
frozen dense query and document vectors, with all of the following built into
the representation objective:

1. anisotropic or inner-product-preserving error rather than isotropic MSE;
2. query-document score and top-k isometry;
3. scalar-decomposable signed impacts;
4. balanced code usage and explicit per-code maximum-DF constraints;
5. native posting-byte and traversal accounting during selection.

An SNRM-style learned latent sparse basis is a valid control only if it uses the
same modern dense-isometry and native-cost gates. A CITADEL/PLAID-style
multi-vector or centroid-residual control may be informative, but must be
labeled as a relaxed-engine branch if it needs vector posting payloads,
candidate finalization, or late interaction.

For every family, report:

- dense overlap at 10, 50, 100, and 256;
- score and inner-product error;
- NDCG@10, MAP@100, Recall@100, and MRR@20;
- candidate upper bound;
- mean and p95 nonzeros;
- maximum DF and posting bytes;
- exact native parity and BMP latency.

No representation should proceed to encoder training unless it passes a fixed
quality/cost gate on native shared3 and then shared15.

### Stage 2: Frozen Unified Index Oracle

For the winning oracle representation:

1. assign semantic centroids or residual codes to a latent posting namespace;
2. retain lexical impacts in a lexical namespace;
3. add the two score contributions inside one native scorer;
4. implement safe score bounds for exact block pruning;
5. verify index lifecycle, update behavior, parity, bytes, and latency.

This stage must work without retrieval-label training. It establishes that the
index shape, not a teacher or selector, carries the required capability.

### Stage 3: Structured Output Head

Only after Stage 2 passes, train a head on a frozen dense root to predict:

- code assignments;
- residual weights;
- lexical impacts;
- balanced code/term load.

The first objective should reproduce the deterministic compiler:

- code-assignment loss;
- vector or residual reconstruction;
- pairwise inner-product/isometry loss;
- corpus load-balancing and explicit DF constraints.

Do not add qrels, BM25 rescue, hard-negative ranking, or dataset-specific
selection at this stage. First prove that the head reproduces the known-good
compiler.

### Stage 4: Retrieval Adaptation

Only a compiler-faithful checkpoint may receive contrastive or listwise
retrieval adaptation. Dense overlap, code agreement, maximum DF, and native
cost remain hard gates rather than soft terms that may be traded for training
loss.

### Stage 5: Encoder Compression

If the dense-root plus structured head succeeds, compress or distill the trunk
as a separate problem. Do not simultaneously learn semantic geometry,
quantization, sparsity, lexical expansion, and ranking.

## Stop Conditions

Stop the one-index dense-equivalence goal when the qrels-free representation
oracle cannot meet the agreed dense-overlap and retrieval floors within the
native cost budget. Text-encoder training cannot recover information that the
chosen index representation cannot express.

Stop a compiler-head route when:

- oracle representation passes but the head cannot reproduce code assignments
  or inner products on heldout corpora;
- gains require BEIR qrels, dataset-specific thresholds, or post-hoc unions;
- native parity, maximum DF, bytes, or latency fail despite offline quality;
- the next proposal only changes scalar weights or training duration.

Interpret failures by stage:

| Failure stage | Conclusion |
| --- | --- |
| Representation oracle | index shape is insufficient under the cost budget |
| Structured head | learnability, model capacity, or data coverage problem |
| Native replay | scoring or engine implementation problem |
| Retrieval adaptation | supervision damages a valid representation |

## Final Recommendation

The project should not be abandoned, but it should stop searching within the
current learned vocabulary-sparse objective family and must not repeat the
frozen balanced/residual compiler rejected by M1710. The next credible question
is not which loss can make SPLADE become dense. It is whether a
retrieval-oriented, load-balanced, score-decomposable representation can be
co-designed with the native index to approximate dense geometry at the required
cost, and only then whether a structured encoder head can reproduce it.

If that capacity proof fails, the scientifically correct product conclusion is
to retain hybrid dense and lexical retrieval. If it passes, it provides a
well-defined teacher artifact and decomposes the unified-encoder problem into
measurable, independently falsifiable stages.

## Local Evidence Index

The directional synthesis above is anchored in these local closure and
decision reports:

| Phase | Primary local evidence |
| --- | --- |
| Early SAE and posting formation | [M27 closure](sae-m27-exploration-closure-report.md), [M90 Stage-A closure](sae-m90-stage-a-closure-report.md), [M116-M122 cost/quality push](m0100-m0199/sae-m116-m122-sota-push-results-report.md) |
| Posting-native admission and dense reset | [M310-M323 handoff](m0300-m0399/ii42-m310-m323-posting-level-roadmap-handoff-report.md), [M370 project reset](m0300-m0399/ii42-m370-project-reset-review-report.md), [M396 MTEB frontier](m0300-m0399/ii42-m396-mteb-frontier-milestone.md) |
| P1/M549U and stage-two capability | [M547 dense-only gate](m0500-m0599/ii42-m547-stage1-dense-only-retrieval-report.md), [M549U compiler](m0500-m0599/ii42-m549u-unified-output-compiler-report.md), [M550 BM25-aware result](m0500-m0599/ii42-m550-bm25-aware-trained-fusion-report.md), [M601 auditable replay](m0600-m0699/ii42-m601-stage2-auditable-bm25-ranking-rebuild-report.md) |
| Boundary/compiler and generated-posting line | [M599 broader validation](m0500-m0599/ii42-m599-m551-broader-validation-report.md), [M653 evidence synthesis](m0600-m0699/ii42-m653-first-stage-evidence-synthesis-report.md), [M683 oracle boundary crossing](m0600-m0699/ii42-m683-support-safe-boundary-crossing-report.md), [M730 two-head stop](m0700-m0799/ii42-m730-two-head-dense-tail-report.md), [M733 synthesis](m0700-m0799/ii42-m733-evidence-synthesis-and-design-report.md) |
| Teacher/source observability | [M1217 LODO teacher](m1200-m1299/ii42-m1217-lodo-contrastive-teacher-replay-report.md), [M1225 CUB-specific replay](m1200-m1299/ii42-m1225-cub-specific-atom-native-report.md), [M1251 signed-sum replay](m1200-m1299/ii42-m1251-query-delta-transfer-ablation-report.md), [M1253 harm anatomy](m1200-m1299/ii42-m1253-signed-sum-harm-anatomy-report.md), [M1338 risk-penalty stop](m1300-m1399/ii42-m1338-atom-risk-tail-penalty-report.md) |
| Structural alternatives and URSI | [M1400-M1406 final](m1400-m1499/ii42-m1400-m1406-anchored-joint-sparse-interaction-final-report.md), [M1500-M1506 final](m1500-m1599/ii42-m1500-m1506-anchored-sparse-bilinear-residual-final-report.md), [M1510-M1518 final](m1500-m1599/ii42-m1510-m1518-retrieval-trained-sparse-final-report.md), [M1520C capacity](m1500-m1599/ii42-m1520c-ursi-codebook-capacity-report.md) |
| Protected-tail capability and source conversion | [M1540-M1545 review](m1500-m1599/ii42-m1540-m1545-recall-breakthrough-review.md), [M1546-M1549 native milestone](m1500-m1599/ii42-m1546-m1549-native-recall-breakthrough-report.md), [M1557 residual final](m1500-m1599/ii42-m1557-dense-root-unified-residual-final-report.md), [M1560-M1572 source final](m1500-m1599/ii42-m1560-m1572-unified-source-final-report.md) |
| Balanced discrete and learned-sparse engine line | [M1600 final](m1600-m1699/ii42-m1600-m1600a-balanced-discrete-final-report.md), [M1610 final](m1600-m1699/ii42-m1610-retrieval-aligned-source-final-report.md), [M1630 exact engine](m1600-m1699/ii42-m1630-exact-signed-block-engine-report.md), [M1640 sparse control](m1600-m1699/ii42-m1640-learned-sparse-engine-control-report.md), [M1650-M1651 transfer](m1600-m1699/ii42-m1650-m1651-sparse-foundation-transfer-report.md), [M1660 official BMP](m1600-m1699/ii42-m1660-official-bmp-engine-report.md) |
| Paper-native training and latest capacity gate | [M1670 score-spectrum](m1600-m1699/ii42-m1670-score-spectrum-sparse-distillation-report.md), [M1680-M1682 final](m1600-m1699/ii42-m1680-m1682-vocabulary-transfer-final-report.md), [M1691 paper schedule](m1600-m1699/ii42-m1691-paper-schedule-sparse-kd-report.md), [M1700 positive curriculum](m1700-m1799/ii42-m1700-literature-native-positive-curriculum-report.md), [M1710 additive-code stop](m1700-m1799/ii42-m1710-score-decomposable-semantic-posting-report.md) |

## References

- SPLADE-v3: <https://arxiv.org/abs/2403.06789>
- SNRM: <https://doi.org/10.1145/3269206.3271800>
- SPARTA: <https://arxiv.org/abs/2009.13013>
- COIL: <https://arxiv.org/abs/2104.07186>
- CITADEL: <https://arxiv.org/abs/2211.10411>
- ColBERTv2: <https://arxiv.org/abs/2112.01488>
- PLAID: <https://arxiv.org/abs/2205.09707>
- RepCONC: <https://arxiv.org/abs/2110.05789>
- Distill-VQ: <https://arxiv.org/abs/2204.00185>
- Anisotropic Vector Quantization: <https://arxiv.org/abs/1908.10396>
- Searching Dense Representations with Inverted Indexes:
  <https://arxiv.org/abs/2312.01556>
- Seismic: <https://arxiv.org/abs/2404.18812>
- BMP: <https://arxiv.org/abs/2405.01117>
- DF-FLOPS: <https://arxiv.org/abs/2505.15070>
- Minimizing FLOPs to Learn Efficient Sparse Representations:
  <https://arxiv.org/abs/2004.05665>
