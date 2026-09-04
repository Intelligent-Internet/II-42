# II-42 M504-M505 PPLX Transfer Posting Plan

## Question

Can we stop training posting encoders from scratch and instead turn PPLX into a
compressed, posting-oriented encoder through quantization, transfer, and
possibly reinforcement/listwise optimization?

Short answer: yes, this is now the most plausible route, but the sequence
matters. The next model should not start with RL. It should start from a
dense-equivalent compressed PPLX surface and only add ranking/posting
optimization after supervised preservation is stable.

The compression target is not fixed to int4. Int4 is an extreme-limit probe.
The current proven mainline is row-wise int8; any lower-bit route must earn its
place by matching the same dense-preservation gates.

## Why This Is Plausible Now

M500-M503 established three useful facts:

1. PPLX final embeddings are the valuable surface; intermediate layers do not
   preserve retrieval geometry.
2. Row-wise int8 output/index compression is dense-equivalent across 10
   materialized tasks.
3. Row-wise int4 is not dense-equivalent by overlap, but qrels metrics remain
   close enough to be interesting as an approximate-tier or lower-bound probe.

This suggests the bottleneck is not "can dense be represented compactly"; it
can. The bottleneck is "can the encoder emit an indexable representation that
keeps this dense function while reducing index/runtime cost".

## Recommended Staging

### M504: Broad Hybrid Validation

Goal: confirm that row-int8 remains equivalent after BM25 fusion.

Evaluate:

- `exact_dense`;
- `row_int8`;
- `row_int4`;
- `bm25`;
- `exact_dense_bm25_zblend`;
- `row_int8_bm25_zblend`;
- `row_int4_bm25_zblend`;
- M396 structural route if the same task root and metric surface are directly
  comparable.

Promotion rule:

- `row_int8_bm25_zblend` must match `exact_dense_bm25_zblend` within noise;
- `row_int4_bm25_zblend` can be kept only if its recall loss is acceptable or
  if it is used with a rerank stage.

Status: completed.

Result:

- `row_int8_bm25_zblend` matches exact hybrid within noise: macro NDCG@10
  0.59052 versus 0.59061, Hybrid O@100 0.99687.
- `row_int4_bm25_zblend` is qrels-close but not overlap-faithful enough for the
  default path: macro NDCG@10 0.59073, Hybrid O@100 0.95471.
- row-int8 is promoted as the compressed dense component for both dense-only
  and hybrid retrieval.

### M505: Model-Side Compression Gate

Goal: compress the PPLX model itself without changing the embedding contract.
Start with the strongest safe compression, not the smallest possible model.

Evaluate:

- bf16 baseline;
- weight-only int8;
- weight-only int4;
- activation-aware/QAT variants if plain weight quantization fails;
- optional final-layer/head-only quantization.

Gate:

- dense vector cosine;
- dense Top100 overlap;
- M503 qrels metrics;
- row-int8 output equivalence after quantized-model inference.

Do not proceed to posting training if the compressed model cannot reproduce the
row-int8 dense surface.

Status: completed for the first preservation gate.

Result:

- raw `AutoModel` final hidden mean is close but not sufficient: FiQA sampled
  Top100 is about 0.96 against the materialized teacher;
- `fp16` raw model inference produces non-finite vectors and remains rejected;
- CPU dynamic int8 is unsupported on the current architecture;
- `bitsandbytes` int8/int4 is unavailable on the current spark-1 environment;
- official `SentenceTransformer` int8 output is the correct model-side surface:
  broad10 sampled Top100 0.99596, Score Pearson 0.99963, Doc Cos 0.99941.

Decision:

- use official ST/TEI int8 PPLX output as the preservation target;
- do not train against raw AutoModel hidden-state mean;
- postpone lower-bit model compression until the official int8 surface is
  preserved end-to-end.

### M506: PPLX-Initiated Posting Adapter

Goal: add an indexable posting surface without asking a small student to
rediscover semantics.

Architecture candidates:

- frozen PPLX + trainable posting projection/head;
- PPLX + LoRA adapters + posting head;
- quantized PPLX + small trainable adapter;
- final hidden state -> signed active coordinates + residual sketch;
- final dense vector -> deterministic row-int8 + structural posting compiler.

Loss stack:

- dense-vector preservation to exact PPLX;
- row-int8 consistency;
- score-matrix distillation on sampled query-doc groups;
- TopK overlap / pairwise agreement against dense teacher;
- active-support and sign/magnitude posting loss;
- residual-sketch loss;
- posting load/fanout balance;
- quantization-aware noise during training.

The first gate should be teacher preservation, not NDCG:

- query/doc dense cosine;
- score Pearson;
- dense Top100 overlap;
- posting active Jaccard;
- candidate recall under fixed fanout.

Status: first gate completed.

Result:

- `teacher_row_int8_dense` again matches materialized dense, so M505's official
  int8 surface remains the correct teacher;
- a single learned scorer is not viable yet: `m506_linear_adapter` and MLP
  variants score poorly even when they find useful candidates;
- the useful shape is a split head: learned linear candidate/support head plus
  structural dense-tail scorer;
- FiQA prefix 256 reaches NDCG@10 0.48516 versus row-int8 dense 0.51134, with
  Dense O@100 0.82656, Candidate R@100 0.90188, and Touch 0.37445;
- broad4 sampled prefix 256 reaches macro NDCG@10 0.45643 versus row-int8 dense
  0.49871, with Dense O@100 0.71888 and Candidate R@100 0.78130.

Decision:

- promote M506b support-head training;
- keep structural dense-tail scoring fixed until candidate coverage is stable;
- do not start RL/ranking-aware tuning yet.

### M506b: Candidate-Support Head

Goal: train only the support/candidate coordinates while keeping the structural
dense-tail scorer fixed.

Status: first gate completed.

Result:

- membership-only support training helps but is not enough: FiQA prefix 256
  reaches NDCG@10 0.48748-0.48926 versus row-int8 dense 0.54526;
- query-only support adaptation is worse than the structural compiler and is
  rejected for now;
- combined dense-score + TopK membership support loss is the current winner:
  FiQA prefix 256 reaches NDCG@10 0.51325, Dense O@100 0.78117, Candidate R@100
  0.83641, Touch 0.37831;
- FiQA prefix 384 reaches NDCG@10 0.52483, Dense O@100 0.82984, Candidate R@100
  0.91023, Touch 0.49673;
- broad4 prefix 256 reaches macro NDCG@10 0.46254 versus row-int8 dense
  0.50301, improving over the structural compiler at 0.42930.

Decision:

- promote `rotation_residual` support head;
- keep score+membership support loss;
- keep structural scoring fixed;
- route M506c toward query-adaptive fanout and false-negative recovery.

### M506c: Adaptive Fanout Probe

Goal: determine whether the Broad4 TRECCOVID failure is a semantic failure of
the PPLX-root support head or a fixed-fanout coverage failure.

Status: diagnostic gate completed.

Result:

- TRECCOVID fixed prefix 256 was the visible Broad4 blocker: NDCG@10 0.68205
  versus row-int8 dense 0.78947, Dense O@100 0.34080, Candidate R@100 0.34120,
  Touch 0.15779;
- TRECCOVID prefix 512 reaches NDCG@10 0.77018, Dense O@100 0.54800,
  Candidate R@100 0.56240, Touch 0.27949;
- TRECCOVID prefix 768 reaches NDCG@10 0.78104, Dense O@100 0.66200,
  Candidate R@100 0.69480, Touch 0.37707;
- an oracle-mixed Broad4 estimate, replacing only the low-coverage TRECCOVID
  row, would move macro NDCG@10 from 0.46254 to 0.48457 at prefix 512 or
  0.48729 at prefix 768.

Decision:

- the route is still alive;
- do not treat this as dataset tuning;
- promote M506d qrels-free query-adaptive fanout;
- keep M507 ranking-aware or RL tuning behind the dynamic-fanout gate.

### M506d: Qrels-Free Count-Adaptive Fanout

Goal: replace the diagnostic oracle with a real query-time policy that uses no
qrels, no dataset names, and no BM25.

Status: first Broad4 gate completed.

Policy:

- start at prefix 256;
- if the base-prefix touched ratio is below 0.30, use prefix 512;
- if the base-prefix touched ratio is below 0.20, use prefix 768.

Result on Broad4 sampled seed 5062:

- row-int8 dense teacher: macro NDCG@10 0.46462;
- fixed prefix 256: 0.42581, Dense O@100 0.73040, Candidate R@100 0.78740,
  Touch 0.54270;
- count-adaptive: 0.45246, Dense O@100 0.81150, Candidate R@100 0.87850,
  Touch 0.59705;
- fixed prefix 512: 0.45156, Touch 0.68362;
- fixed prefix 768: 0.45285, Touch 0.75818.

Decision:

- dynamic fanout is promoted as a real route improvement;
- the current hard threshold is not the final policy;
- M506e should calibrate a richer qrels-free fanout gate and run Broad10
  sampled.

### M506e: Coverage-Calibrated Fanout

Goal: replace M506d hand thresholds with thresholds calibrated from train-query
dense teacher Top100 coverage.

Status: completed and not promoted.

Result:

- Broad4 touch-budget calibrated-count reaches NDCG@10 0.45222, Dense O@100
  0.81880, Candidate R@100 0.89820, Touch 0.65941;
- Broad4 hand-count reaches 0.45246, Dense O@100 0.81150, Candidate R@100
  0.87850, Touch 0.59705;
- Broad10 sampled calibrated-count reaches 0.49718, Dense O@100 0.75912,
  Candidate R@100 0.84508, Touch 0.53848;
- Broad10 sampled hand-count reaches 0.49956, Dense O@100 0.76016,
  Candidate R@100 0.83376, Touch 0.47409;
- fixed prefix 768 remains the quality upper point at 0.50778, Touch 0.64421.

Decision:

- calibrated dense-coverage fanout is a useful diagnostic but not the promoted
  policy;
- M506d hand-count remains the best efficiency/quality dynamic policy;
- M506f should target dense ranking agreement or prefix-stability, not just
  dense Top100 candidate coverage.

### M506f: Rank-Agreement Fanout

Goal: calibrate query-time fanout from train-query agreement between the route
ranking and the dense-teacher ranking, instead of candidate coverage alone.

Status: completed and not promoted.

Result:

- Broad4 rank-agreement count reaches NDCG@10 0.45222, Dense O@100 0.81880,
  Candidate R@100 0.89820, Touch 0.65941;
- Broad4 hand-count remains slightly better at 0.45246 with lower Touch
  0.59705;
- Broad10 sampled rank-agreement count reaches 0.49711, Dense O@100 0.75948,
  Candidate R@100 0.84552, Touch 0.53849;
- Broad10 sampled hand-count reaches 0.49956, Dense O@100 0.76016,
  Candidate R@100 0.83376, Touch 0.47409;
- fixed prefix 768 remains the quality upper point at 0.50778, Touch 0.64421.

Decision:

- rank-agreement is a better diagnostic than pure candidate coverage, but the
  threshold-only gate is not strong enough;
- M506d hand-count remains the default dynamic policy;
- further threshold sweeps are low-value;
- M507 should move to learned admission/ranking under explicit dense-overlap
  and fanout constraints.

### M507: Ranking-Aware Fine-Tuning or RL

Goal: after teacher preservation is stable, optimize the posting/index behavior
for retrieval quality and fanout.

This stage can use:

- listwise distillation from dense or dense+BM25 teacher;
- LambdaRank-style surrogate;
- qrels-aware fine-tuning on train splits;
- policy-gradient/RL only for discrete fanout/admission decisions.

RL should be late-stage, not the base training method. If used too early, it
will optimize noisy ranking metrics while destroying dense equivalence. A safe
RL formulation should include hard penalties for:

- dense-overlap loss;
- fanout explosion;
- posting load imbalance;
- recall collapse on held-out dense teacher topK.

Status: first learned-admission gate completed.

Result:

- Broad10 sampled full-feature learned admission/rerank reaches NDCG@10
  0.55028, beating same-run hand-count at 0.54819 and fixed p768 at 0.54942;
- the full-feature gate is diagnostic, not a clean runtime win, because it
  needs p768 candidate generation and full route features over the generated
  pool;
- support-only learned admission reaches only 0.51230 on the same Broad10
  matrix, with Dense O@100 0.52388;
- support-only distillation from the full gate fails on Broad4, matching the
  weak direct support-only gate rather than recovering full-gate behavior.

Decision:

- stop threshold-only and linear support-only admission work;
- keep M506d hand-count as the default low-cost dynamic policy;
- keep M507 full-feature gate as a diagnostic upper point;
- any next step must change the model-side support/admission representation,
  not keep tuning posthoc gate features.

### M508: Direct Posting-Head Target Shaping

Goal: move the pressure back into the posting/support head.  Train the head
against dense-teacher positives and high-fanout route winners, instead of
trying to recover dense behavior with a posthoc admission gate.

Status: completed and promoted.

Result:

- Broad4 confirms the target shape: fixed p768 reaches NDCG@10 0.49194 versus
  row-int8 dense 0.49808; hand-count reaches 0.48927 at Touch 0.60210;
- Broad10 sampled confirms the route is not a narrow Broad4 artifact: fixed
  p768 reaches 0.57165 versus row-int8 dense 0.58652;
- Broad10 hand-count reaches 0.56711 at Touch 0.47449, improving over M507
  hand-count 0.54819 and M507 full-feature diagnostic 0.55028;
- fixed p512 reaches 0.56614, close to hand-count but with higher Touch
  0.55119;
- fixed p256 remains below the quality target at 0.52541.

Decision:

- promote M508 stage-A as the current best direction;
- stop linear support-only/posthoc admission loops;
- train the next model as a PPLX-root direct posting encoder;
- keep the first-stage objective no-BM25 and supervised dense-preserving;
- keep RL and BM25-aware optimization as second-stage additions only after
  supervised dense/posting preservation passes.

### M509: PPLX-Root Direct Posting Encoder Smoke

Goal: verify that raw text can be routed through PPLX into the M508 direct
posting target before investing in LoRA or RL.

Status: pipeline completed, model not promoted.

Result on FiQA smoke:

- frozen PPLX + heads reaches doc/query dense cosine around 0.95 against the
  row-int8 teacher;
- the support/posting shape is still weak: active Jaccard is about 0.10-0.14
  across smoke runs;
- a tiny last-layer transfer smoke reaches subset candidate recall 0.64250,
  but the larger same-shape last-layer/e2 gate falls to 0.56156;
- therefore full last-layer unfreezing is not a clean win yet.

Decision:

- keep M509 as proof that the PPLX-root pipeline and direct target wiring are
  valid;
- do not promote head-only or one-layer transfer as a retrieval model;
- move to proper LoRA/adapters as M510;
- continue to keep BM25 and RL out of the first-stage encoder loss.

### M510: PPLX LoRA Direct Posting

Goal: adapt the PPLX root with lightweight LoRA while keeping the M508 direct
posting target and no-BM25 first-stage loss.

Status: completed as a support-shape gate and promoted as the current best
raw-PPLX-root route.

Result:

- FiQA LoRA2/e2 reaches subset candidate recall 0.56266;
- FiQA LoRA2 support-heavy/e4 reaches 0.62000, with doc/query support cosine
  0.45249 / 0.41990;
- Broad4 LoRA2 support-heavy/e4 reaches subset candidate recall 0.64258,
  doc/query active Jaccard 0.17351 / 0.19364, and keeps dense cosine around
  0.93;
- Broad4 task rows show the route generalizes beyond FiQA: TRECCOVID reaches
  subset candidate recall 0.80969, FiQA 0.63219, ArguAna 0.58531, SCIDOCS
  0.54312.

Decision:

- promote LoRA/adapters over head-only and full last-layer unfreezing;
- do not promote M510 as a final retrieval model until route/qrels evaluation
  passes;
- keep first-stage training no-BM25 and no-RL;
- make M511 an actual posting-route evaluation of the LoRA support surface.

### M511: PPLX LoRA Route/Qrels Gate

Goal: test the M510 LoRA support output as the real posting candidate surface,
while keeping the M506 structural scorer fixed.

Status: completed as a FiQA route-subset gate; not yet promoted to full-corpus
retrieval.

Result:

- wide prefixes confirm safety: `m511_lora_support_candidates_p256` matches
  the structural route metrics on the FiQA route subset, with NDCG@10 0.46704,
  Recall@100 0.87786, candidate recall 0.99922, and Touch 0.98870;
- tight prefixes expose the next bottleneck: `m511_lora_support_candidates_p128`
  reaches NDCG@10 0.46717, Recall@100 0.86693, candidate recall 0.97891, and
  Touch 0.91362;
- p64 reduces Touch to 0.73024, but candidate recall falls to 0.86328, which is
  too low for promotion;
- p32 reaches Touch 0.50278, but candidate recall collapses to 0.65047.

Decision:

- keep the PPLX-LoRA route alive because quality can survive the real route
  evaluator;
- do not scale M511 to Broad4/full corpus as a final result yet, because useful
  quality still requires very high fanout;
- make M512 fanout-aware support training: preserve dense teacher candidates
  while explicitly penalizing broad coordinate sharing;
- keep BM25/RL out of this first-stage route until the p64/p96 candidate gate
  is credible.

### M512: Route-Fanout LoRA Fine-Tune

Goal: reduce the excessive posting candidate fanout seen in M511 while keeping
the first-stage training no-BM25/no-qrels.  M512 adds a dense-teacher candidate
objective: teacher top docs are positives, sampled corpus docs are negatives,
and the M508 direct support/dense target remains an anchor.

Status: completed as a FiQA route-subset gate; promising but not yet promoted.

Result:

- p128 keeps NDCG@10 0.46711 and Recall@100 0.87266 while reducing Touch to
  0.74261;
- compared with M511 p128, M512 cuts Touch from 0.91362 to 0.74261 while
  keeping NDCG flat and improving Recall@100;
- p96 keeps NDCG@10 0.46711 and Recall@100 0.87005 with Touch 0.68090;
- candidate recall is the remaining bottleneck: p128 reaches 0.95359, p96
  reaches 0.92484, and p64 reaches 0.85656.

Decision:

- keep this route alive and promote M512 as the current best fanout-shaping
  evidence;
- do not call it final because p96/p64 candidate recall is still below the
  promotion target;
- make M513 an objective-balance grid over positives, negatives, route epochs,
  negative penalty, and anchor weight;
- scale to Broad4 only after p96 or p128 keeps candidate recall near 0.95+
  with Touch below roughly 0.80.

### M513: Route Objective Balance Grid

Goal: test whether simple scalar objective balancing can recover the M512
candidate recall/fanout tradeoff.

Status: completed as a FiQA route-subset gate; negative result for the simple
grid, but it preserves the M512 baseline as current best.

Result:

- `baseline` p128 remains best: NDCG@10 0.46711, Recall@100 0.87266,
  candidate recall 0.95359, Touch 0.74261;
- `preserve16` lowers Touch to 0.69367 at p128, but candidate recall falls to
  0.92953 and NDCG@10 drops to 0.46248;
- `preserve16_neg48_e2` lowers Touch further to 0.66830 at p128, but candidate
  recall falls to 0.84172 and Recall@100 drops to 0.84922;
- p96/p64 remain below the promotion target under all tested presets.

Decision:

- keep M512/M513 `baseline` p128 as the current best;
- stop the simple scalar loss sweep here: more positives, more negatives, and
  more route epochs narrow fanout by removing useful candidates;
- make M514 a coordinate-load/load-balancing objective rather than another
  query-local sampled-negative sweep;
- if load-balancing cannot recover p96 candidate recall, switch to
  query-adaptive prefix/fanout rather than forcing p64.

## Position on Bit Depth

There are two different compression questions:

1. output/index compression;
2. model weight/activation compression.

M503 says output row-int8 is currently the dense-faithful default. Output
row-int4 is not dense-faithful enough to be the default, even though qrels
metrics are close. It is a candidate for approximate storage, a second-stage
rerank pipeline, or an extreme lower-bound study.

Model-side bit depth should be selected empirically:

- if int8-weight PPLX preserves row-int8-equivalent embeddings, it is already a
  useful win;
- if int4-weight PPLX also passes, it is a stronger compression win;
- if int4 fails but int8 passes, use int8 and spend effort on posting transfer
  instead of forcing int4;
- if both fail, try QAT or LoRA repair before rejecting the model-compression
  route.

## Main Risk

The main risk is repeating the earlier mistake: combining semantic imitation,
posting discretization, fanout control, and ranking optimization into one loss
too early.

The safer route is staged:

1. preserve PPLX dense;
2. compress dense;
3. emit posting;
4. control fanout;
5. optimize retrieval ranking.

Each step should have its own pass/fail gate.

## Immediate Next Actions

1. Implement M514 coordinate-load/load-balancing route objective.
2. Penalize high global coordinate document frequency while separately
   preserving dense teacher top-k recall.
3. Gate p96 and p128 first; do not force p64 unless p96 becomes stable.
4. If M514 improves p96/p128 candidate recall at low Touch, scale to Broad4.
5. If M514 fails, move to query-adaptive prefix/fanout instead of continuing
   scalar loss sweeps.
