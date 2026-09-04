# II-42 M370 Project Reset Review

## Purpose

This report resets the II-42 sparse retrieval project from first principles.
It reviews the experiment reports, roadmaps, and productization notes in this
checkout, then decides which route is most likely to produce a real
breakthrough.

The central question is:

- What did we actually prove?
- Which lines failed, and why?
- What did we do wrong strategically?
- If we restarted today, what should the next mainline be?

## Executive Verdict

The most likely breakthrough direction is not another downstream
admission/ranking/posthoc scorer tweak.

The next mainline should be:

1. Use dense-coordinate postings as the quality baseline.
2. Attack the real remaining blocker: fanout and efficiency.
3. Only then return to learned sparse atoms if they beat the coordinate
   quality-fanout frontier.

M366 and M367 changed the project state. They showed that sparse postings can
preserve dense quality if the postings are signed dense coordinates. On the
BEIR15 local shared artifact face, `coordinate_k512` is essentially dense:

| Row | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Mean touched ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_exact | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 1.0000 | 1.0000 |
| coordinate_k384 | 0.8523 | 0.8685 | 0.7719 | 0.6921 | 0.8820 | 1.0000 |
| coordinate_k512 | 0.8518 | 0.8719 | 0.7745 | 0.6944 | 0.9346 | 1.0000 |

That quality result is stronger than every learned SAE/posting/scorer line so
far. The failure is cost:

| Row | NDCG@10 | MAP@100 | Mean touched ratio |
| --- | ---: | ---: | ---: |
| coordinate_k8 | 0.2751 | 0.1980 | 0.1325 |
| coordinate_k16 | 0.4411 | 0.3477 | 0.3918 |
| coordinate_k32 | 0.5956 | 0.4968 | 0.8289 |
| coordinate_k64 | 0.6967 | 0.6046 | 0.9987 |
| coordinate_k96 | 0.7300 | 0.6449 | 1.0000 |

So the project should restart around a dense-only quality-fanout frontier:

- rotated / orthogonal dense coordinates;
- impact pruning and block-max pruning over coordinate postings;
- sparse dense autoencoders only if they beat coordinate postings at the same
  active budget and lower fanout;
- no BM25, qrel objective, admission scorer, or posthoc anchor until dense
  neighborhood preservation and fanout are solved.

## Current State, Compressed

### Product Engineering

The product/index engineering track is usable and should continue as an
independent model-swappable layer.

The mutable hybrid index, BM25/SAE posting lifecycle, runner hardening, cache
validation, and VectorChord / pgvector baselines are valuable infrastructure.
They do not, by themselves, prove the research claim that the learned sparse
representation can replace or exceed dense retrieval.

### Research Claim

The original implicit claim was:

> Learn sparse atoms/postings that preserve semantic dense behavior, then use a
> posting engine to retrieve with dense-like quality and better product
> properties.

That claim is not proven for the learned SAE/posting route.

The strongest learned/scorer rows recover qrel ranking quality downstream, but
the atom representation itself was not dense-neighborhood faithful:

- M360 showed atom-only rows far below dense.
- M361 measured current atom-vs-dense overlap directly and found very low
  overlap on representative datasets.
- M362 showed reconstruction-style dense objectives can improve overlap, but
  older posting-native / BM25-aware objectives had already drifted away from
  dense faithfulness.
- M366/M367 showed that dense-coordinate postings preserve dense quality much
  better than the learned atoms, but with unacceptable fanout.

### Main Research Blocker

The blocker is not candidate existence alone.

Several broad8 candidate upper bounds are high, but trained rankers fail to
promote the right positives high enough. At the same time, atom-only
representation quality is weak before the scorer is added.

The actual blocker is the joint condition:

1. preserve dense neighborhoods in a sparse posting representation;
2. keep first-stage fanout low enough to be product-relevant;
3. only after that, use supervised qrel objectives to try to beat dense.

We repeatedly optimized condition 3 before conditions 1 and 2 were solved.

## What Failed

### Early SAE/BM25 Fusion And Stage Gates

M130/M150/M160 established that SAE atoms can be made to work as BM25-like
terms and can sometimes improve recall or continuity metrics. The cost was
high, and ranking quality did not establish a dense-replacement claim.

Important lessons:

- Atom BM25 is not obviously wrong as an engine primitive.
- But high-dimensional sparse atoms alone were not enough to match dense top
  ranks.
- Segment/query granularity and pooling choices mattered.
- The early gates were useful for engineering continuity, not final research
  proof.

### M160A / Oracle Candidate Pool

M160A showed the right failure shape early:

- oracle candidate pools had headroom;
- shallow runtime-safe rank/admission heads could not select the right
  candidates;
- generic scalar fusion was not sufficient.

This was the first strong warning that "candidate exists somewhere" is not the
same as "posting route has learned the right retrieval geometry."

### M200 / M201 Stage-A Losses

M200 coverage loss helped some admission/hit metrics but not ranking. M201
pairwise rank loss regressed quickly.

Lesson:

- Stage-A representation training cannot be driven directly by a shallow qrel
  ranking objective before the representation preserves dense neighborhoods.
- The loss was answering the wrong layer of the problem too early.

### M300-M306 Atom Reliability And Safety Gates

M300/M302 found small real atom-level signals. M303 per-id reliability was
weaker and mostly learned family-scale correction, not atom-local semantics.
M304 deep fusion had the first better combined top-rank/tail smoke. M305 soft
safety gate was negative. M306 explicit admission policy was useful as a
diagnostic but not a breakthrough.

Lesson:

- Atom features carry some retrieval signal.
- Existing features were not enough to decide safe overrides at product
  quality.
- Small safe deltas were not evidence that the representation was dense-like.

### M310 Latent-Term BM25

M310 validated latent-term BM25 as a candidate-generation primitive. It did not
close the dense gap.

The dense gap remained material:

| Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| dense | 0.7092 | 0.6670 | 0.5874 | 0.4318 |
| best M310 | 0.6831 | 0.6276 | 0.5477 | 0.3973 |
| gap | -0.0261 | -0.0394 | -0.0397 | -0.0345 |

Lesson:

- Latent BM25 can be useful infrastructure.
- It is not the main path to a dense-quality sparse representation.

### M320-M325 Posting-Native Training

M320 showed direct posting-native training was trainable and directionally
interesting. It also showed that high-quality atoms leaned heavily on high-DF
heads, making pruning and fanout hard.

M322 validated a learned final scorer, but M323/M324/M325 variants did not
make the learned postings beat exact dense. Top-k/source-balance changes did
not solve the gap.

Lesson:

- "SAE to posting" and "posting-native direct training" both ran into the same
  unresolved issue: learned sparse units were not a clean dense-neighborhood
  representation.
- Direct training to posting targets did not magically solve the encoder
  problem.

### M326-M335 Dense Teacher And Interaction Scorers

M326B was a good candidate-pool scorer canary. M327 product dense teacher and
BM25-positive rescue did not materially solve top-rank quality. M328/M329
boundary-aware scoring was better than another teacher swap. M330 fixed BM25
preservation helped some datasets and hurt others. M331 adaptive preservation
gate learned near zero and closed that local line.

M332 natural candidate union increased upper-bound recall, but the scorer
could not convert the headroom into enough NDCG/MAP. M333 explicit win/loss
was flat. M334 interaction features were a real scorer-side improvement. M335
broad8 confirmed that the scorer route could beat BM25/unified rows, but still
left a large candidate-upper-bound gap.

Representative M335 qidfix broad8 evidence:

| Row | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5104 | 0.2804 | 0.2748 | 0.2374 |
| unified | 0.5485 | 0.2981 | 0.2871 | 0.2490 |
| scorer heldout | 0.8000 | 0.4370 | 0.4238 | 0.3700 |
| candidate upper bound | 0.8754 | 1.0000 | 0.9137 | 0.8754 |

Lesson:

- Interaction features and final scorers help.
- They are downstream repair mechanisms.
- They cannot answer whether the sparse encoder itself is the right
  representation.

### M336-M348 Broad8 Ranking And Admission

M336 fixed a qid collision issue; trec-covid collapse was evaluator artifact,
not candidate failure. M337 showed top160 was too aggressive and
`cqadupstack` had cache recall ceiling issues. M338 learned admission head was
weaker than simple anchor. M339 K1000 diluted the scorer. M340 rank-reference
helped top-rank but hurt recall. M341/M342 core/quota variants recovered some
recall but lost top-rank quality. M343 explicit admission was enough to justify
broad8. M344 broad8 completed but still had a huge upper-bound gap. M345
schedule/weight sweeps were small. M346 rank-discount was the best broad8
scorer row in that family. M347 LambdaRank did not beat M346A. M348 diagnosed
the remaining gap as ranking quality, not raw candidate availability.

Lesson:

- The scorer/admission line was professionally useful, but it became a local
  optimization basin.
- The upper-bound gap remained too large for another shallow objective tweak to
  be a plausible breakthrough.

### M349-M353 Posthoc Anchor Policies

M349/M350 found a small clean anchor signal. M351 heuristic query-shape policy
failed. M352 learned anchor policy was the first positive learned gate but
reduced recall. M353 recall-constrained thresholding made the learned gate
safe at the macro level.

M353 `thr0.70` versus fixed `alpha=0.03`:

| Split | dR@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| all-query | +0.000229 | +0.000222 | +0.000490 | +0.000376 |
| heldout | +0.000954 | -0.000147 | +0.000811 | +0.000397 |

But per-dataset risk remained:

- `trec-covid`: NDCG -0.017331, MAP -0.005418;
- `fiqa`: Recall -0.004129;
- `scidocs`: Recall -0.003052.

Lesson:

- The posthoc anchor line is real but too small.
- It is not a research breakthrough path.
- M353 should be parked as a downstream safety/risk diagnostic, not continued
  as the mainline.

### M360-M365 Dense-Faithfulness Reset

M360 asked the right question: do current atom representations preserve dense
retrieval before BM25/scorer rescue?

Answer: no.

M361 measured direct atom-vs-dense overlap:

- `nfcorpus` O@10 around 0.0984;
- `scifact` O@10 around 0.1375;
- dense-kNN canaries remained below about 0.50 O@10.

M362 showed a better route: hard TopK reconstruction over dense embeddings.

| Active k | Dense overlap O@10 |
| ---: | ---: |
| 16 | 0.5174 |
| 32 | 0.5696 |
| 64 | 0.6609 |
| 128 | 0.6913 |

M363 then showed signed dense identity atoms can trivially preserve dense
behavior if sparsity is allowed to be coordinate-like. M364 learned dense
autoencoder variants improved but did not clear the gate. M365 soft/ST/projection
variants were negative.

Lesson:

- Reconstruction was not inherently wrong.
- The project drifted when we optimized posting/ranking/admission behavior
  before enforcing dense-neighborhood preservation.
- Learned sparse atoms are not the baseline anymore; dense-coordinate postings
  are.

### M366-M367 Coordinate Postings

M366/M367 are the most important reset evidence.

They prove that posting-style sparse retrieval can preserve dense quality if
the sparse representation is allowed to be signed dense coordinates. They also
prove the product blocker: fanout is too high at the active-k values needed for
dense-level quality.

Lesson:

- The form "posting over sparse dimensions" is not the blocker.
- The blocker is finding a sparse basis/code that preserves dense geometry
  while producing selective postings.
- This is now a quality-fanout frontier problem.

## What We Did Wrong

### 1. We optimized downstream rescue for too long

After the scorer route started producing useful gains, we kept refining
admission heads, rank losses, preservation gates, K sweeps, and anchor policies.

Those experiments were not useless. They built strong diagnostics and product
scaffolding. But they increasingly optimized around a missing representation
property instead of fixing it.

The mistake was treating scorer improvement as evidence that the encoder route
was correct.

### 2. We mixed proof surfaces

Several distinct proof surfaces were repeatedly blended:

- dense-neighborhood preservation;
- qrel ranking quality;
- candidate upper bound;
- product ANN dense baseline;
- exact dense oracle;
- BM25+dense hybrid;
- sparse runtime fanout;
- posthoc policy safety.

This made small wins look more important than they were.

Correct rule going forward:

- representation experiments report dense O@10/O@100 and fanout first;
- ranking experiments report qrel metrics only after representation gates pass;
- product baselines are separated from exact dense references.

### 3. We asked dense teacher objectives to solve the wrong layer

Dense teacher scoring is useful, but it was often applied to final admission or
ranking before we had a sparse representation that preserves dense top-k
neighborhoods.

That makes the teacher a downstream rescuer, not a representation guarantee.

### 4. We did not compare learned atoms against the simplest dense-coordinate
baseline early enough

M366/M367 should have existed much earlier.

The dense-coordinate baseline exposed the project clearly:

- dense-quality sparse postings are possible;
- learned atom quality is currently behind;
- efficiency, not abstract representation form, is the next hard problem.

### 5. We let infrastructure friction shape research decisions

Distributed cache preparation, partial caches, stale roots, qid collision bugs,
and long runner loops consumed too much attention.

These were real issues and needed to be fixed, but they should not have driven
research direction. A new line should start with small local or single-machine
canaries that cannot be blocked by multi-day broad8 cache orchestration.

### 6. We continued local basins after stop evidence

M331, M333, M338, M347, and M353 all had enough evidence to stop or park their
families. In several cases, the next experiment answered only "can we get a
slightly safer version of the same small effect?"

That is not how we should spend breakthrough budget.

## Which Encoder Direction Is More Correct?

The answer is neither "SAE to posting" nor "directly train target postings" as
we practiced them.

The more correct direction is:

> dense-only sparse projection first, exact dense-overlap gated, then efficient
> posting execution, then qrel supervision.

Older SAE-to-posting failed because the learned atoms did not preserve dense
neighborhoods. Direct target/posting/scorer training improved downstream qrel
ranking but did not establish a better representation. It learned policies
around the missing representation rather than solving it.

So the right encoder program is:

1. Start from dense embeddings.
2. Learn or choose a sparse basis/code.
3. Require exact dense top-k overlap before any BM25/qrel objective.
4. Measure posting fanout at the same time.
5. Only then train text-to-sparse encoders and supervised rank improvements.

## Routes To Stop Or Park

The following should not be the mainline:

| Route | Status | Reason |
| --- | --- | --- |
| M353 posthoc learned anchor | Park | Real but tiny; per-dataset risk remains. |
| M349-M352 anchor sweeps | Park | Useful diagnostic only. |
| M347 LambdaRank | Stop | Did not beat M346A. |
| M345 schedule/weight sweeps | Stop | Small, tuning-only. |
| M339-M342 K1000/core/quota variants | Stop | K dilution and fixed quotas did not solve ranking gap. |
| M338 learned admission head | Stop | Weaker than simple anchor. |
| M333 explicit win/loss scorer | Stop | Flat. |
| M331 adaptive preservation gate | Stop | Learned near-zero; negative. |
| M330 fixed preservation | Park | Dataset-uneven; use only as diagnostic. |
| M305 soft safety gate | Stop | Feature set not enough. |
| M303 per-id reliability | Stop | Mostly family-scale correction. |
| M200/M201 direct Stage-A qrel losses | Stop | Wrong layer too early. |
| M1060 reusable atom posting | Park | Reuse constraints alone do not teach useful retrieval geometry. |
| Old SAE BM25 fusion as mainline | Park | Candidate primitive, not breakthrough. |

## Routes To Keep

| Route | Status | Role |
| --- | --- | --- |
| Product hybrid index | Continue | Engineering/product infrastructure, model-swappable. |
| M335/M346 scorer family | Keep as diagnostic | Downstream ranking benchmark, not representation proof. |
| M310 latent-term BM25 | Keep as primitive | Useful candidate-generation primitive. |
| M362 hard TopK dense reconstruction | Secondary active | Learned sparse representation candidate. |
| M366/M367 dense-coordinate postings | Primary active | Current quality baseline and frontier anchor. |

## New Mainline: Dense-Only Quality-Fanout Frontier

### M368: Rotated Coordinate Posting Efficiency Frontier

This is the next most likely useful experiment.

Goal:

- keep M367-level dense quality as much as possible;
- reduce touched ratio and posting fanout below raw coordinate postings;
- stay dense-only: no BM25, no qrel labels, no admission scorer.

Inputs:

- existing dense embeddings and BEIR15 shared artifact face;
- exact dense baseline from M367;
- coordinate posting evaluator from M366/M367.

Variants:

1. Raw coordinate baseline replay.
2. Random orthogonal rotations.
3. Fast Hadamard or sign-flip rotations if dimensionality supports it.
4. PCA / whitening-like rotations.
5. Block rotations.
6. Optional OPQ-like block quantization later, only if simple rotations help.

Budgets:

- active dims: 16, 32, 64, 96, 128, 192, 256, 384, 512;
- impact pruning thresholds;
- block-max candidate pruning levels.

Metrics:

- R@100, MRR@20, NDCG@10, MAP@100;
- dense O@10 and O@100;
- touched document ratio;
- posting document-frequency distribution;
- mean candidate count;
- p50/p95 query latency if executable in the current local evaluator.

Promotion target:

- a k64/k96 route that materially lowers touched ratio versus raw coordinate
  postings while preserving most k384/k512 dense quality;
- if no rotation can reduce fanout, prove that clearly and move to learned
  dense sparse codes.

### M369: Learned Dense Sparse Code Against Coordinate Baseline

Only after M368 defines the frontier.

Goal:

- learn sparse codes from dense embeddings;
- compare directly against coordinate postings at equal active budget;
- do not use BM25 or qrel labels.

Acceptable training objectives:

- hard TopK reconstruction;
- dense top-k overlap loss;
- local pairwise dense-neighbor preservation;
- multi-seed stability checks.

Gate:

- must beat coordinate quality-fanout, not old M320 atom rows;
- should aim for O@10 >= 0.85 and O@100 >= 0.80 at a meaningfully lower
  touched ratio than raw coordinate k384/k512;
- should not require downstream scorer rescue to look good.

### M370+: Text-To-Sparse Distillation

Only after a sparse code/basis is proven.

Goal:

- train query/doc encoders to emit the selected sparse representation directly;
- preserve the code-level dense-overlap gate;
- then introduce qrel supervision.

This is where the project can eventually try to exceed dense:

1. match exact dense with sparse retrieval;
2. add qrel/task supervision on top;
3. compare against exact dense and product ANN dense separately.

## Updated Evaluation Contract

Every future experiment must declare one primary proof surface:

| Surface | Required primary metrics | Forbidden shortcut |
| --- | --- | --- |
| Representation | Dense O@10/O@100, fanout | Reporting only qrel NDCG/MAP. |
| Candidate generation | R@100, candidate upper bound, fanout | Calling high upper bound a ranking success. |
| Final ranking | NDCG@10, MAP@100, MRR@20 | Ignoring candidate upper bound gap. |
| Product runtime | latency, touched ratio, memory | Calling exact dense quality product-ready. |
| Policy safety | macro and per-dataset deltas | Hiding dataset-specific regressions. |

Required baselines:

- exact dense;
- product ANN dense, if product relevance is claimed;
- raw coordinate postings;
- best current learned sparse code;
- BM25 only, only when lexical/hybrid claims are made;
- candidate upper bound for ranking experiments.

## Immediate Practical Plan

1. Freeze this reset report as the decision point.
2. Do not continue M353 as a mainline.
3. Start M368 locally first, not as a distributed broad8 job.
4. Reuse M367 evaluator and artifacts to add rotation/fanout variants.
5. Run small BEIR15 shared-face sweeps in minutes, not days.
6. If M368 finds a fanout-quality improvement, then expand to a stricter full
   corpus/product-index benchmark.
7. If M368 fails cleanly, launch M369 learned dense sparse code against the
   coordinate frontier.
8. Only after M368/M369 passes should we return to text encoders,
   admission/ranking objectives, or productization claims.

## Bottom Line

The project did not fail because sparse postings are impossible.

It failed to break through because we spent too long optimizing scorer and
admission behavior around a representation that had not passed a dense
neighborhood preservation gate.

The strongest current evidence says:

- sparse posting form can preserve dense quality;
- raw dense-coordinate postings prove that quality but are too expensive;
- learned atoms must now beat dense-coordinate postings on quality and fanout;
- downstream scorers are useful only after that representation problem is
  solved.

The next serious experiment is M368: rotated/structured coordinate postings
with strict dense-overlap and fanout measurement.
