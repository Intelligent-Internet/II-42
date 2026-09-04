# M1900-M1920 Learned-Sparse Success And Failure Factors

Date: 2026-07-13

## Executive Conclusion

Our locally trained sparse models did not fail because learned sparse retrieval
is unworkable, and they did not fail only because the optimizer stopped too
early. The evidence separates three effects:

1. **Training depth was initially insufficient.** M1902 to M1903 produced a
   large, disjoint-heldout ranking improvement.
2. **Depth alone did not solve the route.** M1904 completed the FLOPS ramp and
   exposed a stable ranking-versus-global-activation conflict.
3. **The strongest causal difference was representation preconditioning.** In
   the paired M1904/M1905 experiment, a pretrained MLM vocabulary survived the
   same schedule while the newly learned SAE basis did not.

Successful learned-sparse systems do not learn semantic geometry, sparse
coordinates, ranking, hard-negative separation, and corpus cost from a small
local surface at the same time. They begin from a strongly pretrained language
or retrieval representation, train on millions of positive/candidate sets,
use strong and often heterogeneous teachers, and treat sparsification as a
late or staged constraint. Our early routes attempted too many of these tasks
simultaneously with 10,000 rows and sampled cost proxies.

The closest local evidence to success is not the M1904 from-scratch
SAE-SPLADE checkpoint. It is:

- M1905, which proves that the mature vocabulary basis is trainable under the
  same bounded schedule;
- M1911, which proves that a paper-shaped, corpus-trained latent vocabulary can
  approach OpenSearch quality in one exact inverted index, but at high posting
  cost;
- frozen OpenSearch sparse-v2, which remains the best balanced product parent;
- M1914, which remains the compact learned-sparse control.

## 1. What "From Scratch" Actually Meant

None of the serious routes initialized every parameter randomly. Both our
experiments and successful papers use pretrained Transformer backbones. The
important distinction is what had to be newly learned.

| Route | Pretrained component | Newly learned component | Practical burden |
| --- | --- | --- | --- |
| M1902-M1904 SAE-SPLADE | DistilBERT token states | 65,536-latent SAE basis plus retrieval geometry | Learn a new interaction vocabulary and ranking under sparsity |
| M1905 standard SPLADE | DistilBERT plus pretrained MLM vocabulary projection | Retrieval weights and sparse ranking | Reuse token interaction structure learned during MLM |
| M1911 Latent Terms | Nomic retrieval-trained backbone | 32,768-latent reconstruction SAE | Learn latent basis, but reuse an already retrieval-shaped state space |
| OpenSearch sparse-v2 | CoCondenser/BERT retrieval-oriented root | Sparse document weighting and expansion | Large heterogeneous distillation program |
| Granite 30M Sparse | Retrieval-oriented six-layer WatBERT root | MLM sparse head and calibrated sparse ranking | Dense-to-sparse contrastive distillation with fixed support |
| Published SAE-SPLADE | DistilBERT | SAE basis, then full encoder retrieval adaptation | Two large, separately optimized stages |

Calling M1902-M1904 simply "from scratch" hides the real difficulty. The
Transformer could already represent language, but the query/document shared
coordinate system was new. A latent only becomes a useful posting key when it
is activated consistently by matching queries and documents, remains selective
over the full corpus, and receives a calibrated impact. Reconstruction alone
does not provide those properties.

## Earlier Project-Specific Attempts

The same pattern existed before M1900, but those experiments used different
proof surfaces and must not be merged into the M1904 matrix.

- M170A trained a broad Stage-A representation from scratch and reached
  candidate-surface Hit@20 `0.8736` versus dense `0.6817`. It never produced a
  comparable full-corpus Recall@100 gate. The result proved a learnable local
  representation, not an indexable retriever.
- M180A repaired M170A's source-mixture and streaming defects. It established
  correct weighted data exposure, but the recorded artifact is an execution
  contract rather than a mature full-corpus promotion result.
- M1513 trained a seeded random grouped sparse head on 100,000 MS MARCO
  triplets. Heldout pair accuracy improved from `0.3576` to `0.5114`, yet full
  NFCorpus NDCG@10 reached only `0.07638` and SciFact only `0.06018`. Its K=4
  touch ratios were `0.85384` and `0.99903`; the learned support was effectively
  a corpus scan on SciFact.
- M409-M417 small-student work repeatedly showed that a compact generic
  encoder could not rediscover PPLX dense geometry and posting geometry at the
  same time. M500-M504 consequently changed the problem to preserving a proven
  dense surface before attempting posting specialization.

These experiments distinguish "training changed the model" from "the model
became a useful retriever." Their common limitation was not zero gradient or
total collapse. Local teacher/candidate gains did not survive full-corpus
admission, ranking, and posting-cost gates.

## 2. Local Depth Curve: What Training More Actually Fixed

M1902 used 100 reconstruction and 100 retrieval steps. M1903 increased the SAE
stage to 4,000 steps and retrieval training to 2,000 steps on 10,000 MS MARCO
rows without changing the architecture, teacher, losses, or FLOPS schedule.

| Surface | Pairwise | Positive top1 | KL | Doc nnz | Doc maxDF |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1902 SAE | 0.707520 | 0.285156 | 1.368354 | 308.4 | 0.999132 |
| M1903 SAE | **0.809814** | **0.484375** | **0.994030** | **122.7** | 1.000000 |
| OpenSearch control | 0.924072 | 0.675781 | 1.250686 | 177.4 | 0.063802 |

This is a genuine positive result. More exposure improved pairwise ordering by
0.102295, positive top1 by 0.199219, and KL by 0.374325 on unseen rows while
reducing average document nnz. The original 100-step negative result was
undertrained.

But the same table identifies the remaining failure. M1903 had fewer average
postings than OpenSearch while at least one latent still activated on every
sampled document. Its document FLOPS remained 49.45x the control. Average nnz
therefore concealed a highly concentrated posting distribution.

**Lesson:** training depth repaired local ranking, but not corpus selectivity.
This is why both ranking and maxDF/native traversal must be measured.

## 3. Full Ramp: Why The SAE Branch Was Stopped

M1904 continued the same SAE checkpoint to 10,000 retrieval steps and crossed
the complete 6,000-step FLOPS ramp.

| Step | Pairwise | Positive top1 | KL | Doc FLOPS | Doc maxDF |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 2,000 | 0.830078 | **0.578125** | 0.949281 | 10.0955 | 0.986979 |
| 4,000 | **0.849609** | 0.531250 | **0.931307** | 3.8157 | 0.943576 |
| 6,000 | 0.802734 | 0.414062 | 1.112573 | 1.3528 | 0.357639 |
| 10,000 | 0.796875 | 0.429688 | 1.152607 | **0.8099** | **0.229167** |

The ramp did reduce sampled maxDF and FLOPS. It did so by removing useful
positive ordering. After full regularization, positive top1 dropped by more
than 0.16 from the step-2,000 frontier and never recovered.

This rejects two explanations:

- maxDF was not high merely because the regularizer had not reached full
  strength;
- continuing the same schedule longer was not converging toward a joint
  quality/cost solution.

It does not globally reject SAE-SPLADE. The local run was much smaller than the
published route and measured maxDF on candidate documents, not complete MS
MARCO. It specifically rejects M1904 as the parent for further local loss or
schedule tuning.

## 4. The Causal M1904/M1905 Comparison

M1905 changed one structural variable: it replaced the width-65,536 SAE output
basis with DistilBERT's pretrained MLM vocabulary head. It retained the same
10,000 rows, teacher, seed, batch schedule, optimizer, ranking loss, FLOPS
weights, and ramp.

| Disjoint surface | Pairwise | Positive top1 | KL | Doc nnz | Doc FLOPS | Doc maxDF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1904 SAE | 0.798584 | 0.460938 | 1.017299 | 101.36 | 10.0720 | 0.987630 |
| M1905 standard SPLADE | **0.869629** | **0.562500** | **0.585913** | **37.38** | **0.3250** | **0.153646** |
| OpenSearch control | 0.924072 | 0.675781 | 1.250686 | 177.41 | 0.6178 | 0.063802 |

The standard vocabulary was the only post-ramp survivor. Relative to the SAE
branch, it improved ranking while reducing nnz, FLOPS, and maxDF. Because data,
teacher, objective, and schedule were held constant, this is the strongest
local evidence that the output basis and its pretraining mattered more than
another loss variation.

Why the MLM basis helps:

- query and document token states are already trained to map into the same
  vocabulary coordinates;
- vocabulary terms carry strong lexical priors and non-random frequency
  structure;
- masked-language-model pretraining has already shaped the output projection
  scale and contextual expansion behavior;
- the retrieval stage can calibrate useful existing coordinates instead of
  inventing a shared posting language from 10,000 rows.

M1905 was still only a mechanism control. It saw 80,000 query presentations,
had no full-corpus native evaluation, and remained below OpenSearch ranking.
It proves the direction of the missing precondition; it is not itself a mature
checkpoint.

## 5. The M1911 Counterexample: A New Basis Can Work At Scale

M1911 prevents an over-broad conclusion that only lexical vocabularies work.
It trained five TopK-16 SAEs over 9.6M Nomic token states, using a
retrieval-trained Nomic backbone and qrels-free reconstruction. It then used
sum pooling, square-root impacts, corpus IDF, and latent BM25.

| Route | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Doc nnz | Native p95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1911 five-seed mean | 0.367006 | 0.309725 | 0.655292 | 0.448176 | 519.42 | - |
| M1911 selected exact BMP | 0.369506 | 0.312466 | 0.658295 | 0.450808 | about 519 | 27.495 ms |
| OpenSearch exact control | 0.370244 | 0.310860 | 0.656042 | 0.456006 | 177.41 | 17.499 ms |

M1911 nearly matched OpenSearch quality and passed exact single-index BMP
closure. It therefore demonstrates that a newly learned latent vocabulary can
be useful when the source backbone is retrieval-native and activation training
is broad enough.

Its failure is cost shape rather than ranking. Documents contained about 2.9x
as many postings as OpenSearch, maxDF remained 1.0, and removing the highest-DF
1% latents materially damaged every quality metric. The universal latent head
was carrying retrieval information, not removable dead capacity.

**Lesson:** scale and a retrieval-shaped backbone can make a latent basis
competitive, but reconstruction plus post-hoc pruning does not automatically
produce a selective product index.

## 6. What Successful Systems Do Differently

### 6.1 They do not ask a small local surface to invent relevance geometry

The OpenSearch inference-free sparse recipe retains 5,359,292 pretraining
queries after filtering and trains for 150,000 steps with 48 queries and eight
documents per query. It then fine-tunes on 502,939 MS MARCO queries for 50,000
steps with one positive and ten hard negatives. The local M1904 route used
10,000 rows and 10,000 retrieval steps.

Published SAE-SPLADE trains its SAE on all 8.8M MS MARCO passages for 160,000
steps with 768 documents per batch, then performs 240,000 retrieval steps with
32 queries and eight hard negatives per query. That is not a minor extension
of M1904; it is a different exposure regime.

### 6.2 They use informative positive and negative geometry

SPLADE-v3 mines many hard negatives, scores them with an ensemble of
cross-encoders, normalizes teacher score distributions, and combines KL with
MarginMSE. It also starts its strongest model from SPLADE++SelfDistil rather
than generic DistilBERT.

OpenSearch uses a heterogeneous dense+sparse teacher during broad pretraining,
self-mined hard samples, consistency filtering, and a stronger ensemble that
adds cross-encoders during MS MARCO fine-tuning.

Several local routes instead used eight-candidate stored surfaces, weak or
single teachers, and repeated examples from a 10,000-row pool. Loss values were
optimizable, but the candidate graph did not expose enough corpus-wide
confusions to define a robust sparse coordinate system.

### 6.3 They separate representation learning, retrieval, and sparsification

Successful recipes use a curriculum:

1. initialize or pretrain a language/retrieval representation;
2. establish relevance with positives and hard negatives;
3. distill richer score geometry;
4. progressively impose retrieval cost;
5. select using heldout quality and real retrieval measurements.

The published SAE-SPLADE paper explicitly trains the SAE first, discards its
decoder, and only then adapts the complete encoder under the retrieval
objective. It reports joint SAE reconstruction plus retrieval training as
detrimental because the objectives compete.

Our older experimental families often combined representation invention,
teacher fit, support selection, and cost pressure inside one short phase. A
failure could therefore be attributed neither to the basis nor to the ranking
objective until M1904/M1905 isolated them.

### 6.4 They use a representation-compatible sparsity mechanism

FLOPS is not universally sufficient. OpenSearch adds an IDF-aware objective so
low-information frequent terms are easier to remove while rare informative
terms receive stronger ranking gradients. Granite found ordinary FLOPS
insufficient for its six-layer starting point and added query/document total
NORM regularization plus fixed 50/192 active-dimension limits.

DF-FLOPS goes further by explicitly tracking document frequency because a low
average activation can coexist with a few extremely long posting lists.

Our early selection relied heavily on mean nnz and candidate-sample FLOPS. The
native results later showed that maxDF, total postings, posting-list skew, and
actual traversal must be hard gates rather than retrospective diagnostics.

### 6.5 They begin from a useful output geometry

The successful routes reuse one of these:

- a pretrained MLM vocabulary;
- a retrieval-trained dense root followed by sparse distillation;
- a separately pretrained SAE exposed to a very large corpus;
- an already successful sparse checkpoint followed by further distillation.

They do not generally initialize an arbitrary high-dimensional posting basis
and expect a small ranking set to discover a reusable language. SPLADE-v3's
strongest model improves when initialized from SPLADE++SelfDistil; Granite
first constructs a retrieval-oriented root; published SAE-SPLADE spends
160,000 steps preconditioning its latent basis before retrieval adaptation.

## 7. Success And Failure Factor Matrix

| Factor | Mature success pattern | Local failed pattern | Evidence |
| --- | --- | --- | --- |
| Backbone | Retrieval/MLM-preconditioned | Language backbone with new interaction basis | M1904 vs M1905; Granite |
| Output coordinates | Shared semantics already established | 65K SAE coordinates newly invented | M1905 dominates M1904 |
| Data breadth | Millions of queries/passages | 10K repeated rows | OpenSearch and SAE-SPLADE recipes |
| Candidate construction | Positives plus mined hard negatives | Small stored candidate sets | SPLADE-v3/OpenSearch |
| Teacher | Dense+sparse or cross-encoder ensemble | Single/local teacher surfaces | M1917 complementarity; M1918 transfer failure |
| Curriculum | Pretrain, relevance, hard-negative, sparsify | Several responsibilities in one short phase | Paper recipes vs local canaries |
| Cost objective | IDF/DF/NORM plus measured engine | Mean nnz and batch FLOPS emphasized early | M1903 maxDF; M1911 native cost |
| Selection | Heldout plus full corpus/native gate | Candidate loss before native closure | M1918 heldout-positive/native-negative |
| Scale decision | Scale only after a causal mechanism gate | Loss/threshold variants before representation proof | M1904/M1905 reset |
| Generalization | Broad source mixture and untouched benchmarks | Narrow or reused local surfaces | M1917 full native matrix |
| Proof surface | Full corpus plus native engine | Candidate Hit/pairwise used as early evidence | M170A and M1513 |

## 8. Which Failures Are Repairable

### Repairable by a faithful reproduction

- insufficient data diversity and training exposure;
- weak candidate mining and missing hard negatives;
- single-teacher supervision;
- incomplete curriculum and premature cost pressure;
- missing periodic corpus-DF/native validation;
- selecting on KL or sampled ranking instead of complete retrieval.

These factors were not fairly tested by M1904. A true paper-scale reproduction
could answer them.

### Not repairable by continuing the same checkpoint

- the M1904 SAE basis loses positive ordering when the fixed FLOPS ramp becomes
  strong;
- M1918's heldout improvement fails every primary native macro metric;
- M1920's fixed threshold repairs cost but not quality;
- post-hoc removal of M1911's universal latent head destroys useful retrieval
  signal.

More steps, another threshold, or another scalar loss weight on these same
checkpoints is not a justified experiment.

### Structurally unresolved

- whether a latent vocabulary can match mature vocabulary sparse quality at
  OpenSearch-like native cost;
- whether corpus-aware DF training can preserve M1911's useful universal-head
  semantics in more selective coordinates;
- whether PPLX can improve a mature sparse student without requiring the
  student to imitate PPLX-only geometry.

These require new training programs or representations, not local tuning.

## 9. Recommended Interpretation And Next Baseline

The correct conclusion is not "our from-scratch sparse training was useless."
It produced two durable findings:

1. the SAE retrieval signal improves substantially with depth;
2. under matched conditions, output preconditioning determines whether that
   signal survives sparsification.

The correct operational baseline is frozen OpenSearch sparse-v2. The next
training program should start from that mature parent and reproduce its public
large-data, heterogeneous-teacher curriculum in explicit rungs. M1914 remains
the compact control; P1 remains the Recall/dense-faithfulness control; PPLX is
one teacher, not the sparse parent.

If the project wants to answer the scientific from-scratch SAE question, it
must be a separate, faithful SAE-SPLADE reproduction using the 8.8M-passage
corpus, the 160K SAE stage, the 240K retrieval stage, and complete native cost
gates. It should not be represented as a continuation of M1904.

The first scale rung must demonstrate all of the following before expansion:

- heldout pairwise and positive-top1 improvement;
- stable results on an untouched source split;
- corpus maxDF/posting-skew improvement, not only mean nnz;
- no native Recall/NDCG/MRR regression;
- bounded index bytes and p95 traversal.

Without these conjunctive gates, more training can make the loss look mature
while moving the product farther from deployment.

## 10. Primary Evidence

Local reports:

- `docs/research-sae/reports/m0100-m0199/ii42-m130-m170-recall-comparison-report.md`
- `docs/research-sae/reports/m1500-m1599/ii42-m1513-ssr-grouped-canary-report.md`
- `docs/research-sae/reports/m0500-m0599/ii42-m500-m504-pplx-compression-milestone.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1900-m1902-paper-native-learned-sparse-reset-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1903-sae-splade-medium-depth-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1904-sae-splade-full-ramp-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1905-paired-standard-splade-full-ramp-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1911-nomic-latent-terms-reproduction-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1900-m1913-learned-sparse-reset-consolidated-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1917-m1920-parent-and-pplx-closure-report.md`

Primary literature:

- OpenSearch inference-free sparse retrieval:
  https://arxiv.org/html/2411.04403
- SPLADE-v3:
  https://arxiv.org/html/2403.06789
- SAE-SPLADE:
  https://arxiv.org/html/2604.21511
- Granite Embedding Models:
  https://arxiv.org/html/2502.20204
- DF-FLOPS:
  https://arxiv.org/html/2505.15070
