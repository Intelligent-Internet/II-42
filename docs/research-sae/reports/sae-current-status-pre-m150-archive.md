# SAE Current Status And Engineering Readiness Report

Date: 2026-05-13
Last updated: 2026-05-24

## Executive Decision

The project has enough engineering evidence to keep a product-shaped
read-only evaluation harness, but the product model is not closed. M27 closed
without passing the dense-removal product gate, M29 closed the query-prefix
question, and M30 closed the scalar query-calibration attempt without passing
the robustness gate. M57-M59 have now closed the first post-M56 Stage-B push:
shared-encoder widening hurts teacher-shape fidelity, query-side fixed-doc
training improves over BM25 but remains far from the teacher, and full15 qrel
supervision alone does not close the gap. M60 scaled supervision is now closed
as useful but insufficient evidence. M70-M76 proved that the final BM25+SAE
ranking path is executable, but also showed that direct from-scratch
text-to-atoms final-ranking training falls into BM25-preserving fine-tuning.
M80 is the active model-side reset: return to a two-stage DiffSAE-style
pipeline, first train a strong dense retrieval student and only then compress
it to hard sparse atoms. M90 closed Stage A at `8192/k1024`, M93 compressed the
same gate to `8192/k768`, and M96 now closes the Stage-A compression gate at
`8192/k512` through checkpoint interpolation. This proves representation and
support-compression feasibility. M97 then validates `m96_k512` as the default
compressed checkpoint for the next read-only engine evaluation: candidate-surface
BM25+SAE ranking stays effectively tied with the M93 fallback while estimated
payload/doc-pair cost drops. It also exposes the next blocker: naive full-union
query execution opens the whole eval document set, and the current full-root M20
Python payload builder needs streaming/postings-driven work before SQL/API
productization. M98 starts the actual Stage-B ranking work and produces the
first positive ranking-calibration result: a runtime-safe BM25+SAE calibrator
trained over the full M81 split beats fixed BM25+SAE and BM25+dense on
validation, holdout, and combined eval. M99 then extends this into a
candidate-level residual ranker and materially improves validation, holdout,
combined eval, and the broad-generated family that remained weak after M98.
M100 confirms that this gain is robust across seed and residual-capacity
variants. M101 closes the immediate row-loop engineering bottleneck by porting
the ranker into a batched/exportable path with strict top-k parity. M102 then
wires the exported scorer into a minimal runtime scoring path and shows that
the scorer remains viable under candidate-budget sweeps, but also proves that
the existing M81 candidate surface is too small to validate true full-corpus
candidate control. M103 then implements postings-driven full-corpus candidate
generation over the M97/M81 eval corpus and exposes a distribution-shift
blocker: fixed BM25+SAE is more robust than the M101 residual scorer on the
new generated candidate pools. M104 addresses that blocker by retraining the
same residual-ranker shape on postings-generated rows initialized from fixed
BM25+SAE; it produces a small but consistent holdout/eval gain over fixed
scoring. M105 broadens that result across four postings-generated candidate
configs with query-stable splits and cost-aware selection. It preserves a small
but consistent gain over fixed BM25+SAE across holdout/eval averages and keeps
`d8_p16_bm25100` as the first compact runtime-contract candidate. M106 then
shows that Stage-B is still supervision-limited: simply increasing
qrel/pairwise weights or adding larger hard-negative pools does not help much,
while re-materializing more M81 human-qrel rows under the same `m96_k512`
representation improves NDCG deltas. M107 scales that clean path to the full
M81 train surface and proves the exported ranker still transfers back to the
original M105/M97 surface without retraining. However, M107's transfer ranking
delta is smaller than M105's in-surface ranking delta, so the active next step
was mixed-surface Stage-B selection/training before SQL-facing runtime work.
M108 now closes that simple mixing family: M97 transfer-aware selection
improves over M107 transfer, but direct M97 train-loss mixing and combined
M81+M97 normalization do not recover the M105 M97-local NDCG/MAP delta. M109
then reviews the independent `diffsae-codex` route and reframes the next step:
before adding another surface-aware residual ranker, normalize our candidate
bank and reproduce a DiffSAE-style objective where training loss, sparse
representation, and runtime sparse-index scoring are the same contract. The
first M109 aligned-training runs validate that direction on candidate-set
heldout: `8192/k64` reaches M97 `MRR@10 0.6685`, slightly above the dense
baseline `0.6673`, and `hit@10 0.8591`, well above dense `0.8129`. This is not
yet a product claim because full-corpus sparse-index evaluation is still
missing. M110 adds that full-corpus/index-contract validation and shows the
candidate-set gain does not fully transfer: SAE alone is below BM25, while
BM25+SAE improves BM25 but remains below dense. M111 then tests full-corpus
hard-negative refresh. It improves the refreshed candidate-set metrics beyond
dense, but regresses full-corpus SAE retrieval and increases postings touched.
M112-M115 close the first fanout-aware push: margin-only background negative
training can make the candidate surface strong, but it opens too many
full-corpus postings. Hard background-score control in M114 is useful because
it drops postings below M109 and slightly improves BM25+SAE fusion
Recall@100/MRR@10, but NDCG@10 and MAP@100 remain below M109. M116-M122 now
close the next SOTA push. Score-only recovery routes are negative: M117 keeps
candidate ranking but explodes postings, M118 fusion-weight tuning cannot close
the ranking gap, M119 does not recover ranking from M114, and M120 proves that
low random background score mean alone is not enough. M121 adds a posting
overlap proxy and becomes the first checkpoint that improves quality and cost
together. M123 then keeps the overlap-constrained objective and adds stronger
recall/teacher coverage pressure. M124 confirms the promoted profile:
`score_fusion_sae_weight=1.0` reaches fusion Recall@100 `0.4261`, NDCG@10
`0.3691`, MAP@100 `0.2363`, and about `39.95k` postings/query. M125 then
validates explicit dense teacher-neighborhood coverage as the next useful
objective. Its promoted `score_fusion_sae_weight=1.0` profile reaches
Recall@100 `0.4269`, MRR@10 `0.5035`, NDCG@10 `0.3729`, MAP@100 `0.2388`,
and about `36.74k` postings/query. M126-M129 then explore the remaining
teacher-coverage frontier. Top-30 coverage is mostly a cost-focused tradeoff,
while stronger top-20 coverage produces the current top-rank SOTA profile:
M128 with `score_fusion_sae_weight=1.0` reaches Recall@100 `0.4261`, MRR@10
`0.5077`, NDCG@10 `0.3755`, MAP@100 `0.2401`, and about `35.11k`
postings/query. M125 remains the highest Recall@100 balanced checkpoint. The
active next step should be data-side candidate-neighborhood refresh, not more
scalar loss or fusion sweeps on the same candidate bank.
M130 starts that next step. It saves the M129 frontier, adds same-surface
BM25+dense full-corpus controls, introduces miss taxonomy for dense/BM25+dense
hits that BM25+SAE misses, and evaluates `perplexity-ai/pplx-embed-v1-0.6B`
as a new semantic teacher/backbone beside Snowflake and Stella. The M130
promotion gate is no longer "improve BM25+SAE slightly"; it is to beat
BM25+dense on the same full-corpus index-contract evaluator while reporting
candidate docs, BM25 postings, SAE postings, rerank terms, and latency.
The first M130 baseline confirms PPLX is the strongest current teacher:
restricted to the same `25,863`-document M128 docset, PPLX dense reaches
Recall@100 `0.4600`, MRR@10 `0.5157`, NDCG@10 `0.4003`, and MAP@100
`0.2712`, versus M128 BM25+SAE at Recall@100 `0.4261`, MRR@10 `0.5077`,
NDCG@10 `0.3755`, and MAP@100 `0.2401`. The active M130 training target is
therefore PPLX-neighborhood coverage plus BM25-aware score calibration, not a
new scalar fusion sweep. M130 is also redirected away from legacy checkpoint
initialization: M128/Snowflake is a control only. The active model path is a
fresh PPLX/DiffSAE run trained from scratch on the largest available non-test
surface, with M97/M130 eval query IDs excluded from training qrels.
M130 Stage A/B/C now has full-corpus held-out results on the
`993,336`-document PPLX all-data eval surface. Stage B passed the canonical
BM25+dense gate but remained slightly behind dense-only MAP@100. Stage C closes
that gap: default Stage-C BM25+SAE reaches Recall@100 `0.3388`, MRR@20
`0.3648`, NDCG@10 `0.2490`, and MAP@100 `0.1583`, beating dense-only and
BM25+dense on all reported metrics. Its default `doc96/query96` sparse profile
is too expensive, so the active production-shaped profile is `doc_active_k=64`
and `query_active_k=80`: BM25+SAE reaches Recall@100 `0.3349`, MRR@20
`0.3688`, NDCG@10 `0.2521`, and MAP@100 `0.1626`, while reducing SAE
postings from `1.981M/query` to `1.536M/query` and sparse elapsed from about
`1.125s/query` to `0.735s/query`. A global DF stoplist is not promoted because
even a `5%` DF cap collapses sample recall and ranking quality.
M140 then preserves M130 as a milestone and runs a small hybrid-complement
probe over the currently completed official BEIR full-corpus artifacts. The
probe is not a promoted model result: it trains only a runtime-safe linear
scorer over existing BM25/SAE rankings. It does show useful direction. On the
completed strict-heldout datasets (`arguana`, `nfcorpus`, `scidocs`,
`scifact`), the BM25+SAE oracle candidate pool reaches Recall@100 `0.6948`,
roughly matching dense `0.6931`, while the learned scorer only improves
Recall@100 over fixed BM25+SAE (`0.6670` vs `0.6615`) and slightly regresses
NDCG/MAP. The active M140 interpretation is therefore: candidate coverage has
headroom, but ranking/calibration inside the BM25+SAE union is the blocker.
The next useful experiment is differentiable/listwise hybrid ranking, not
another scalar fusion sweep.
M140.1 then tests that next step with ClearML-tracked residual listwise
ranking. A direct scorer and a large residual are negative because they damage
the strong fixed-fusion head. A conservative residual scorer
(`residual_scale=0.03`) is small but positive: on the completed strict-heldout
surface, BM25+SAE improves from Recall@100/MRR@20/NDCG@10/MAP@100
`0.6615/0.4578/0.4051/0.3075` to `0.6615/0.4582/0.4056/0.3080`; on all-test
it improves from `0.6762/0.4982/0.4364/0.3348` to
`0.6762/0.4984/0.4366/0.3359`. This does not close the BM25+dense gap on
strict-heldout, but it confirms that bounded query-level hybrid training can
improve ordering without sacrificing Recall@100. The next M140 path should be
a two-band scorer: conservative residual within the fixed-fusion head plus a
separate admission gate for useful non-fixed SAE/BM25 candidates.
M140.2 implements that two-band Stage-D admission probe. The best quick
setting (`admission_base=0.0`, `admission_scale=0.30`) improves the completed
strict-heldout macro from `0.6615/0.4578/0.4051/0.3075` to
`0.6645/0.4604/0.4076/0.3105`, and all-test from
`0.6762/0.4982/0.4364/0.3348` to `0.6787/0.4997/0.4383/0.3370`.
This confirms Stage-D can be a useful calibration/admission layer. It does not
solve the `scidocs` Recall@100 failure: both strict-heldout and all-test remain
at `0.4220` versus BM25+dense `0.4705`. The remaining gap is therefore likely
in candidate generation or atom allocation for scientific semantic
neighborhoods, not in another downstream admission threshold.

Allowed now:

- M18: use the implemented `EATMH002` compact top-128 doc-row payload as the
  first compact read-only evaluation candidate.
- M19: use the implemented standalone C and PostgreSQL read-only by-id paths
  for deterministic EATMH001/EATMH002 parity testing.
- M20: use the implemented efficiency-only matrix runner for latency, fanout,
  payload size, memory, cache behavior, and diagnostics.
- M21: use the implemented runtime-query evidence-atom SQL path as the
  read-only evaluation harness after full15 Python/C/PostgreSQL parity.
- M27: keep the closure report as the latest model-side exit decision.
- M28: keep the post-M27 reset result as negative evidence for frozen
  pretrained projection and concept-vocabulary controls.
- M29: keep the prefix-aware ranking-first report as the latest blocker
  decision. The Snowflake prefixed teacher is canonical, but query-side
  text-to-atoms still fails robustness.
- M30: keep the robust query-side calibration report as negative evidence for
  scalar calibration. Calibration improves diagnosis and collapse severity, but
  does not pass the no-collapse gate.
- M70-M76: keep the end-to-end deep text-to-atoms implementation and
  diagnostics as reusable evidence. These runs prove the final BM25+SAE
  training path is executable, but they are no longer the active model line.
  Do not keep extending them with scalar loss/weight sweeps.
- M80: use the two-stage DiffSAE reset as the active model line. Stage A trains
  a strong dense retrieval student on a neutral mixed corpus and then proves
  hard sparse SAE preservation with a low sparsity tax. Stage B reintroduces
  BM25/qrel complementarity only after Stage A passes. `diffsae-codex` is now
  the main engineering reference because it already has adaptive TopK SAE,
  candidate-set training, BEIR general preparation, and Python/C++ sparse-index
  execution parity. M80-A0 is implemented: the neutral candidate-set corpus builder
  produced `/Volumes/Betty/Tmp/ii42_sae_m80/neutral-stage-a-v0` with
  28,351 documents, 540 queries, and 440 candidate rows. M80-A1 smoke is also
  implemented with a lightweight token dual encoder over candidate rows. That
  smoke validates the loss/data path but is not a promotable dense student.
  M80-A1 now also has a trainable SentenceTransformer/Snowflake-backed
  entrypoint with projection heads and Snowflake-safe attention config
  overrides. The first Spark A1 runs show that 256-dimensional random
  projection training is the wrong immediate path because it overfits the
  small candidate-row surface and degrades BEIR current-surface quality.
  Frozen 768-dimensional Snowflake geometry is the current ceiling on the
  M80-A0 V0 candidate eval, and low-LR last-2-layer tuning with
  `projection=truncate` slightly exceeds that ceiling. The active A1 route is
  therefore 768-dimensional teacher-geometry preservation with explicit
  best-epoch selection. The first 6-epoch curve peaks at epoch 2 for BEIR
  current-surface NDCG; later epochs increase Recall@10 but reduce MRR, so the
  next checkpoint candidate should save the epoch-2 profile before M80-A2
  sparse preservation. M80-A2 is now implemented as a sparse-tax harness. The
  first smoke validates the path, but 2048/k32 is not promotable because sparse
  Recall@10/MRR/NDCG@10 drops by roughly `-0.0856/-0.1164/-0.1143` versus the
  dense checkpoint on the M80-A0 candidate eval. Capacity helps, and A2.1 shows
  that validation selection is now mandatory: the previous 8192/k256 hard-TopK
  last-epoch result had sparse tax about `-0.0182/-0.0563/-0.0376`, while the
  best validation-selected 8192/k256 hard+pairwise point reaches about
  `-0.0154/-0.0278/-0.0177` for Recall@10/MRR/NDCG@10. A2.2 then adds a
  balanced selection gate, which selects a more retrieval-appropriate checkpoint
  with tax about `+0.0039/-0.0295/-0.0202`. This is the strongest practical
  sparse-preservation evidence so far, but it is not final because k256 is
  expensive. Soft and straight-through TopK were tested and parked as primary
  fixes: ST k256 improves candidate coverage but worsens top-rank tax under
  hard export. Naive activation-value flattening (`sqrt`, `log1p`, `binary`) is
  also parked because it damages MRR/NDCG. A naive independently initialized
  query/document asymmetric SAE was also parked because it breaks latent
  coordinate alignment and worsens sparse tax. A2.3 then tested calibrated
  sparse scoring and budget-aware k160/k224 selection. The best raw-dot
  calibration only slightly improves the k256 tax to about
  `+0.0039/-0.0289/-0.0193`, while k160/k224 still miss k256 badly. A focused
  expanded Stage-A surface (`36,867` documents, `1,013` queries, `812`
  candidate rows) exposes a larger tax around `-0.0193/-0.0571/-0.0477`,
  dominated by broad generated query rows. Stage B remains blocked. The next
  model step should change the representation/training objective itself:
  retrieval-aware atom allocation, active-budget-aware support learning, and a
  larger Stage-A surface. It is still research; it does not authorize
  dense-removal product claims.
- M130: use `docs/research-sae/reports/m0100-m0199/sae-m130-bm25-sae-super-recall-plan.md` as the single canonical
  `bm25sae` training plan. Same-surface PPLX baselines and miss taxonomy are
  complete enough to promote PPLX as the main teacher/control. The next active
  line is no longer the bridge `m130-pplx-diffsae-*` candidate-surface route:
  train a fresh full-data PPLX/DiffSAE representation model under the
  `bm25sae-` run family, then run BM25-aware ranking and full-corpus
  hard-negative refresh against the fixed BM25+PPLX dense gate.
- M90: promote Stage-A closure evidence. The promoted profile is a single
  interpolated `8192/k1024` sparse encoder between the recall-selected M89
  checkpoint and the MRR-selected M88 checkpoint. The clean canonical point is
  `alpha=0.35` with no extra score calibration, producing overall
  Recall@10/MRR/NDCG@10 tax about `-0.00449/-0.01864/-0.00844` and passing
  every source-family Recall/NDCG gate. This closes Stage A at the
  representation level. It is superseded for query-time support cost by the
  later M96 `k512` checkpoint, but remains the high-support reference.
- M91-M92: close the first support-compression push negatively but with useful
  direction. M91 added sparse-teacher compression from the promoted M90
  checkpoint. `k384` is too aggressive, while `k512` is close but not closed:
  the best clean `k512/teacher025` run has sparse tax about
  `-0.0066/-0.0210/-0.0152`. Stronger teacher pressure improves MRR/NDCG but
  loses too much recall. M92 then interpolated the best `k512` checkpoint back
  toward M90; the best point, `alpha=0.25/rawp005`, reaches about
  `-0.00565/-0.02136/-0.01314`, so aggregate Recall/NDCG pass but MRR still
  misses and BEIR-current Recall@10 remains below the source-family gate.
  This parks direct `k512` promotion and moves the active work to M93
  coverage-aware support compression at `k640/k768`.
- M93: promote coverage-aware support compression. The promoted point is
  `m93-k768-teacher025` with baseline scoring, initialized from M90 and trained
  with M90 as a sparse teacher at weight `0.25`. It reduces active support from
  `k1024` to `k768` and passes the unchanged Stage-A gate with overall
  Recall@10/MRR/NDCG@10 tax about `-0.00390/-0.01943/-0.01023`. BEIR-current
  Recall@10 tax is positive at about `+0.00521`, which fixes the dominant M92
  coverage failure. `k640` remains close but not promoted because it still
  misses overall Recall@10. M93 is the safe compressed Stage-A fallback and was
  used as the teacher for the successful follow-up `k512` compression attempt.
- M94-M96: promote `8192/k512` Stage-A compression. Direct M94 `k512` training
  from M93 was close but failed overall MRR and the large-supervised Recall@10
  family gate. M95 family-balanced repair worsened aggregate recall and is
  parked. M96 interpolated the two compatible M94 k512 endpoints and promoted
  `interp_alpha_0.35` with baseline scoring. The promoted point has overall
  Recall@10/MRR/NDCG@10 tax about `-0.00495/-0.01855/-0.01058`, with
  family Recall@10 tax about `-0.00025/-0.00590/+0.00000` for BEIR current,
  broad generated, and large supervised surfaces. This is now the canonical
  Stage-A compressed checkpoint, while M93 remains the fallback.
- M97: promote `m96_k512` for read-only engine evaluation, but not for product
  claims. On the M81 eval candidate surface (`886` rows, `30059` docs, `886`
  queries), `m96_k512_bm25_sae_w1` reaches Recall@10/MRR/NDCG@10
  `0.3500/0.6592/0.5098`, effectively tied with `m93_k768_bm25_sae_w1`
  (`0.3509/0.6575/0.5092`) and clearly above BM25 (`0.3178/0.5960/0.4505`).
  The `w2` point reaches `0.3560/0.6669/0.5180`. Estimated payload cost drops
  to `145.4MB` and `15.39M` SAE doc-pairs, versus M93's `204.1MB` and
  `23.09M` SAE doc-pairs. The negative finding is equally important: naive
  full query-atom union touches all `30059` docs on average, so the next engine
  task is fanout-aware candidate control and streaming/postings-driven M20
  full-root payload build. The M20 subset smoke over `1149` docs and `20`
  queries passes Python/C exact parity with C mean latency about `0.8045ms`,
  but this is only a path smoke, not a full-root readiness result.
- M98: promote Stage-B ranking calibration as the active next model line. M98A
  trains a runtime-safe scoring calibrator on the full M81 candidate surface
  (`3920` train rows, `486` validation rows, `400` holdout rows). Dense is used
  only as a BM25+dense teacher/control, not at runtime. The best checkpoint is
  selected at epoch `70`. On combined eval, `m98_calibrated` reaches
  Recall@10/MRR/NDCG@10 `0.3608/0.6729/0.5219`, above `BM25+dense w2`
  (`0.3530/0.6631/0.5137`) and fixed `BM25+SAE w2`
  (`0.3564/0.6649/0.5177`). Holdout also improves:
  `0.3817/0.6687/0.5304` versus fixed BM25+SAE `0.3721/0.6611/0.5247`.
  This confirms that Stage-B ranking adjustment has real signal. The remaining
  model gap is broad-generated query NDCG/MRR, where calibrated BM25+SAE
  improves over fixed BM25+SAE but remains slightly behind BM25+dense. The next
  model step should deepen Stage-B with query-side ranking/listwise training,
  not return to Stage-A compression.
- M99: promote candidate-level Stage-B residual ranking as the current best
  candidate-surface result. M99 keeps `m96_k512` Stage A frozen and trains a
  small runtime-safe residual ranker over BM25/SAE score, rank, margin, and
  query distribution features. Dense is still only a BM25+dense teacher/control
  during training. Best epoch is `150`. On combined eval, M99 reaches
  Recall@10/MRR/NDCG@10 `0.4049/0.6905/0.5576`, above M98
  `0.3608/0.6729/0.5219`, fixed BM25+SAE w2 `0.3564/0.6649/0.5177`, and
  BM25+dense w2 `0.3530/0.6631/0.5137`. Holdout also improves from M98
  `0.3817/0.6687/0.5304` to `0.4296/0.6869/0.5704`. The previously weak
  broad-generated family improves from M98 `0.2807/0.6433/0.4651` to
  `0.3313/0.6632/0.5062`, passing BM25+dense on that family as well. This is a
  Stage-B ranking breakthrough, but it is still candidate-surface ranking and
  does not solve full-corpus candidate generation or native index execution.
- M100: promote M99's candidate-level residual direction as robust enough to
  become the current Stage-B mainline. The robustness sweep reruns the ranker
  with a different seed, lower residual capacity, and no broad-family training
  upweighting. All variants stay clearly above M98 on combined eval. The best
  seed123 variant reaches Recall@10/MRR/NDCG@10 `0.4036/0.6901/0.5594`; the
  no-broad-weight variant reaches `0.4025/0.6940/0.5570`; and the conservative
  residual reaches `0.3949/0.6796/0.5479`. This confirms the Stage-B signal is
  not a single-seed artifact. Broad-family upweighting is not required for the
  core gain. The remaining blocker is engineering the ranker into a fast,
  batched, exportable runtime scoring path and then validating full-corpus
  candidate generation.
- M101: promote the batched/exportable Stage-B ranker path. M101 ports the
  M99/M100 ranker from row-level Python loops to padded tensor training and
  scoring while preserving the same BM25/SAE-only runtime feature contract.
  The full 220-epoch Spark CUDA run finishes in `6.00s`; best epoch is `180`.
  On combined eval, M101 reaches Recall@10/MRR/NDCG@10
  `0.4036/0.6930/0.5602`, matching the M100 quality envelope and staying well
  above M98. Holdout reaches `0.4259/0.6927/0.5733`. Export parity is strict:
  row-loop vs batched/exported scoring has max absolute score delta
  `0.00000191` and `0` top-10 mismatches over `4806` rows. This closes the
  Stage-B row-loop bottleneck. It still does not solve full-corpus candidate
  generation or native SQL/API productization.
- M102: promote the exported M101 scorer as the Stage-B runtime scoring
  component for the next read-only evaluation step, but not the candidate
  surface as a product path. M102 sweeps BM25/SAE candidate budgets over the
  existing M81 surface. Full-surface M101 reaches Recall@10/MRR/NDCG@10
  `0.4036/0.6930/0.5602` with mean `118.7` candidates. A low-cost
  `bm255_sae10` profile uses mean `12.8` candidates and still beats fixed
  full-surface BM25+SAE on NDCG@10 (`0.5191` vs `0.5177`), but it is far below
  M101 full-surface quality. The near-lossless `bm255_sae120` profile reaches
  `0.4049/0.6908/0.5590`, but still uses mean `114.7` candidates. This proves
  the scorer is reusable and the next blocker is postings-driven full-corpus
  candidate generation with real posting/rerank costs.
- M103: promote the postings-driven simulator as the correct Stage-B
  evaluation harness, but do not promote M101 to SQL execution yet. M103 runs
  over the full M97/M81 eval document set: `30059` docs, `886` queries,
  `2.80M` BM25 postings, and `15.39M` `m96_k512` SAE postings. Full BM25
  reaches Recall@100/MRR@20/NDCG@10/MAP@100
  `0.4063/0.4513/0.3434/0.2348`. A compact postings candidate point,
  `d8_p16_bm25100`, uses mean `204.3` candidates and `128.0` SAE generation
  postings; fixed `BM25+SAE w2` reaches `0.4231/0.5138/0.4081/0.2840`, while
  the exported M101 ranker reaches `0.4154/0.5134/0.3973/0.2747`. Larger
  candidate pools improve fixed-score Recall, but M101 degrades further
  relative to fixed scoring. This shows that M101 learned the M98 candidate
  surface distribution and must be retrained or recalibrated on
  postings-generated candidate rows before read-only PostgreSQL integration.
- M104: promote postings-generated Stage-B retraining as the next ranker
  direction. M104 trains on the compact `d8_p16_bm25100` candidate config,
  initialized from fixed `BM25+SAE w2` and using qrels plus fixed-score teacher
  supervision. It rebuilds query and candidate feature normalization from the
  postings-generated train split. Best epoch is `100`; elapsed time is
  `66.79s` on Spark CUDA. On holdout, fixed `BM25+SAE` reaches
  Recall@100/MRR@20/NDCG@10/MAP@100 `0.4239/0.5338/0.4307/0.2891`; M104
  reaches `0.4249/0.5439/0.4347/0.2959`. On eval, fixed reaches
  `0.4355/0.5217/0.4289/0.2940`; M104 reaches
  `0.4366/0.5317/0.4332/0.2986`. This is a modest but consistent distribution
  repair. It is not yet product-ready because it only covers one compact
  candidate config; the next step should train/evaluate over multiple configs
  with a candidate-cost-aware gate.
- M105: promote the postings-trained Stage-B route across multiple candidate
  configs. M105 trains one residual ranker across `d8_p16_bm25100`,
  `d8_p32_bm25100`, `d16_p16_bm25100`, and `d16_p32_bm25100`, with the same
  query assigned to the same split in every config. Best epoch is `100`;
  elapsed time is `160.45s` on Spark CUDA. Average holdout fixed
  `BM25+SAE` reaches Recall@100/MRR@20/NDCG@10/MAP@100
  `0.4277/0.5342/0.4314/0.2905`; M105 reaches
  `0.4294/0.5446/0.4358/0.2974`. Average eval fixed reaches
  `0.4425/0.5226/0.4297/0.2956`; M105 reaches
  `0.4447/0.5327/0.4340/0.3003`. The recommended compact point remains
  `d8_p16_bm25100`, with mean `204.3` candidates and `128.0` SAE generation
  postings. This authorizes a narrow read-only runtime contract experiment,
  not SQL/API productization.
- M106: reopen Stage-B data scale before SQL work. A qrel-heavy loss-only
  probe (`m106-qrel-hard-v1`) and an expanded hard-negative candidate probe
  (`m106-expanded-hardneg-v1`) do not materially improve M105; compact
  `d8_p16_bm25100` is effectively unchanged. The useful signal comes from
  re-materializing 2,000 M81 train rows with the current `m96_k512`
  representation, then training the same postings Stage-B ranker. This
  expands the surface to `55,118` docs, `2,000` queries, and `8,000` runtime
  rows. On that surface, average eval fixed `BM25+SAE` reaches
  Recall@100/MRR@20/NDCG@10/MAP@100 `0.4439/0.5451/0.4505/0.3221`; M106C
  reaches `0.4457/0.5550/0.4575/0.3265`, improving eval NDCG delta to
  `+0.0071` versus M105's `+0.0044`. This supports the hypothesis that
  Stage-B is supervision-limited. M107 should scale this to the full M81 train
  split and then test whether the larger-supervision ranker transfers back to
  the original M105/M97 eval surface.
- M107: promote larger-supervision Stage-B as a positive but not final
  SQL-facing result. M107 materializes the full M81 train split with
  `m96_k512`, producing `71,417` documents, `3,920` queries, and `15,680`
  runtime rows across the same four M105 candidate configs. Training selects
  epoch `240` and takes `922.38s` on Spark CUDA. On its own full-train eval
  surface, fixed `BM25+SAE` reaches Recall@100/MRR@20/NDCG@10/MAP@100
  `0.4104/0.4748/0.3844/0.2778`; M107 reaches
  `0.4133/0.4862/0.3911/0.2819`. The exported M107 ranker transfers back to
  the original M105/M97 eval surface without retraining: fixed reaches
  `0.4425/0.5226/0.4297/0.2956`; M107 transfer reaches
  `0.4464/0.5285/0.4319/0.2976`. This is a transfer pass, but it does not
  fully preserve M105's original M97 ranking delta
  (`+0.0022/+0.0100/+0.0044/+0.0047`). The next step should be M108
  mixed-surface selection/training: keep full M81 supervision while explicitly
  selecting against the M105/M97 transfer surface before any SQL runtime
  contract work.
- M108: close simple mixed-surface Stage-B negatively. M108 adds the original
  M105/M97 surface as a transfer validation surface while training on the full
  M81 train rows, then tests direct M97 train-loss mixing and combined
  M81+M97 feature normalization. All variants stay positive against fixed
  BM25+SAE, but none recover M105's M97-local ranking delta. The best variant
  is selection-only: transfer eval delta vs fixed reaches
  Recall@100/MRR@20/NDCG@10/MAP@100 `+0.0041/+0.0050/+0.0029/+0.0025`,
  compared with M107 transfer `+0.0040/+0.0058/+0.0022/+0.0020` and M105
  in-surface `+0.0022/+0.0100/+0.0044/+0.0047`. Transfer train weights `0.35`
  and `1.0`, plus combined normalization, do not improve this. The next step
  should be M109 surface-aware residual/calibration with runtime-safe query and
  candidate descriptors, not further mixture-weight sweeps.
- M81: active preparation. M80-A2.3 proved that the focused `36,867` document
  surface is still too narrow, so M81 shifts the immediate work to a larger
  Stage-A surface and dense-vector cache before additional model changes.
  Spark is the primary data/training host because it has M52, M39, M60, and
  BEIR15 data. Flora and xiaoni are useful for source control and light
  validation, but they currently lack the full data stack. The first M81 target
  is `/home/huoju/leask/data/ii42_sae_m81/large-stage-a-v0`, with
  200k M52 representation docs, 50k M52 queries, full cached BEIR/M39
  supervised rows, capped M60 rows (`20k` docs / `300` queries for the first
  stable run), and candidate depth 128. Full M60 is intentionally deferred
  until dense candidate construction is cached or vectorized. M90 proved stable
  Stage-A preservation on this larger surface at `k1024`; M96 now compresses
  that route to `k512`. Stage B should use M96 as the default compressed
  checkpoint, but still needs separate ranking and physical payload validation.
- M31: keep the joint final-ranking result as earlier positive training
  evidence. It confirms that final-ranking supervision plus BM25-preservation
  has signal, but the first pass still fails the no-collapse gate.
- M32: keep the large-split final-ranking result as the latest model-side
  evidence. Clean official train/dev data improves aggregate quality, but the
  model still fails the no-collapse gate on `trec-covid`, `msmarco`, and
  `dbpedia-entity`; M32 is not promoted.
- M33: keep the hard-family pseudo-query result as negative evidence for
  corpus-title self-qrels. The arm gives only weak `trec-covid` MAP recovery
  and does not solve semantic recall or NDCG; do not promote it.
- M34: keep the `trec-covid` difficulty analysis as the latest diagnostic.
  The blocker is a general high-DF broad-query / dense-qrel neighborhood
  problem, not a reason to add dataset-specific tuning.
- M35: keep the teacher-neighborhood pseudo-query result as negative evidence
  for document-sentence questionization. The teacher-neighborhood mechanism is
  useful as tooling, but the tested pseudo-query distribution does not fix
  broad-query collapse and should not be swept further.
- M36: keep the query-distribution reset result as negative evidence for
  current real-query expansion and default M35/M35b synthetic sources. The
  audit found only 3 TREC-like broad/many-positive queries in M32 train versus
  50 in current eval. Expanded `nfcorpus` train/dev replay lowers SAE postings
  but still fails the no-collapse gate.
- M37: keep the query-encoder depth result as negative evidence for simply
  adding transformer capacity on the same M32 data/objective. The transformer
  arm slightly improves `trec-covid` ranking, but collapses full15 aggregate,
  `msmarco`, `dbpedia-entity`, and `nfcorpus`.
- M38: keep the hard-bucket weighting result as negative evidence for
  reweighting the current expanded real-query training surface. It preserves
  M36 aggregate quality, but does not improve hard-dataset collapse.
- M39: keep the validated broad-query generator as a positive supervision
  signal but failed product gate. The generated source passes distribution
  validation and improves `trec-covid` under lower SAE weight, but global
  quality and other hard datasets regress at that setting.
- M40: keep lexical-DF scale gating as the current strongest positive
  direction. It does not pass the strict product gate and increases SAE
  postings, but it improves both full15 NDCG/MAP and `trec-covid` MAP versus
  M36.
- M41: keep exported-atom fanout regularization as positive cost-control
  evidence for the M40 route. It does not dominate M40 on quality, but it
  creates a useful quality-cost frontier and shows the M40 postings increase
  is trainable.
- M42: promote utility/fanout-aware payload allocation as the current strongest
  quality-cost direction. The best result uses the M40-trained query atom pool
  and selects the exported payload by atom weight divided by fanout cost; this
  preserves M40 quality while cutting SAE postings roughly in half.
- M43: promote the integrated M42 payload allocation path as the current
  canonical research profile. It reproduces the M42 sweep exactly from the
  normal query-latent export/evaluation path and fixes the lexical feature vs
  gate-threshold semantic split.
- M44: promote pool-aware export as the current strongest quality-cost profile.
  The best result uses the M40 checkpoint with pool96 -> export48,
  fanout_power `0.25`, and lexical-DF gate `threshold=0.12, low=0.40,
  high=1.00`. Export-aware finetuning was tested but is not promoted.
- M45: keep checkpoint export sweep as the latest payload-frontier result.
  It promotes a high-quality profile, pool96 -> export48, fanout_power `0.15`,
  gate `threshold=0.12, low=0.45, high=1.00`, while keeping M44 and the M45
  pool128 balanced profile as lower-cost comparison points.
- M46: promote runtime-safe profile selection as the current canonical
  research profile. The best quality-cost selector routes high predicted
  fanout queries to M44 and keeps the rest on M45 high-quality, improving
  full15 NDCG/MAP versus M45 high-quality while lowering SAE postings.
- M47: park cost-aware exported-ranking finetuning. It lowers postings, but
  loses too much NDCG/MAP and worsens hard-dataset stability versus M46.
- M48: keep the M46 selector after LODO and family holdout robustness checks.
  The canonical `fanout_to_m44@3000` rule is selected in 13/15 LODO splits,
  and the two deviations are worse on the heldout datasets.
- M49: keep the runtime selector contract as the first engineering handoff
  point. It records one encoder, multiple runtime export profiles, and the
  `fanout_to_m44@3000` selector as a research contract, not a SQL/API freeze.
- M50: keep the hard-dataset semantic-retention diagnostic as positive but
  incomplete evidence. Larger export budgets repair current `trec-covid` and
  `msmarco` regressions versus M46/M49, but no tested profile beats BM25 on
  both datasets for both NDCG@10 and MAP@100.
- M52: earlier model reset. Split training into large-corpus teacher-imitation
  pretraining and multi-source supervised ranking fine-tuning. The first
  support-ranking-loss smoke is closed negatively: it improves observability
  and training speed after vectorization, but does not improve teacher support
  recall versus the M52A plain pretrain. The balanced arXiv/PubMed/BEIR 100k
  plain pretrain is now positive: docs teacher recall improves from `0.1889`
  to `0.2219`, and queries improve from `0.1855` to `0.1993`. The immediate
  work is now Stage-A medium scaling with a broad, balanced corpus, not another
  small rank-loss sweep. M52E has started teacher materialization for a
  747,192-unit no-Wikipedia medium shard. Spark/GB10 is now available as a
  monitored training host for follow-up runs, with optional Aim tracking at
  `http://100.123.2.95:43800/`. Aim is intentionally opt-in through
  `--aim-repo`; existing xiaoni jobs are not interrupted or retrofitted.
- M53: keep differentiable retrieval fine-tuning as a Stage-B research tool,
  not the Stage-A quality target. Its main value is diagnosis and loss-shaping;
  Stage-A should prioritize teacher-shape retention before recall.
- M54: keep the Stage-A distillation result as the current checkpoint base.
  The `m54-e2-clean` checkpoint is the active baseline because it preserves
  teacher shape better than later longer/continued runs.
- M55: park rank-heavy Stage-B anchoring. It proves ranking signal exists, but
  damages teacher-shape fidelity too much to scale.
- M56: keep safe Stage-B anchoring as positive two-dataset evidence. It
  improves the small ranking sanity matrix while mostly preserving M54
  neighborhood fidelity, but it requires wider validation.
- M57: close shared-encoder widening negatively. Scaling the M56-style balance
  to `scifact/nfcorpus/fiqa/scidocs` improves some ranking metrics, but
  `safe_anchor` and `shape_lock` both lose teacher-shape fidelity versus M54.
- M58: keep fixed-doc query-side training as the cleanest active diagnostic.
  It improves over BM25 and isolates query-side failure, but the best
  four-dataset run still trails teacher by roughly `0.048-0.062` on the core
  metrics and fails robustness.
- M59: close full15 supervised query-side training negatively. Training the
  same fixed-doc query-side model on all 15 BEIR qrel sets does not improve the
  full15 teacher gap, so more BEIR labels on the same encoder/objective are
  not enough.
- M60: closed, useful but insufficient. The source registry now
  gates train/eval counts before launch; the Spark cache was corrected from a
  partial two-dataset BEIR eval root to full15. M60A unfrozen broad training is
  negative: it improves over BM25 but is worse than M59 and far from the
  teacher. M60A frozen-control is a small positive diagnostic: full15
  Recall@100 `0.8076`, NDCG@10 `0.6884`, MAP@100 `0.6509`, and `trec-covid`
  MAP improves to `0.6236`, but the teacher gap remains too large. M60B adds an
  official MS MARCO 100k-doc / 2k-query source and fixes the training artifacts
  to slim text/qrel/SAE-latent form. The M60B frozen-control confirms
  head-only training cannot absorb the new source: the learned calibrated head
  improves to Recall@100 `0.8007`, NDCG@10 `0.6861`, MAP@100 `0.6468`, but the
  selected fixed-weight path remains the same as M60A frozen. The M60B
  guarded-unfrozen run confirms official MS MARCO labels produce small local
  gains on `msmarco` and `cqadupstack`, but full15 best remains below
  frozen-control: Recall@100 `0.7997`, NDCG@10 `0.6868`, MAP@100 `0.6473`.
  Next M60 work should change the training structure or supervision quality:
  true B0/B1 staged training, TREC DL-style deeper judgments, and ranking cache
  before raising the query cap. Do not simply add epochs or loss-only sweeps to
  the same objective.
- M70-M76: closed as implementation and diagnostic evidence. The original
  recommendation was to stop extending the M60-style shallow model family and
  start an all-in end-to-end reset: full-scale corpus representation
  pretraining, strict split/leakage manifests, large supervised
  query/judgment training, a deeper asymmetric text-to-atoms encoder,
  differentiable hard top-k sparse retrieval, BM25-complementarity loss,
  physical-cost loss, and final BM25+SAE ranking supervision. Initial
  implementation has
  started: the M70 source registry v2 reports zero current train/eval query-id
  overlaps, and the first from-scratch local smoke proves the new text ->
  sparse atoms -> dynamic BM25+SAE ranking path is executable. The smoke is not
  a quality claim; it is a reproducibility gate before scaling. The first
  Spark M39 broad from-zero run also completed: 64816 documents, 1453 train
  queries, 594 eval queries, 1024 latent dims, 8 epochs on CUDA. It is stable
  but not promoted: student Recall@100 `0.6477` is slightly below BM25
  `0.6484`, while MRR/MAP are near-ties. The result confirms the objective
  scales, but it is currently too conservative and collapses toward
  BM25-preserving low-SAE-scale behavior. The follow-up coverage smoke fixes
  one concrete flaw: all qrel positives now stay in the candidate pool, dense
  and SAE teacher near-miss candidates are added, and semantic coverage loss
  trains SAE scores directly. On the local two-dataset smoke, student
  Recall@100 improves to `0.5404` versus BM25 `0.5258`, NDCG@10 improves to
  `0.4733` versus `0.4705`, and SAE scale remains active. This is enough to
  justify a broader Spark run, but not enough to promote the model. The
  follow-up soft-teacher target keeps dense and SAE teacher near-miss scores as
  SAE-only semantic supervision. A light local setting
  (`semantic_teacher_weight=0.10`) improves the two-dataset smoke to
  Recall@100 `0.5442`, NDCG@10 `0.4717`, MAP@100 `0.3542`, but heavier teacher
  weights regress. The scaled Spark soft-teacher run used 120,816 documents,
  2,007 train queries, 802 eval queries, 2,048 latent dims, and 18 epochs. It
  keeps all 43,596 qrel positives in the pool and slightly improves MRR/NDCG/MAP
  over BM25, but Recall@100 falls from BM25 `0.6602` to student `0.6596`.
  SAE scale still decays from `0.2403` to `0.1042`. A simple staged semantic
  pretrain smoke and stronger coverage-weight sweeps did not beat the light
  single-stage soft-teacher smoke. Current conclusion: M70's implementation is
  sound, but the semantic target/candidate-label structure still needs a deeper
  reset before another large single-stage sweep. M71 tested that reset by
  adding teacher-neighborhood loss and separating stage weights. The local
  two-dataset smoke found a real signal: mixed dense/SAE neighborhood
  `w=0.15` reaches Recall@100 `0.5420`, MRR@20 `0.6244`, NDCG@10 `0.4753`,
  MAP@100 `0.3546`; dense-dominant targets improve NDCG/MAP but not Recall.
  The medium Spark validation rejects the same global-loss structure:
  M71 mixed `w=0.15`, mixed `w=0.05`, and dense-dominant `w=0.15` all remain
  below the M70 coverage baseline on Recall@100. M71 therefore closes as a
  useful negative result: embedding teacher should be stronger, but as
  curriculum/candidate mining or confidence-gated residual training, not as a
  blind global single-stage ranking loss. M72 tested the confidence-gated
  residual version directly: teacher-only semantic targets were added to the
  final listwise target only with `low_bm25` gating. Locally,
  `teacher_residual_weight=0.05` plus light teacher KL `0.10` is a balanced
  smoke result with Recall@100 `0.5421`, MRR@20 `0.6085`, NDCG@10 `0.4708`,
  MAP@100 `0.3538`, all above BM25 on `scifact+nfcorpus`. Medium Spark
  validation rejects the structure: residual `0.05` falls to Recall@100
  `0.6644`, and residual `0.02` reaches only `0.6649`, both below the M70
  coverage baseline `0.6662`. M72 therefore closes as another useful negative:
  teacher residuals need query-family routing/curriculum, not another
  single-stage residual weight sweep. M73 added query-family diagnostics
  (`short_keyword`, `long_query`, `broad_many_positive`, `lexical_heavy`,
  `semantic_heavy`) and tested `semantic_or_broad` residual gating. Locally it
  works on `scifact+nfcorpus`: Recall@100 `0.5311`, MRR@20 `0.6085`,
  NDCG@10 `0.4724`, MAP@100 `0.3544`, all above BM25. Spark medium diagnostics
  reject it as a training gate: aggregate Recall@100 falls to `0.6646` versus
  BM25 `0.6661`, with NDCG/MAP only slightly higher. Family metrics show
  long/lexical queries unchanged, but short/semantic/broad recall still drops
  on medium. M73 promotes the diagnostics and reporting path, not residual
  family-gated training. The next direction is candidate-source utility and
  curriculum: use dense/SAE teacher signal to choose better training examples
  and hard candidates, not to rewrite every query's ranking target. M74
  implements that diagnostic route. On local `scifact+nfcorpus`, teacher_union
  recovers 157 qrel positives missed by BM25 top-60, but hard
  `teacher_extra_positive` filtering keeps only 43/112 training examples and
  falls back to Recall@100 `0.5280`, worse than the no-curriculum coverage
  smoke `0.5410`. On the M39 medium train surface, teacher_union recovers
  6,125 qrel positives missed by BM25 top-128, adding extra recall `0.2162`,
  but those extra hits appear in only 474/1,750 queries. M74 therefore promotes
  candidate-source utility diagnostics and rejects hard filtering. The next
  route should keep all queries and use source-utility weighting or stratified
  sampling. M75 implements per-candidate source-utility weighting. Local smoke
  confirms the signal: `source_utility_weight=1.0` improves Recall@100 to
  `0.5512` versus the M74 no-curriculum `0.5410`, but MRR/NDCG drop. The M39
  medium Spark run is weaker but directionally useful: it improves slightly
  over the M73 family baseline rerun (`0.6649/0.6693/0.5858/0.4950`), while
  still losing Recall@100 versus BM25 `0.6661`. The weighted-positive signal is
  too diluted at candidate granularity: 5,432 weighted positives inside the
  candidate tensors produce mean utility weight only `1.0169`. M75 promotes the
  reusable utility-weight machinery, but not per-candidate weighting as the
  mainline. The next route should be query-level source-utility sampling or a
  candidate-budget target. M76 implements query-level source-utility row
  weighting and closes it negatively. Local `scifact+nfcorpus` smoke shows row
  weighting is blunter than M75 candidate weighting: `extra_positive_any,w=1.0`
  reaches only Recall@100 `0.5270`, `any,w=0.25` reaches `0.5254`, and
  `extra_positive_fraction,w=1.0` reaches `0.5290`, all below M74 no-curriculum
  `0.5410` and M75 candidate `w=1.0` `0.5512`. M76 therefore keeps the reusable
  row-weight plumbing but rejects row weighting as the next training route.
  The originally proposed M77 candidate-budget target is parked until the
  representation problem is cleaner. M80 supersedes it as the active route:
  first reproduce the DiffSAE two-stage shape, then bring candidate-budget
  complementarity back in Stage B.
- M80: active planning. The route is now explicitly two-stage. Stage A focuses
  on representation and sparse preservation: train a Snowflake-backed dense
  student on neutral mixed candidate/group data, then train a TopK SAE to
  preserve that dense retrieval geometry under hard sparse atoms. Stage B
  reintroduces BM25/qrel final-ranking and teacher-extra candidate-budget
  objectives only after Stage A proves a low sparse tax. Policy corpora are
  held-out/domain probes first, not the first promotion corpus.

Not allowed yet:

- stable product API freeze;
- mutable access method or maintenance design;
- dense-vector removal claims;
- real-workload Recall/MRR/NDCG/MAP claims without qrels or defensible
  proxy-qrels.

Latest milestone snapshot:

```text
sae-milestone26-softsae-adaptive-sparsity-report.md
sae-m21-runtime-evidence-atom-mainline-report.md
sae-m20-real-corpus-pilot-report.md
sae-m27-exploration-closure-report.md
sae-m28-post-m27-next-phase-plan.md
sae-m28-post-m27-results-report.md
sae-m29-prefix-aware-ranking-results-report.md
sae-m30-robust-query-calibration-results-report.md
sae-m31-joint-final-ranking-results-report.md
sae-m32-large-split-final-ranking-training-plan.md
sae-m32-large-split-artifact-build-report.md
sae-m32-large-split-training-results-report.md
sae-m33-hard-family-supervision-plan.md
sae-m33-hard-family-supervision-results-report.md
sae-m34-trec-covid-difficulty-analysis-report.md
sae-m35-teacher-neighborhood-distillation-plan.md
sae-m35-teacher-neighborhood-distillation-results-report.md
sae-m36-query-distribution-reset-plan.md
sae-m36-query-distribution-reset-results-report.md
sae-m37-query-encoder-depth-plan.md
sae-m37-query-encoder-depth-results-report.md
sae-m38-hard-bucket-weighting-plan.md
sae-m38-hard-bucket-weighting-results-report.md
sae-m39-validated-broad-query-generator-plan.md
sae-m39-validated-broad-query-generator-results-report.md
sae-m40-lexical-df-scale-gate-plan.md
sae-m40-lexical-df-scale-gate-results-report.md
sae-m41-gate-aware-fanout-regularization-plan.md
sae-m41-gate-aware-fanout-regularization-results-report.md
sae-m42-utility-aware-payload-allocation-plan.md
sae-m42-utility-aware-payload-allocation-results-report.md
sae-m43-canonical-payload-allocation-plan.md
sae-m43-canonical-payload-allocation-results-report.md
sae-m44-pool-aware-export-regime-plan.md
sae-m44-pool-aware-export-regime-results-report.md
sae-m45-checkpoint-export-sweep-plan.md
sae-m45-checkpoint-export-sweep-results-report.md
sae-m46-runtime-profile-selector-plan.md
sae-m46-runtime-profile-selector-results-report.md
sae-m47-cost-aware-exported-ranking-plan.md
sae-m47-cost-aware-exported-ranking-results-report.md
sae-m48-selector-holdout-robustness-plan.md
sae-m48-selector-holdout-robustness-results-report.md
sae-m49-runtime-selector-contract-plan.md
sae-m49-runtime-selector-contract-results-report.md
sae-m50-hard-dataset-semantic-retention-plan.md
sae-m50-hard-dataset-semantic-retention-results-report.md
sae-m51-retention-calibration-plan.md
sae-m52-large-corpus-semantic-pretraining-plan.md
sae-m53-differentiable-retrieval-finetune-plan.md
sae-m54-stage-a-distillation-plan.md
sae-m54-stage-a-distillation-results-report.md
sae-m55-stage-b-ranking-anchor-plan.md
sae-m55-stage-b-ranking-anchor-results-report.md
sae-m56-safe-stage-b-anchor-results-report.md
sae-m57-m59-query-side-blocker-results-report.md
sae-m60-scaled-supervised-stage-b-plan.md
sae-m60-scaled-supervised-stage-b-results-report.md
sae-m70-end-to-end-deep-text-to-sae-plan.md
sae-m70-end-to-end-deep-training-results-report.md
sae-m71-semantic-neighborhood-results-report.md
sae-m72-teacher-residual-results-report.md
sae-m73-query-family-diagnostics-results-report.md
sae-m74-candidate-source-curriculum-results-report.md
sae-m75-source-utility-weighted-training-results-report.md
sae-m76-source-utility-query-curriculum-results-report.md
sae-m80-two-stage-diffsae-reset-plan.md
```

M26 preserves the M24 training closure and adds one side-track research
direction from `arXiv:2605.06610`: adaptive sparse capacity. M27 has now
closed the previous exploration frame: direct text-to-atoms failed the
teacher-gap gate, SoftSAE is parked, and the SPLADE/concept control is parked.
M29 is now the latest model-side frame. It closed the query-prefix question:
Snowflake prefixed retrieval queries remain the correct canonical teacher
distribution. It also showed that ranking-first query-side training can create
strong aggregate scores, but all useful Arm A variants still collapse on
specific full15 datasets, and the transformer Arm B control is weaker. The
M30 then tested teacher-anchored residual training and a small runtime-safe
calibration head. The best M30 robustness candidate improved collapse severity
but still failed `trec-covid`, `msmarco`, and `dbpedia-entity`. The product
model therefore remains blocked on robust query-side representation and
supervision, not on Snowflake prefix formatting or scalar SAE-weight tuning.
M31 then moved the training surface to the final `BM25 + SAE` score and added
explicit BM25-preservation. This improves `msmarco` and `dbpedia-entity`
collapse in the unfrozen query-encoder run, but `trec-covid` remains far
outside the gate and SAE postings increase. The direction is promising enough
to continue with cleaner/larger train splits, but still not product-ready.
M32 then reset the data surface around official BEIR train/dev queries,
explicit test-query exclusion, and larger `20k/q300`-style candidate context.
It answered whether final-ranking text-to-atoms failed mainly because the prior
1,342-query artifact was too small. The answer is only partial: aggregate
quality improves, but product robustness does not. The best M32 teacher-anchor
fixed run reaches Recall@100 `0.8694`, MRR@20 `0.8780`, NDCG@10 `0.7734`,
and MAP@100 `0.7449`, with lower SAE postings than the fixed-doc teacher.
However, `trec-covid` remains far below teacher quality, and `msmarco` plus
`dbpedia-entity` still violate the no-collapse gate. M32 is therefore useful
evidence for the final-ranking direction, but not a promotable model.
M33 then tested the first targeted hard-family supervision arm by adding
`trec-covid` corpus-derived pseudo queries without using official test qrels.
The x1 arm only recovers `trec-covid` MAP to approximately BM25, and the x3
weighting arm worsens NDCG while barely moving MAP. This closes the simple
biomedical title/self-qrel route: the missing signal is teacher-neighborhood
or relevance distribution, not just biomedical vocabulary exposure.
M34 diagnosed why `trec-covid` is unusually hard: it has only 50 queries but
24,673 qrel pairs, a mean of 493.5 relevant documents per query, high
document-frequency content terms such as `covid` and `coronavirus`, and a
large teacher-vs-BM25 semantic-neighborhood advantage. The general next
direction is therefore teacher-neighborhood distillation for broad high-DF
queries, not `trec-covid`-specific rules.
M35 tested that next direction with teacher-neighborhood labels generated from
document titles and first useful sentences. This closed negatively: M35 q100
slightly improves aggregate quality but does not fix `trec-covid`, while the
diversified retry reduces cost but worsens the hard collapse. The current
model-side blocker is now sharper: query-distribution quality and query-side
intent representation, not simply lack of teacher-neighborhood labels.
M36 then tested whether the issue could be solved by auditing query
distribution, rejecting bad synthetic sources, and expanding clean real
many-positive train data. This also closed negatively. The audit showed
`current_eval` has 50 TREC-like broad/high-DF many-positive queries, while
M32 train has only 3. Expanded `nfcorpus` real-query replay increased training
to 4,781 queries and 126,212 qrel pairs, but the best fixed run still leaves
`trec-covid` MAP at `0.4516`, `msmarco` MAP at `0.8784`, and
`dbpedia-entity` MAP at `0.7438`, all outside the no-collapse gate. The next
model-side move should therefore be a new validated query generator or a
stronger query-side encoder under the same fixed-doc gate, not another
weight-only pass over current data.
M37 tested the stronger-encoder branch with a 1-layer
`transformer_token_lse` query-side encoder, partially initialized from the
M29 checkpoint. This shows capacity is not enough by itself. The run improves
`trec-covid` NDCG/MAP to `0.6620`/`0.4689`, but full15 mean drops to
Recall@100 `0.7868`, NDCG@10 `0.6680`, and MAP@100 `0.6275`, with large
collapses on `msmarco`, `dbpedia-entity`, and `nfcorpus`. The next step should
therefore be better hard-bucket supervision or a validated broad-query
generator before retrying deeper models.
M38 then tested the smallest possible hard-bucket supervision change:
upweighting broad/many-positive and broad/semantic-heavy examples in the M31
final-ranking objective. This closes negatively. The run weights 334 of 4,781
training examples and keeps the M36 aggregate essentially unchanged
(Recall@100 `0.8670`, NDCG@10 `0.7730`, MAP@100 `0.7432`), but `trec-covid`,
`msmarco`, and `dbpedia-entity` remain outside the no-collapse gate. The
current blocker is therefore not a scalar hard-bucket weighting problem; it is
still missing hard-query supervision quality or a structurally better
query-side representation.
M39 then built a new validated broad-query generator instead of reweighting
existing data. The generator matches the audited `trec-covid` hard-query
content-DF target (`0.1985` versus target `0.1959`) and passes the M36
synthetic validation. Training on this source still fails the product gate:
the collapse-aware best has full15 Recall@100 `0.8670`, NDCG@10 `0.7707`,
and MAP@100 `0.7435`, with `trec-covid` MAP only `0.4559`. However, the lower
fixed SAE weight `m31_fixed_w0p25` reaches `trec-covid` NDCG@10 `0.6594` and
MAP@100 `0.4822`, the best `trec-covid` MAP signal so far, while hurting
aggregate quality and other hard datasets. M39 is therefore not promotable,
but it changes the next step: the blocker now looks like query-type-aware
semantic scale/residual calibration over a better supervision source, not just
missing synthetic data volume.
M40 then tested exactly that calibration route. It adds runtime-safe lexical
DF features to the calibration path and evaluates a DF-based semantic scale
gate. The learned calibration head still over-optimizes aggregate quality and
does not solve broad-query collapse by itself, but the post-hoc DF gate forms
the strongest Pareto point so far: threshold `0.15`, low SAE weight `0.35`,
and high SAE weight `0.75` reaches full15 Recall@100 `0.8670`, NDCG@10
`0.7887`, MAP@100 `0.7596`, and `trec-covid` MAP `0.4701`. This is better
than M36 on aggregate NDCG/MAP and better than M36 on `trec-covid` MAP. The
remaining blocker is now physical cost and stability: M40's query atom
distribution raises SAE postings to `2734.7`, so the next step should preserve
the DF-gate signal while adding fanout-aware regularization/model selection.
M41 then tested that immediate cost-control step by regularizing the top-k
query atoms that actually enter the exported payload. This forms a useful
frontier rather than a single pure win. The safest point, `w0p01`, keeps M40
quality nearly intact (NDCG@10 `0.7879`, MAP@100 `0.7580`) while lowering SAE
postings from `2734.7` to `2620.5`. The aggressive point, `w0p04`, lowers SAE
postings to `2240.4` while keeping NDCG/MAP above the M36/M39 fixed-weight
baselines, but loses more of the M40 ranking gain. M41 therefore validates the
physical-cost route, but the model still does not pass the dense-removal
product gate.
M42 then tested the more structural idea: choose the physical query payload by
atom utility divided by posting fanout. The training-side utility loss has
ranking signal but tends to select high-impact/high-fanout teacher atoms, so it
is not promoted. The important result is export selection on the existing M40
model. Pruning M40 query latents to `active=48` with `fanout_power=0.5` reaches
full15 Recall@100 `0.8666`, MRR@20 `0.9000`, NDCG@10 `0.7889`, MAP@100
`0.7597`, and lowers SAE postings from `2734.7` to `1388.7`. This is the
strongest quality-cost point so far and should become the next canonical
research profile before more training is attempted.
M43 then moved that payload allocation out of the sweep script and into the
normal export/evaluation path. The integrated M43 profile exactly reproduces
the M42 sweep row: Recall@100 `0.866596`, MRR@20 `0.899967`, NDCG@10
`0.788874`, MAP@100 `0.759661`, candidate docs `2845.310`, and SAE postings
`1388.685`. M43 also fixed an important semantics issue by separating
`content_high_df_feature_threshold=0.10` from the runtime gate threshold
`df_gate_content_mean_df_threshold=0.15`. The next training work should use
M43 as the baseline, not the older unpruned M40/M41 payloads.
M44 then tested a larger emitted atom pool before export. This produced the
current strongest profile: M40 checkpoint, pool96 -> export48,
fanout_power `0.25`, gate threshold `0.12`, low SAE `0.40`, high SAE `1.00`.
It reaches Recall@100 `0.869071`, MRR@20 `0.909016`, NDCG@10 `0.794456`,
MAP@100 `0.767658`, candidate docs `2822.831`, and SAE postings `1157.431`.
This improves full15 quality over M40/M43 while reducing SAE postings by about
`58%` versus M40. M44 export-aware finetuning was also tested, but it regressed
quality to NDCG@10 `0.785630` and MAP@100 `0.753691`, so the finetune is not
promoted. The canonical model remains the M40 checkpoint with the M44 export
regime.
M45 then turned the remaining pool/export question into a reusable checkpoint
export sweep. The runner caches checkpoint query outputs and BM25 normalized
scores, then sweeps payload allocation and lexical-DF gates without repeatedly
loading the model. M45 found a higher-quality full15 profile: M40 checkpoint,
pool96 -> export48, fanout_power `0.15`, gate threshold `0.12`, low SAE
`0.45`, high SAE `1.00`. It reaches Recall@100 `0.871866`, MRR@20
`0.917690`, NDCG@10 `0.801211`, MAP@100 `0.774830`, candidate docs
`2849.424`, and SAE postings `1465.359`. This is the strongest aggregate
quality result so far, but it costs about `26.6%` more SAE postings than M44
and slightly worsens `trec-covid` MAP. M45 also found a pool128 balanced point
at fanout_power `0.10` with NDCG@10 `0.798249`, MAP@100 `0.771885`, and SAE
postings `1324.638`. M45 therefore extends the frontier rather than closing the
product gate: M44 remains the lower-cost baseline, M45 balanced is the
moderate-cost frontier, and M45 high-quality is the best aggregate-quality
research profile.
M46 then tested a runtime-safe selector over that frontier. The promoted
selector is `fanout_to_m44`: if the M45 high-quality profile predicts at least
`3000` SAE postings for a query, route that query to M44 low-cost; otherwise
use M45 high-quality. It only reroutes 48 of 1,342 queries, but reaches
Recall@100 `0.871949`, MRR@20 `0.917221`, NDCG@10 `0.801862`, MAP@100
`0.775242`, candidate docs `2844.088`, and SAE postings `1312.060`. This
improves NDCG/MAP over M45 high-quality, lowers postings by about `10.5%`, and
recovers TREC MAP to `0.463246`, close to M44. M46 is therefore the current
canonical quality-cost research profile.
M47 then tested whether direct cost-aware exported-ranking finetuning could
beat the M46 selector. It could not. The focused M47 arm lowers SAE postings to
`1049.822`, but drops to NDCG@10 `0.796081`, MAP@100 `0.766390`, and TREC MAP
`0.449518`. Running the selector on the M47 checkpoint creates an even cheaper
point at `906.341` SAE postings, but quality remains below M46 and TREC
stability remains worse than M44. M47 is parked as cost-control evidence, not
as a promoted model.
M48 then tested whether the M46 selector is just overfit to the full15
aggregate. It is stable enough to keep. LODO selection chooses the exact
canonical `fanout_to_m44@3000` rule in 13 of 15 heldout datasets. The two
exceptions, `msmarco` and `trec-covid`, pick alternative policies on the train
side but perform worse than canonical on the heldout NDCG/MAP. Family holdout
also finds no clear replacement: biomedical gets a tiny Recall/MAP gain from a
different policy but loses NDCG, web QA gets worse on all major metrics, and
the other families keep canonical. M48 therefore keeps the one-encoder,
multi-runtime-profile selector as the current product-shaped research
candidate.
M49 then converted that candidate into a generated runtime-selector contract:
`sae_runtime_selector_m49_v0`. The contract records the M40 text-to-atoms
checkpoint, the available M44/M45 export profiles, the runtime-safe
`predicted_sae_postings` selector signal, and M48's full15 plus holdout
evidence. It also keeps the important non-goals explicit: no SQL/API freeze,
no mutable index or maintenance design, no dense-removal product claim, and no
real-workload quality claim without labels. M49 is therefore an engineering
handoff artifact for a read-only prototype, not product approval.
M50 then focused on the two remaining local regressions, `trec-covid` and
`msmarco`. A targeted semantic-retention sweep over 5,760 rows found 1,269
profiles that improve both datasets versus current M46/M49 on NDCG/MAP, which
confirms that retained semantic evidence matters. The best target profile
uses `pool96 -> export96` with low/high SAE weights `0.30/0.75` and improves
`trec-covid` by NDCG@10 `+0.017992`, MAP@100 `+0.022274`, and `msmarco` by
NDCG@10 `+0.081163`, MAP@100 `+0.102927` versus current. However, no tested
row beats BM25 on both datasets for both NDCG@10 and MAP@100, and the best
target row costs `13,332` SAE postings. M50 therefore confirms semantic
retention as part of the blocker, but the next step must combine it with
BM25-preserving score calibration rather than only increasing export atoms.
M52 now resets the model-training plan around two layers. Stage A uses a broad
and balanced corpus mix, Wikipedia/arXiv/PubMed/BEIR15, to train the encoder
to preserve Snowflake-SAE teacher semantics before applying relevance labels.
Stage B then uses all available supervised retrieval sources, including BEIR,
MS MARCO, TREC Deep Learning, Natural Questions, LoTTE, and TREC-COVID/CORD-19
folds, to learn human relevance ranking. The data manifest has been generated
under `results/sae/m52/data-manifest`, and the first Stage-A corpus builder is
implemented as `scripts/research_sae_m52_corpus_builder.py`. Existing compact
BEIR/shared, official BEIR, and real-corpus pilot artifacts have been staged to
`xiaoni-mbp.local`, which has about 96GB unified memory. Spark/GB10 is prepared
as the monitored follow-up training host with an Aim UI at
`http://100.123.2.95:43800/`; the current xiaoni M52E run remains unmodified
because it was started before Aim instrumentation. M52's core purpose is to
avoid forcing a small, imbalanced qrel set to learn both semantic preservation
and final ranking.
M53 adds the next small-data fine-tuning hypothesis before scaling M52 Stage B:
keep the M52 teacher-imitation checkpoint as the semantic anchor, then apply
low-LR retrieval fine-tuning with multi-positive coverage and a train-only
soft TopK recall surrogate. This absorbs the useful `diffsae` lesson that the
deployed hard TopK selection should be represented during training, while
avoiding single-positive losses that turn other valid positives into negatives.
The implementation is in `scripts/research_sae_m53_refined_flow.py` and
`scripts/research_sae_text_atom_train.py`; the plan is
`sae-m53-differentiable-retrieval-finetune-plan.md`.
M54 tightens that into a Stage-A-only distillation plan: teacher support/value
losses are dominant, train-only soft TopK recall is a weak neighborhood-shape
regularizer, and qrel ranking is only a sanity check. The driver is
`scripts/research_sae_m54_stage_a_distill.py`, and the design is documented in
`sae-m54-stage-a-distillation-plan.md`.
M54 A/B smoke then compared checkpoint-initialized distillation against
from-scratch distillation. The initialized arm improved teacher support recall
on `scifact`/`nfcorpus` docs and queries, while the scratch arm collapsed to
only about `650` unique doc atom dimensions and failed teacher support. This
promotes `Stage-A checkpoint -> low-LR M54 distillation` as the route and
parks scratch M54 as inefficient without a separate warmup. See
`sae-m54-stage-a-distillation-results-report.md`.
The scaled Spark M54 run then used `M52E no-wiki e6` on the 750k no-Wikipedia
Stage-A corpus. `e2-clean` improved teacher-shape fidelity from docs/query
recall `0.283956`/`0.261925` to `0.310884`/`0.283413`. The automatic low-LR
`e4-cont` continuation improved pure fidelity further to `0.313332`/`0.286103`,
but full15 ranking sanity did not improve over `e2-clean`. The best balanced
checkpoint is therefore `M54 e2-clean`: full15 Recall@100 `0.798566`,
MRR@20 `0.789807`, NDCG@10 `0.678558`, MAP@100 `0.638212`. `e4-cont` remains
a semantic-fidelity control, not the default Stage-B starting point. Stop
same-loss Stage-A continuation here; the next useful step is Stage-B supervised
ranking with teacher-shape anchoring.
M55 is now the Stage-B smoke plan. It starts from `M54 e2-clean`, keeps
support/value/fanout losses as an anchor, and adds small supervised ranking,
listwise teacher, coverage, soft-topk, and candidate-budget pressure. The
runner is `scripts/research_sae_m55_stage_b_anchor.py`; the plan is
`sae-m55-stage-b-ranking-anchor-plan.md`.
The first M55 smoke on `scifact/nfcorpus` is directionally useful but not
promotable. It improves two-dataset NDCG@10 from `0.592530` to `0.596859` and
MAP@100 from `0.480114` to `0.485356`, but same-corpus `scifact` query teacher
recall drops from `0.188750` to `0.153281` and neighborhood recall drops from
`0.308500` to `0.274200`. This proves the supervised ranking signal is useful,
but the exact loss balance damages the teacher atom shape too much. Do not
scale M55 as-is; the next iteration must preserve query/doc atom fidelity more
strictly while retaining the ranking signal.
M56 applies that correction: lower LR, stronger support/value anchor, and
ranking/listwise/coverage/budget losses reduced by about 4x. On the same
`scifact/nfcorpus` smoke it improves over M54 e2 on all aggregate ranking
metrics: Recall@100 `0.640675`, MRR@20 `0.708789`, NDCG@10 `0.597724`,
MAP@100 `0.487054`. Same-corpus `scifact` query teacher recall stays close to
baseline (`0.188750 -> 0.184375`), and neighborhood recall is effectively flat
(`0.308500 -> 0.308000`). M56 is therefore the current Stage-B candidate; the
next run should widen to `scifact/nfcorpus/fiqa/scidocs` before any full15 or
Spark scale-up.

The current technical direction is no longer generic late fusion. It is a
single source-blind sparse evidence-atom engine:

```text
BM25 token atoms
+ SAE latent atoms
  -> one atom namespace
  -> impact-head candidate generation
  -> exact candidate rerank through doc-row sparse vectors
```

This direction remains attractive because it can eventually express lexical
matching and semantic latent matching inside one sparse-impact physical engine.
The main unsolved product question is not whether the signal exists. It is
whether the source-blind sparse engine can keep enough semantic recall on real
RAG workloads while staying faster and simpler than a BM25+dense two-index
hybrid.

## Current Evidence Chain

### 1. Quality Signal Exists

The current five-dataset matrix uses:

```text
datasets = scifact, scidocs, nfcorpus, arguana, fiqa
top_k = 100
sae = Snowflake 768-dim embeddings -> 8192/64 SAE profile
score mode = normalized_idf_dot
```

Mean quality:

| Path | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.6025 | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| BM25+dense | 0.6947 | 0.7817 | 0.6860 | 0.5984 | 0.5010 |
| BM25+SAE | 0.7045 | 0.7947 | 0.6835 | 0.6036 | 0.5052 |

Mean delta versus BM25:

| Path | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25+dense | +0.0922 | +0.0782 | +0.0956 | +0.0953 | +0.0877 |
| BM25+SAE | +0.1020 | +0.0912 | +0.0931 | +0.1005 | +0.0918 |

Interpretation:

- BM25+SAE beats BM25 clearly on the five-dataset matrix.
- BM25+SAE slightly beats BM25+dense on Recall@20, Recall@100, NDCG@10, and
  MAP@100.
- BM25+dense slightly beats BM25+SAE on MRR@20.
- A three-way BM25+dense+SAE path is best in the research matrix, but it keeps a
  dense side channel and therefore does not answer the single-index question.

### 2. Current Python Research Cost Is Too High

Mean query performance from the same matrix:

| Path | Mean ms | P95 ms | QPS |
| --- | ---: | ---: | ---: |
| BM25 | 1.9422 | 3.5483 | 1166.5 |
| BM25+dense | 2.4125 | 3.8083 | 536.2 |
| BM25+SAE | 5.4446 | 8.1954 | 214.8 |

Interpretation:

- The Python research BM25+SAE path is not production-shaped.
- The quality signal is real, but the product value depends on native/resident
  execution and compact payloads.
- These timings must not be treated as final PostgreSQL access-method
  performance.

### 3. Unified Evidence Payload Is Proven

`EATMH001` proved that BM25 token atoms and SAE latent atoms can share one
serialized physical payload:

```text
doc ids
atom dictionary
doc-row sparse vectors
impact-head directory
```

Mean five-dataset payload frontier:

| Payload | Recall@100 | MRR@20 | Touched postings | Candidate docs | Rerank doc terms |
| --- | ---: | ---: | ---: | ---: | ---: |
| head8 | 0.7856 | 0.6791 | 705.4 | 516.8 | 88,089.6 |
| head16 | 0.7964 | 0.6790 | 1,381.7 | 852.9 | 146,385.1 |
| head32 | 0.7948 | 0.6790 | 2,652.3 | 1,268.1 | 219,228.3 |

The selected quality guardrail remains:

```text
h16_qb0_weight / payload_head16
Recall@100 = 0.7964
MRR@20     = 0.6790
candidate docs = 852.8940
rerank terms   = 146,385.0740
```

Payload scale from the first source-blind evidence payload:

```text
head16 ~= 4.06 MB / 2k docs ~= 2.03 GB / 1M docs
```

That is plausible for a resident server-side research index, but the doc-row
section needs compression before mutable production design.

### 4. Native And PostgreSQL Feasibility Is Proven

The standalone C evidence payload reader preserved 100% doc-order parity and
reduced the head16 research path from about 7.65 ms in Python to about 0.23 ms
in standalone C on the five-dataset slice.

The PostgreSQL read-only/resident path established the implementation boundary:

```text
application stores generation once
query passes generation_id + sparse query atoms
PostgreSQL returns top-k rows with diagnostics
```

Key systems evidence:

- Parsed generation cache removed repeated bytea fetch/decode.
- Adaptive workspace kept synthetic 10k-document cached by-id latency below
  1 ms on the EATMH path while preserving exact parity.
- Existing PostgreSQL resident prototypes return diagnostics such as candidate
  docs, rerank terms, memory bytes, cache state, and payload revision.

Important caveat:

The older `SBMXM001` v4 PostgreSQL candidate path is SAE-dimension oriented,
not the final source-blind `EATMH` path. It is still useful systems evidence:
impact-head candidate generation plus row-wise doc-vector rerank can run inside
PostgreSQL with stable diagnostics.

Representative `SBMXM001` v4 PostgreSQL candidate result:

```text
PG ms/query = 1.688
Exact@100 overlap = 0.6874
Candidate docs = 292.96
Generator visits = 12
Rerank doc-vector terms = 35,136.11
Resident memory = 3.08 MB
```

This validates the mechanics of impact-head candidate generation and row-wise
doc-vector rerank, but it is not the final BM25+SAE evidence-atom product path.

### 5. M14-M17 Closed The Last Broad Exploration Round

M14 tested whether reliability selectors, graph expansion, or mixed
candidate-only atoms could produce a better selectivity frontier.

Result:

| Run | Recall@100 | MRR@20 | NDCG@10 | Candidates | Rerank terms | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| h16_base_full | 0.7964 | 0.6790 | 0.6030 | 852.8940 | 146,385.0740 | 6.7631 |
| h8_graph_candidate | 0.7951 | 0.6789 | 0.6029 | 1,212.4680 | 211,860.5960 | 9.7916 |
| h16_doc128 | 0.7937 | 0.6840 | 0.6104 | 852.8940 | 106,998.4100 | 6.3904 |
| h8_mixed_candidate | 0.7858 | 0.6791 | 0.6034 | 517.3120 | 88,162.8640 | 4.2069 |
| h8_base_full | 0.7856 | 0.6791 | 0.6034 | 516.8160 | 88,089.6020 | 4.2418 |

M14 decision:

```text
selectivity frontier 2: failed
selected quality/cost row remains h16_base_full
```

M15 tested compact doc-row rerank:

| Run | Recall@100 | MRR@20 | NDCG@10 | Candidates | Rerank terms | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| h16_doc160 | 0.7968 | 0.6891 | 0.6117 | 852.8940 | 126,629.1360 | 6.6549 |
| h16_doc128 | 0.7937 | 0.6840 | 0.6104 | 852.8940 | 106,998.4100 | 6.4705 |
| h16_doc96 | 0.7919 | 0.6760 | 0.6013 | 852.8940 | 81,717.6840 | 5.9854 |

M15 decision:

```text
doc128 passed
Recall drop = 0.0027
MRR delta   = +0.0050
rerank term reduction = 26.9%
```

M16/M17 decision:

```text
canonical arxiv/pubmed/commons qrels = not found
stable product SQL/API = blocked
read-only engineering path = allowed
```

## Product Engineering Readiness

### Can We Start Engineering?

Yes, but the scope must be narrow:

```text
read-only experimental PostgreSQL path
compact source-blind evidence payload
no stable public API
no mutable maintenance
no dense-removal claim
```

The basis is strong enough:

- BM25+SAE quality is competitive with BM25+dense on the five-dataset matrix.
- The evidence-atom abstraction unifies lexical and latent sparse features in
  one payload.
- C and PostgreSQL read-only prototypes have already demonstrated parity,
  residency, diagnostics, and cache/workspace patterns.
- `doc128` gives the first concrete cost reduction without breaking the
  quality band.

### What Engineering Should Start

M18:

```text
EATMH002 = compact top-128 doc-row evidence payload
```

Gate:

- exact parity with current `h16_doc128` simulation;
- same doc ordering within f32 score tolerance;
- diagnostics expose candidate docs, query atoms, rerank doc terms, payload
  kind, payload revision, and memory bytes.

M19:

```text
standalone C reader + PostgreSQL read-only by-id path for EATMH002
```

Gate:

- deterministic parity versus Python simulation;
- repeated benchmark pass over the five-dataset matrix;
- cached by-id behavior reuses parsed payloads;
- no public API freeze.

M20:

```text
real workload efficiency matrix
```

This should use real arxiv/pubmed/commons-style corpora and real query shapes,
but only measure runtime feasibility:

- cached latency;
- cold/decode latency;
- payload bytes;
- resident memory;
- candidate docs;
- rerank doc terms;
- head-list touches;
- cache hit state;
- result-shape sanity and overlap against current production retrieval if
  useful.

M20 must not report Recall/MRR/NDCG/MAP unless labels or defensible proxy labels
exist.

M20b:

```text
real workload quality matrix
```

This is a separate data task:

- arxiv query set plus qrels or defensible proxy qrels;
- pubmed query set plus qrels or citation/MeSH/proxy qrels;
- commons policy query set only if representative enough;
- fixed corpus snapshots and reproducible generation scripts.

M21:

```text
read-only SQL model draft
```

Allowed only after M19 parity and real-corpus M20 efficiency pass. Product
quality claims still require M20b.

## Residual Exploration

### Still Worth Exploring

- Real workload qrels/proxy-qrels. This is the only way to answer whether SAE
  can replace dense recall in the target RAG workloads.
- `EATMH002` doc-row compression. The doc-row sparse vector section dominates
  payload size and rerank cost.
- Candidate selectivity. The current `h16` row preserves quality but touches
  about 853 candidate docs; `h8` is much cheaper but loses too much Recall@100.
- Atom reliability or retrieval-aware training, but only as a side track until
  it beats the current `h16` frontier.
- Mixed token-latent atoms as candidate-only expansion. They are not calibrated
  enough for final scoring.
- BGE-M3/SPLADE stronger sparse baselines. Current SPLADE exploration did not
  beat the SAE path, but a stronger export/checkpoint could still be useful.
- Tri-hybrid BM25+dense+SAE as a quality oracle. It should guide analysis, not
  become the first single-index product design.

### Not Worth Blocking M18-M20 On

- More global fusion-weight tuning.
- More query-budget tuning without a new signal.
- Mutable index maintenance.
- Stable product API naming.
- External field signals such as time/recency.
- Direct mixed-atom final scoring.

## Current Product Risk

The major risk is not implementation feasibility. It is product relevance:

```text
Does source-blind BM25+SAE preserve enough semantic recall on arxiv/pubmed/
commons RAG and memory-retrieval workloads to justify removing or shrinking
the dense vector layer?
```

The current answer is:

```text
promising, but not proven on target workload quality
```

That is why the correct next step is not full productization. It is a
read-only engineering prototype plus real workload efficiency validation.

## Current Canonical Roadmap

| Milestone | Status | Decision |
| --- | --- | --- |
| M14 selectivity frontier 2 | Closed, failed | Keep h16 baseline; continue selectivity research as side track |
| M15 compact doc-row rerank | Closed, passed | Promote doc128 to the next payload candidate |
| M16 real workload quality readiness | Blocked | No canonical qrels/proxy-qrels found |
| M17 old SQL model gate | Superseded | Do not freeze product API |
| M18 EATMH002 payload | Implemented, passed | Compact top-128 doc-row payload is the current candidate |
| M19 C/PG compact parity | Implemented, passed | C reader and PostgreSQL by-id smoke accept EATMH002 |
| M20 efficiency runner | Implemented, benchmark pass | Measure efficiency only, no quality claim |
| M20 real workload corpus run | 5k-doc pilot passed | Scale after M27 chooses model lineage |
| M20b real workload quality matrix | Future | Build qrels/proxy-qrels before product quality claims |
| M21 read-only SQL model draft | Full15 parity passed | `EATMH001/EATMH002` runtime query by-id SQL is the evaluation harness |
| M22 SAE-SPLADE concept vocabulary | Parked by M27 | Reopen only as a true concept encoder with stronger evidence |
| M26 adaptive sparse capacity | Parked by M27 | Keep SoftSAE signals as diagnostics, not as standalone optimization |
| M27 exploration closure | Completed, failed product gate | Dense-removal not ready |
| M28 post-M27 reset | Completed, failed product gate | Teacher-path harness allowed, dense-removal blocked |
| M29 prefix-aware ranking push | Completed, failed robustness gate | Prefixed teacher canonical; query-side direct encoder still not robust |
| M30 robust query calibration | Completed, failed robustness gate | Scalar calibration improved collapse severity but did not pass no-collapse gate |
| M31 joint final-ranking training | Completed, failed robustness gate | Final-ranking supervision has signal; larger split needed |
| M32 large-split final-ranking training | Completed, failed robustness gate | Aggregate improved, but hard-family collapse remains; do not promote |
| M33 hard-family pseudo supervision | Completed, failed robustness gate | Corpus-title self-qrels are not enough; use teacher-neighborhood distillation next |
| M34 TREC-COVID difficulty analysis | Completed diagnostic | Treat as general high-DF broad-query neighborhood problem, not dataset-specific tuning |

## Bottom Line

Do not start the next product engineering stage yet. The correct label is:

```text
engineering feasible, product model not closed
```

Do not present it as:

```text
production replacement for dense retrieval
```

The current evidence supports using the product-shaped engine as an evaluation
harness. It does not yet support making final product quality claims or
removing query-time dense embedding dependency.

## M18-M20 Engineering Update

The current engineering update is saved in:

```text
sae-m18-m20-engineering-report.md
scripts/research_sae_m20_efficiency_matrix.py
results/sae/m18/eatmh002/summary.md
results/sae/m19/eatmh002-c/summary.md
results/sae/m20/efficiency-matrix/summary.md
```

Result:

```text
M18 EATMH002 compact payload: implemented and passed
M19 standalone C reader: implemented and passed
M19 PostgreSQL read-only by-id smoke: implemented and passed
M20 efficiency matrix runner: implemented and passed on benchmark artifacts
M20 real arxiv/pubmed/commons corpus pilot: passed on 5k-doc artifacts
```

Five-dataset quality/cost guardrail:

| Run | Recall@100 | MRR@20 | Candidates | Rerank terms | Payload bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| `payload_head16` | 0.7964 | 0.6790 | 852.8940 | 146,385.0740 | 4,056,935.2000 |
| `payload_head16_doc128` | 0.7937 | 0.6840 | 852.8940 | 106,998.4100 | 3,279,632.8000 |

Standalone C read-only result:

| Run | Exact ratio | Mean ms | Rerank terms |
| --- | ---: | ---: | ---: |
| `head16_scan` | 1.0000 | 0.2350 | 146,385.0740 |
| `head16_doc128_scan` | 1.0000 | 0.1726 | 106,998.4100 |

PostgreSQL smoke:

```text
PostgreSQL EATMH001/EATMH002 evidence-atom smoke passed
```

The current M20 runner is an efficiency-only harness. It intentionally does
not report Recall/MRR/NDCG/MAP. The real arxiv/pubmed/commons quality gate is
still blocked until qrels or defensible proxy-qrels exist.

The first real-corpus pilot is now saved in:

```text
sae-m20-real-corpus-pilot-report.md
```

It used 5,000 records each from arxiv, pubmed, and policy CA chunks. The pilot
found that elm commons vectors are 256-dimensional, while the full15 teacher is
768-dimensional, so it trained a separate `shared_sae_4096_64` pilot model for
efficiency validation only. The result keeps `EATMH002/head16_doc128` as the
preferred payload shape: mean C reader latency was `0.2503 ms` versus
`0.3577 ms` for the full doc-row payload, and mean rerank terms dropped from
`210,656.7400` to `136,740.0833`.

## M21-M22 Planning Update

The `EATMH001/EATMH002` evidence-atom payload family is the runtime-query
evaluation harness:

```text
ii42_evidence_atom_query
ii42_evidence_atom_query_by_id
```

This is the product-shaped read-only SQL contract because callers pass runtime
`query_atoms[]` and `query_weights[]` from an external encoder. It remains
useful for M28 because it can measure the physical cost of teacher-path or
student-path candidate atoms, but it is not a product API freeze.

`UBMXM001` also has a first PostgreSQL read-only surface:

```text
sae-unified-payload-pg-readonly-report.md
ii42_unified_payload_query
ii42_unified_payload_query_by_id
```

This remains valuable as a source-aware embedded-query research/control path,
but it is not the product API.

The M21 runtime evidence-atom parity harness is now implemented:

```text
scripts/research_sae_m21_runtime_evidence_atom_parity.py
```

The first smoke used `scifact` with two queries and temporary PostgreSQL. It
passed strict Python/C/PostgreSQL top-k parity for both `head16` and
`head16_doc128`. The full15 run has now also passed.

Full15 M21 result:

| Payload | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | C exact | PG strict | PG mean ms | Rerank terms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head16` | 0.8365 | 0.8416 | 0.7506 | 0.7148 | 1.0000 | 1.0000 | 0.8112 | 109,857.3877 |
| `head16_doc128` | 0.8354 | 0.8414 | 0.7492 | 0.7136 | 1.0000 | 1.0000 | 0.7460 | 84,820.7950 |

This is the strongest engineering evidence so far: the product-shaped
runtime-query SQL accepts external query atom ids and weights, preserves exact
Python/C/PostgreSQL parity, and gives sub-millisecond cached by-id latency on
the benchmark payloads. M28 keeps this as the teacher-path evaluation harness
while the model-side dense-removal blocker remains unresolved.

The concept-vocabulary roadmap is now archived:

```text
docs/research-sae/reports/sae-milestone22-sae-splade-concept-roadmap.md
```

The paper `From Tokens to Concepts: Leveraging SAE for SPLADE` supports a more
direct concept-vocabulary route: use SAE latents as the sparse output
vocabulary of a learned sparse retriever. The key operational changes are:

- add QD-FLOPs-style and physical posting-read costs to every quality matrix;
- build a TopK SAE-SPLADE-style control before larger model experiments;
- compare concept atoms against the current Snowflake-SAE teacher and text
  student atoms;
- test larger latent vocabularies only under fanout and posting-read gates;
- add multilingual/synonymy/polysemy diagnostics for policy and RAG workloads.

M27 parked the initial concept/SPLADE route because it did not beat the current
text-student ranking quality. M28 may reopen it only as a true
`text encoder -> SAE concept atoms` route with full quality and physical-cost
evidence. This does not change the productization gate: no dense-removal
claim, no stable API freeze, and no mutable access method.

## M26 SoftSAE Update

The SoftSAE paper adds a useful framing for the next model-side side track:
fixed TopK is an avoidable global capacity constraint. For retrieval, the
analog is that a short entity query, a multi-hop RAG query, a simple document,
and a dense scientific abstract should not necessarily share one active atom
budget.

The actionable absorption path is:

- simulate dynamic query/doc/head budgets over existing full15 artifacts;
- train a lightweight budget predictor only if simulation shows a Pareto
  frontier;
- consider retrieval-aware adaptive-k SAE training only after the predictor
  result is positive;
- keep Soft Top-K out of PostgreSQL query execution and export hard sparse
  atoms at inference.

This was a side track inside M27, not an engineering blocker. M21 runtime-query
evidence-atom SQL remains the evaluation harness.

The initial simulation runner is now implemented:

```text
scripts/research_sae_m26_dynamic_budget_sim.py
```

The full15 simulation is now available. It does not promote the initial
adaptive heuristics:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Postings | Candidates | Rerank terms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `adaptive_fanout` | 0.8009 | 0.8235 | 0.7244 | 0.6638 | 624.5487 | 483.5170 | 39,447.6629 |
| `fixed_q64_d96_h12` | 0.8197 | 0.8289 | 0.7352 | 0.6909 | 667.9452 | 523.8214 | 48,782.0034 |
| `fixed_q96_d128_h16` | 0.8334 | 0.8387 | 0.7467 | 0.7110 | 985.1131 | 721.2118 | 81,994.6103 |

`adaptive_fanout` proves there is cost headroom, but the quality loss is too
large for promotion. The follow-up learned selector was completed in M27 and
parked because it preserved quality but reduced postings by only `3.27%`, below
the required cost gate.

## M27 Exploration Closure Update

The archived closure plan is:

```text
docs/research-sae/reports/sae-m27-exploration-closure-plan.md
```

M27 supersedes the previous "start engineering next" framing. The remaining
product blocker is model-side: the direct text-to-atoms student is close to
the Snowflake-derived teacher on Recall@100 but still behind on MRR/NDCG/MAP.

Current closure gap:

| Metric | Gap |
| --- | ---: |
| Recall@100 | -0.0131 |
| MRR@20 | -0.0171 |
| NDCG@10 | -0.0305 |
| MAP@100 | -0.0360 |

M27 originally tracked:

| Track | Status | Decision rule |
| --- | --- | --- |
| Text-to-atoms | `open-final-push` | close teacher gap or record dense-removal not ready |
| SoftSAE selector | `open-final-push` | preserve fixed-high quality while lowering postings or park |
| Concept vocabulary | `open-final-push` | beat current student or improve cost frontier, otherwise park |
| EATMH/M21 | `promoted-harness` | keep for parity and physical-cost measurement only |

The actual M27 exit decision is recorded below and is now the input to M28.

## M27 Execution Result

The M27 closure runner is now implemented:

```text
scripts/research_sae_m27_exploration_closure.py
sae-m27-exploration-closure-report.md
```

Full15 execution result:

| Track | Decision | Key result |
| --- | --- | --- |
| Text-to-atoms | `open-final-push-failed` | scanned `290` full15 student rows; best remains `baseline_budget16` and still fails teacher-gap gates |
| SoftSAE selector | `parked` | learned rule `coverage_slope>=7.25`; quality is close to fixed high, but postings drop is only `3.27%` |
| Concept vocabulary | `parked` | `bm25_sae_splade` control is below the current text-student on ranking quality |

Final M27 exit:

```text
dense-removal not ready
```

Engineering status remains:

```text
engineering feasible
product model not closed
no query-time dense-removal claim
```

The next engineering-safe path is to keep EATMH/M21 as a read-only evaluation
harness or teacher-path prototype. Productization of direct `text -> sparse
atoms` should wait for a materially stronger model architecture or supervision
source.

## M28/M29/M30/M31 Model Blocker Updates

The active next-phase plan is:

```text
sae-m28-post-m27-next-phase-plan.md
```

The first M28 results report is:

```text
sae-m28-post-m27-results-report.md
```

M28 is not a continuation of the same tiny text-student weight/budget sweeps.
It separates the safe engineering line from the blocked product-model line:

| Track | M28 status | Rule |
| --- | --- | --- |
| Teacher-path harness | allowed | keep `EATMH002/doc128` and M21 parity/cost harnesses read-only |
| Direct text-to-atoms | reset required | new experiments must change encoder architecture, retrieval supervision, concept vocabulary, or teacher objective |
| Concept vocabulary | reopened only as true concept encoder | compare against BM25, `teacher_16384`, and current `baseline_budget16` |
| SoftSAE | diagnostic only | keep coverage slope, entropy, top mass, and fanout signals; no standalone selector loop |

M28 label:

```text
teacher-path harness allowed, dense-removal blocked
```

First-pass result:

| Track | Result |
| --- | --- |
| T1 teacher-path harness | still allowed; EATMH002 full15 PG strict parity remains `1.0000` and cached by-id mean latency remains sub-ms |
| T2 pretrained text-to-atoms | failed current gate; full15-trained MiniLM projection reached Recall@100 `0.7998`, NDCG@10 `0.6803`, MAP@100 `0.6441` |
| T3 concept vocabulary | parked after M28 control; best query-side concept route reached Recall@100 `0.8013`, NDCG@10 `0.6790`, MAP@100 `0.6461` |
| T4 SoftSAE | diagnostic-only; no standalone selector loop |

The current product gate remains blocked by model quality, not by the
read-only PostgreSQL/runtime harness.

M29 then tested the most likely query-side blocker:

```text
sae-m29-prefix-aware-ranking-results-report.md
```

M29 result:

| Track | Result |
| --- | --- |
| Prefix diagnostic | prefixed Snowflake query teacher remains canonical; raw/no-prefix and mixed teachers do not beat it |
| Arm A ranking-first | strong qrel target beats teacher aggregate but collapses `trec-covid`, `msmarco`, and `dbpedia-entity` |
| Arm A prefix/style | query/style input prefixes do not remove the same robustness failures |
| Arm B transformer | same ranking loss with transformer checkpoint stays below the baseline frontier |
| Doc-side training | not started because fixed-doc query-side gate failed |

Current M29 label:

```text
prefix canonical, query-side dense-removal still blocked
```

M30 then tested the recommended conservative calibration route:

```text
sae-m30-robust-query-calibration-results-report.md
```

M30 result:

| Track | Result |
| --- | --- |
| Collapse taxonomy | failure is dominated by BM25 suppression, not raw prefix mismatch or only SAE over-promotion |
| Residual-only training | too weak; full15 quality falls below the teacher gate |
| Residual plus calibration head | improves residual-only but remains below quality gate |
| Frozen encoder plus calibration head | preserves aggregate quality and reduces collapse severity, but still fails `trec-covid`, `msmarco`, and `dbpedia-entity` |

Current M30 label:

```text
scalar calibration helps diagnosis but does not close robustness blocker
```

M31 moved the objective closer to the final product scoring path:

```text
sae-m31-joint-final-ranking-results-report.md
```

M31 result:

| Track | Result |
| --- | --- |
| Frozen encoder + joint weight head | mostly reproduces M29/M30 fixed-weight tradeoff; does not solve collapse |
| Unfrozen low-LR query encoder | reduces `dbpedia-entity` and `msmarco` collapse, but `trec-covid` remains a hard failure |
| Physical cost | unfrozen run increases SAE postings from `1738.0` to `2851.2`, so quality gains are not free |
| Data scope | current training uses 49,059 docs and 1,342 queries, about `0.14%` docs and `0.17%` queries versus local official BEIR15 files |

Final M31 label:

```text
final-ranking training has signal, but current sampled query artifact is too small for product gate closure
```

M32 training reports:

```text
sae-m32-large-split-final-ranking-training-plan.md
sae-m32-large-split-artifact-build-report.md
sae-m32-large-split-training-results-report.md
```

M32 changed the next step from another small loss sweep to a clean data split:

| Track | Result |
| --- | --- |
| Data | official-BEIR train/dev split built with current/test query IDs excluded |
| Corpus context | `20k/q300` train artifact has 128,816 docs, 2,167 queries, and 18,957 qrel pairs |
| Materialization | 8/8 train datasets materialized into Snowflake/SAE teacher atoms, 2.4 GB total |
| Candidate coverage | teacher top20 coverage `0.9961`; teacher top100 coverage `0.9949` |
| Objective | M31 final-ranking training rerun on the larger clean split; teacher-anchor variant also tested |
| Gate | aggregate improved, but `trec-covid`, `msmarco`, and `dbpedia-entity` still fail no-collapse gate |

M32.0 artifact build result:

| Artifact | Datasets | Docs | Queries | Qrel pairs |
| --- | ---: | ---: | ---: | ---: |
| `m32-train-20k-q300` | 8 | 128,816 | 2,167 | 18,957 |
| `m32-test-official` | 15 | 257,490 | 3,741 | 59,123 |

Leakage check:

```text
leaks = []
```

M32.2 training result:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| Teacher fixed-doc | 0.8447 | 0.8399 | 0.7490 | 0.7247 |
| M31 unfrozen best | 0.8648 | 0.8721 | 0.7653 | 0.7359 |
| M32 teacher-anchor best fixed | 0.8694 | 0.8780 | 0.7734 | 0.7449 |

Current M32 label:

```text
large clean data improves aggregate quality, but hard-family robustness is still blocked
```

The next useful training phase should not be another global weight sweep. It
should add targeted biomedical/claim-heavy supervision while preserving the
final-ranking objective and fixed teacher doc atoms.

M33 hard-family pseudo reports:

```text
sae-m33-hard-family-supervision-plan.md
sae-m33-hard-family-supervision-results-report.md
```

M33 first arm:

| Track | Result |
| --- | --- |
| Data | 20k `trec-covid` documents and 300 corpus-derived pseudo queries |
| Leakage | no official `trec-covid` test qrels used as train labels |
| x1 result | weak MAP recovery only; `trec-covid` NDCG/MAP still far below teacher |
| x3 weighting | no useful gain; NDCG worsens while MAP barely moves |
| Decision | pseudo self-qrels not promoted |

Current M33 label:

```text
hard-family vocabulary exposure is not enough; need teacher-neighborhood or stronger query encoder
```

M34 diagnostic report:

```text
sae-m34-trec-covid-difficulty-analysis-report.md
```

M34 conclusion:

| Finding | Evidence |
| --- | --- |
| Dense qrels | `trec-covid` has 24,673 qrel pairs for 50 queries, mean 493.5 relevant docs/query |
| Low Recall@100 ceiling | mean max possible Recall@100 is only 0.2674 |
| Weak lexical selectivity | content query mean DF fraction is 0.1909, highest in full15 |
| Teacher advantage | teacher MAP@100 0.7639 vs BM25 0.4693 |
| Student failure | M32/M33 student stays around MAP@100 0.45-0.47 |

Current M34 label:

```text
trec-covid exposes broad-query semantic-neighborhood learning failure, not a need for dataset-specific tuning
```

## Report Archive Update

Closed historical root-level reports, plans, roadmaps, and design summaries have
been moved to:

```text
docs/research-sae/reports/
```

Only the project `README.md`, the active status entrypoint, M27/M28 closure
inputs, and the active M29/M30/M31/M32/M33 files remain in the repository root. Older
sections that mention a bare report or plan filename
should be interpreted as referring to the archived file under
`docs/research-sae/reports` unless that filename still exists at the root.

Remaining root markdown files:

| File | Why it remains at root |
| --- | --- |
| `README.md` | project entrypoint, not a research planning/report artifact |
| `docs/research-sae/reports/milestones/sae-current-status-and-engineering-readiness-report.md` | active status entrypoint and current decision summary |
| `sae-m27-exploration-closure-report.md` | latest closure report for current review |
| `sae-m28-post-m27-next-phase-plan.md` | retained M28 plan input |
| `sae-m28-post-m27-results-report.md` | retained M28 implementation result and blocker decision |
| `sae-m29-prefix-aware-ranking-results-report.md` | latest M29 blocker result |
| `sae-m30-robust-query-calibration-results-report.md` | latest M30 calibration result |
| `sae-m31-joint-final-ranking-results-report.md` | M31 final-ranking result |
| `sae-m32-large-split-final-ranking-training-plan.md` | M32 large-split plan |
| `sae-m32-large-split-artifact-build-report.md` | M32 split artifact result |
| `sae-m32-large-split-training-results-report.md` | latest M32 training result |
| `sae-m33-hard-family-supervision-plan.md` | active hard-family follow-up plan |
| `sae-m33-hard-family-supervision-results-report.md` | latest M33 pseudo-query result |
| `sae-m34-trec-covid-difficulty-analysis-report.md` | latest hard-dataset diagnostic |
